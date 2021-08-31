#include ".//inc/socketManager.h"

static socket_t* socketList[MAX_N_SOCKETS];


static void socket_cb(SOCKET socketId, uint8_t u8Msg, void *pvMsg);
static void dnsCallback(uint8* pu8HostName, uint32 u32ServerIP);
int32_t getFreeSocketIndex (void);
int32_t getIndexFromSocket (socket_t* sock);


bool dnsResolved;
uint32_t dnsResolvedIpAddr;


void socketManager_loadConfigDefaults (socket_config_t* config)
{
	config->server = 0;
	config->ip = 0;
	config->port = 0;
	config->isTCP = 1;
	config->isSecure = 0;
	config->bufferIn = NULL;
	config->bufferOut = NULL;
	config->callback_connect = NULL;
	config->callback_received = NULL;
	config->callback_sent = NULL;
	config->callback_closed = NULL;
	config->callback_server_connection_accepted = NULL;
	config->callback_client_closed = NULL;
}

void socketManager_init (void)
{
	socketInit();
	registerSocketCallback(socket_cb, dnsCallback);
	
	for (int i = 0; i < MAX_N_SOCKETS; i++) {
		socketList[i] = NULL;
	}
}


int32_t socketManager_createSocket (socket_t* sock, socket_config_t* config) 
{
	SOCKET id;
	int32_t index;
	
	index = getFreeSocketIndex();
	
	if (index >= 0)
	{
		sock->connected = 0;
		sock->server = config->server;
		sock->ip = config->ip;
		sock->port = config->port;
		sock->socketId = -1;
		sock->listIndex = index;
		sock->isSecure = config->isSecure;
		sock->isTCP = config->isTCP;
		
		sock->listening = 0;
		sock->clientSocketId = -1;
		
		sock->bufferIn = config->bufferIn;
		sock->bufferOut = config->bufferOut;
		sock->bufferIn_len = config->bufferIn_len;
		sock->bufferOut_len = config->bufferOut_len;
		
		sock->bufferIn_ptr = 0;
		sock->bufferOut_ptr = 0;
		
		sock->callback_connect = config->callback_connect;
		sock->callback_received = config->callback_received;
		sock->callback_sent = config->callback_sent;
		sock->callback_closed = config->callback_closed;
		sock->callback_server_connection_accepted = config->callback_server_connection_accepted;
		sock->callback_client_closed = config->callback_client_closed;
		
		socketList[index] = sock;
		
		return 0;
	}
	
	return -1;
	
}


void socketManager_deleteSocket (socket_t* sock) 
{
	socketList[sock->listIndex] = NULL;
	sock->listIndex = -1;
}


