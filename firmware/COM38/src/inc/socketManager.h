#ifndef SOCKETMANAGER_H_
#define SOCKETMANAGER_H_

#include "asf.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"



#define MAX_N_SOCKETS		5


typedef struct
{
	SOCKET socketId;
	int32_t listIndex;
	
	uint32_t isTCP;
	uint32_t isSecure;
	
	uint32_t server;
	uint32_t listening;
	SOCKET clientSocketId;
	
	uint32_t port;
	uint32_t ip;
	
	uint32_t connected;
	
	void (*callback_connect) (SOCKET id, tstrSocketConnectMsg* msg);
	void (*callback_sent) (SOCKET id);
	void (*callback_received) (SOCKET id, tstrSocketRecvMsg* msg);
	void (*callback_closed) (SOCKET id);
	void (*callback_server_connection_accepted) (SOCKET id); 
	void (*callback_client_closed) (SOCKET id); 
	
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint32_t bufferIn_len;
	uint32_t bufferOut_len;
	
	uint32_t bufferIn_ptr;
	uint32_t bufferOut_ptr;
}socket_t;


typedef struct
{
	void (*callback_connect) (SOCKET id, tstrSocketConnectMsg* msg);
	void (*callback_sent) (SOCKET id);
	void (*callback_received) (SOCKET id, tstrSocketRecvMsg* msg);
	void (*callback_closed) (SOCKET id);
	void (*callback_server_connection_accepted) (SOCKET id); 
	void (*callback_client_closed) (SOCKET id); 
	
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint32_t bufferIn_len;
	uint32_t bufferOut_len;
	uint32_t server;
	uint32_t ip;
	uint32_t port;
	uint32_t isTCP;
	uint32_t isSecure;
}socket_config_t;


void socketManager_loadConfigDefaults (socket_config_t* config);
void socketManager_init (void);
int32_t socketManager_createSocket (socket_t* sock, socket_config_t* config);
void socketManager_deleteSocket (socket_t* sock);
int32_t socketManager_connect (socket_t* sock);
void socketManager_close (socket_t* sock);
void socketManager_closeClient (socket_t* sock);
int32_t socketManager_bufferOutPutByte (socket_t* sock, uint8_t data);
int32_t socketManager_bufferOutPutBytes (socket_t* sock, uint8_t* data, uint32_t len);
uint8_t socketManager_bufferOutGetChecksum (socket_t* sock);
int32_t socketManager_bufferOutPutInt16 (socket_t* sock, uint16_t data);
void socketManager_flushBufferOut (socket_t* sock);
int32_t socketManager_send (socket_t* sock);
int32_t socketManager_sendUdp (socket_t* sock, struct sockaddr* addr);
int32_t socketManager_startReceiving (socket_t* sock);
int32_t socketManager_startReceivingUdp (socket_t* sock);
void socketManager_flushBufferIn (socket_t* sock);
uint32_t socketManager_isConnected (socket_t* sock);
socket_t* socketManager_getSocketBySocketId (SOCKET id);
uint32_t socketManager_isListening (socket_t* sock);
uint32_t socketManager_socketServerIsClientConnected (socket_t* sock);
void socketManager_resolveHostName (uint8_t* url);
bool socketManager_isDnsResolved (void);
uint32_t socketManager_getDnsResolution (void);
void socketManager_dnsResolvedDismiss (void);


#endif /* SOCKETMANAGER_H_ */