int32_t socketManager_connect (socket_t* sock) 
{
	struct sockaddr_in addr;
	int32_t ret;
	
	
	if (sock->socketId < 0) {
		if (sock->isTCP) {
			sock->socketId = (sock->isSecure)? socket(AF_INET, SOCK_STREAM, SOCKET_FLAGS_SSL) : socket(AF_INET, SOCK_STREAM, 0);
		}
		else {
			sock->socketId = socket(AF_INET, SOCK_DGRAM, 0);
		}
		
		if (sock->socketId < 0) {
			return -1;
		}
		
		
		if (sock->server == 0 && sock->isTCP) {
			addr.sin_family = AF_INET;
			addr.sin_port = _htons(sock->port);
			addr.sin_addr.s_addr = _htonl(sock->ip);
			
			ret = connect(sock->socketId, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		}
		else if (sock->server == 1) {
			addr.sin_family = AF_INET;
			addr.sin_port = _htons(sock->port);
			addr.sin_addr.s_addr = 0;
			
			ret = bind(sock->socketId, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		}

		if (ret < 0) {
			close(sock->socketId);
			sock->socketId = -1;
			
			return -1;
		}

		return 0;
	}
	
	return 0;
}


void socketManager_close (socket_t* sock)
{
	socketManager_closeClient(sock);
	close(sock->socketId);
	
	sock->listening = 0;
	sock->connected = 0;
	sock->socketId = -1;
	
	if (sock != NULL && sock->callback_closed != NULL) {
		sock->callback_closed(sock->socketId);
	}
}


void socketManager_closeClient (socket_t* sock)
{
	if (sock->clientSocketId >= 0) {
		close(sock->clientSocketId);
		
		if (sock->callback_client_closed != NULL) {
			sock->callback_client_closed(sock->socketId);
		}
	}
	
	sock->clientSocketId = -1;
}


int32_t socketManager_bufferOutPutByte (socket_t* sock, uint8_t data)
{
	if (sock->bufferOut_ptr < sock->bufferOut_len) {
		sock->bufferOut[sock->bufferOut_ptr] = data;
		sock->bufferOut_ptr ++;
		
		return 0;
	}
	
	return -1;
}


int32_t socketManager_bufferOutPutInt16 (socket_t* sock, uint16_t data)
{
	if ((sock->bufferOut_len - sock->bufferOut_ptr) > 1) {
		sock->bufferOut[sock->bufferOut_ptr] = (uint8_t)((data >> 8) & 0x00FF);
		sock->bufferOut_ptr ++;
		sock->bufferOut[sock->bufferOut_ptr] = (uint8_t)(data & 0x00FF);
		sock->bufferOut_ptr ++;
		
		return 0;
	}
	
	return -1;
}


int32_t socketManager_bufferOutPutBytes (socket_t* sock, uint8_t* data, uint32_t len)
{
	uint32_t bytesWritten = 0;
	uint32_t dataOffset = sock->bufferOut_ptr;
	
	
	for (uint32_t i = 0; i < len; i++) {
		
		if ( (dataOffset + i) < sock->bufferOut_len) {
			sock->bufferOut[dataOffset + i] = data[i];
			sock->bufferOut_ptr ++;
			
			bytesWritten ++;
		}
		else {
			break;
		}
	}
	
	return bytesWritten;
}


uint8_t socketManager_bufferOutGetChecksum (socket_t* sock)
{
	uint32_t sum = 0;
	uint8_t checksum = 0;
	
	
	for (int i = 0; i < sock->bufferOut_ptr; i++) {
		sum += sock->bufferOut[i];
	}
	
	checksum = sum & 0x000000ff;
	
	return (0xff - checksum);
}


void socketManager_flushBufferOut (socket_t* sock) 
{
	sock->bufferOut_ptr = 0;
}


int32_t socketManager_send (socket_t* sock)
{
	if (sock->server == 1) {
		send(sock->clientSocketId, sock->bufferOut, sock->bufferOut_ptr, 0);
	}
	else {
		if (sock->isTCP == 1) {
			send(sock->socketId, sock->bufferOut, sock->bufferOut_ptr, 0);
		}
	}

	
}

int32_t socketManager_sendUdp (socket_t* sock, struct sockaddr* addr)
{
	sendto(sock->socketId, sock->bufferOut, sock->bufferOut_ptr, 0, addr, sizeof(struct sockaddr_in));
}


int32_t socketManager_startReceiving (socket_t* sock)
{
	if (sock->server == 1) {
		recv(sock->clientSocketId, &(sock->bufferIn[sock->bufferIn_ptr]), sock->bufferIn_len - sock->bufferIn_ptr, 0);
	}
	else {
		if (sock->isTCP == 1) {
			recv(sock->socketId, &(sock->bufferIn[sock->bufferIn_ptr]), sock->bufferIn_len - sock->bufferIn_ptr, 0);
		}
	}
}


int32_t socketManager_startReceivingUdp (socket_t* sock)
{
	recvfrom(sock->socketId, &(sock->bufferIn[sock->bufferIn_ptr]), sock->bufferIn_len - sock->bufferIn_ptr, 0);
}


void socketManager_flushBufferIn (socket_t* sock)
{
	sock->bufferIn_ptr = 0;
}


uint32_t socketManager_isConnected (socket_t* sock)
{
	if (sock->connected == 1) {
		return 1;
	}
	else {
		return 0;
	}
}

void socketManager_resolveHostName (uint8_t* url)
{
	gethostbyname(url);
	dnsResolved = false;
}


bool socketManager_isDnsResolved (void)
{
	return dnsResolved;
}


void socketManager_dnsResolvedDismiss (void)
{
	dnsResolved = false;
}


uint32_t socketManager_getDnsResolution (void)
{
	return dnsResolvedIpAddr;
}


/**
 * \brief Callback to get the Data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb(SOCKET socketId, uint8_t u8Msg, void *pvMsg)
{	
	socket_t* sock = socketManager_getSocketBySocketId(socketId);
	
	if (sock == NULL) {
		return;	
	}
	
	switch (u8Msg) {
		
		/* server socket bind */
		case  SOCKET_MSG_BIND:
		{
			tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg*)pvMsg;
			
			if(pstrBind->status == 0) {
				listen(sock->socketId, 0);
				sock->listening = 1;
			}
			else {
				socketManager_close(sock);
			}
		}
		break;
		
		/* socket listening */
		case SOCKET_MSG_LISTEN:
		{
			tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg*)pvMsg;
			
			if(pstrListen->status != 0) {
				socketManager_close(sock);
			}
		}
		break;
		
		/* connection accepted */
		case SOCKET_MSG_ACCEPT:
		{
			// New Socket is accepted.
			tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
			if(pstrAccept->sock >= 0)
			{
				// Get the accepted socket.
				sock->clientSocketId = pstrAccept->sock;
				
				if (sock->callback_server_connection_accepted != NULL) {
					sock->callback_server_connection_accepted(sock->socketId);
				}
			}
			else
			{
#ifdef DEBUG_PRINTF
				printf("Accept Failed\n");
#endif
				socketManager_close(sock);
			}
		}
		break;
				
		
		
		/* Socket connected */
		case SOCKET_MSG_CONNECT:
		{
			tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
		
			if (pstrConnect && pstrConnect->s8Error >= 0) {
				sock->connected = 1;
			}
		
			if (sock != NULL && sock->callback_connect != NULL) {
				sock->callback_connect(sock->socketId, pstrConnect);
			}
		
		}
		break;


		/* Message send */
		case SOCKET_MSG_SEND:
		{
			if (sock != NULL && sock->callback_sent != NULL) {
				sock->callback_sent(sock->socketId);
			}
		}
		break;


		/* Message receive */
		case SOCKET_MSG_RECV:
		case SOCKET_MSG_RECVFROM:
		{
			tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
		
			if (sock != NULL && sock->callback_received != NULL) {
				if (pstrRecv && pstrRecv->s16BufferSize > 0) {
					sock->bufferIn_ptr += pstrRecv->s16BufferSize;
					sock->callback_received(sock->socketId, pstrRecv);
				} 
				else {
					if (sock->server == 1) {
						socketManager_closeClient(sock);
					}
					else {
						socketManager_close(sock);
					}
				}
			}
		}

		break;

		default:
			break;
	}
}


static void dnsCallback(uint8* pu8HostName, uint32 u32ServerIP)
{
	if(u32ServerIP != 0)
	{
		dnsResolvedIpAddr = u32ServerIP;
		
		dnsResolved = true;
	}
	else {
		dnsResolved = false;
	}
}




int32_t getFreeSocketIndex (void) {
	int32_t i;
	
	for (i = 0; i < MAX_N_SOCKETS; i++) {
		if (socketList[i] == NULL) {
			break;
		}
	}
	
	if (i < MAX_N_SOCKETS) {
		return i;
	}
	else {
		return -1;
	}
}


int32_t getIndexFromSocket (socket_t* sock)
{
	int32_t i;
	
	for (i = 0; i < MAX_N_SOCKETS; i++) {
		if (socketList[i] == sock) {
			break;
		}
	}
	
	if (i < MAX_N_SOCKETS) {
		return i;
	}
	else {
		return -1;
	}
}

socket_t* socketManager_getSocketBySocketId (SOCKET id)
{
	socket_t* sock;
	uint32_t i;
	
	for (i = 0; i < MAX_N_SOCKETS; i++) {
		if (socketList[i] != NULL) {
			if (socketList[i]->socketId == id || socketList[i]->clientSocketId == id) {
				sock = socketList[i];
				break;
			}
		}
	}
	
	if (i < MAX_N_SOCKETS) {
		return sock;
	}
	else {
		return NULL;
	}
	
}


uint32_t socketManager_isListening (socket_t* sock)
{
	if (sock->listening == 1) {
		return 1;
	}
	else {
		return 0;
	}
}


uint32_t socketManager_socketServerIsClientConnected (socket_t* sock)
{
	if (sock->clientSocketId >= 0) {
		return 1;
	}
	else {
		return 0;
	}
}