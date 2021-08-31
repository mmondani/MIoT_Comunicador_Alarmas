#include "asf.h"
#include "inc/imClient.h"
#include "inc/project.h"
#include "inc/softTimers.h"
#include "inc/socketManager.h"
#include "inc/WiFiManager.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/dateTime.h"
#include "inc/DateTime_t.h"
#include "inc/pga.h"


enum{
	FSM_INIT = 0,
	FSM_GET_SOCKET,
	FSM_IDLE,
	FSM_MSG_SEND,
	RESOLVE_DNS,
	WAIT_DNS_RESPONSE,
	FSM_CONNECT_SOCKET,
	FSM_WAIT_ACK,
	FSM_WAIT_CONNECTION,
	FSM_WAIT_LOGIN_RESPONSE
	};
uint32_t fsmState;	


static uint8_t myId[4];

static socket_t socketIm;
static socket_config_t socketConfig;
static SoftTimer_t timerAux;
static SoftTimer_t timerCheckConnection;
static SoftTimer_t timerKeepalive;
static SoftTimer_t timerResponseTimeout;
static SoftTimer_t timerBuffer;
static uint8_t sendingError;
static bool needToSendLoginMessage;
static bool authenticated;
static bool inLoginProcess;
static bool resetConnection;


static uint8_t buffIn[200];
static uint8_t buffOut[200];
static uint8_t toAux[4];


static void socket_connect (SOCKET id, tstrSocketConnectMsg* msg);
static void socket_sent (SOCKET id);
static void socket_received (SOCKET id, tstrSocketRecvMsg* msg);
static void socket_closed (SOCKET id);
static void generateFrameToSend (imMessage_t* msg);
static void parseMessageIn (uint8_t* data);


void imClient_init (uint8_t* id)
{
	fsmState = FSM_INIT;
	
	messagePool_init();
	
	softTimer_init(&timerCheckConnection, 1000);
	
	for (int i = 0; i < 4; i++) {
		myId[i] = id[i];
	}	
	
	needToSendLoginMessage = false;
	authenticated = false;
	inLoginProcess = false;
	resetConnection = false;
}


uint32_t imClient_isClientConnected (void)
{
	if (socketManager_isConnected(&socketIm) && authenticated) {
		return 1;
	}
	
	return 0;
}


void imClient_resetConnection (void)
{
	resetConnection = true;
}


imMessage_t* imClient_getFreeMessageSlot (void)
{
	return messagePool_getFreeSlot();
}


void imClient_releaseMessageSlot (imMessage_t* msg)
{
	messagePool_releaseSlot(msg);
}


uint32_t imClient_putPayloadByte (imMessage_t* msg, uint8_t data)
{
	uint32_t i;
	
	if (msg->payload_ptr > MESSAGE_POOL_MESSAGE_MAX_LEN) {
		return 0;
	}
	
	msg->payload[msg->payload_ptr] = data;
	msg->payload_ptr++;
	
	return 1;
}


uint32_t imClient_putPayloadBytes (imMessage_t* msg, uint8_t* data, uint32_t len)
{
	uint32_t i;

	for(i = 0; i < len; i++) {
		if (msg->payload_ptr > MESSAGE_POOL_MESSAGE_MAX_LEN) {
			break;
		}
		
		msg->payload[msg->payload_ptr] = data[i];
		msg->payload_ptr++;
	}
	
	return i;
}


void imClient_send (imMessage_t* msg, uint8_t* to, uint32_t flow, uint8_t cmd, uint16_t reg, uint8_t part, uint8_t qos)
{
	dateTime_t now;
	
	dateTime_now(&now);
	
	
	for (int i = 0; i < 4; i++) {
		msg->to[i] = to[i];
	}
	
	msg->flow = flow;
	msg->cmd = cmd;
	msg->reg = reg;
	msg->part = part;
	msg->qos = qos;
	msg->timestamp.dia = now.dia;
	msg->timestamp.mes = now.mes;
	msg->timestamp.anio = now.anio;
	msg->timestamp.horas = now.horas;
	msg->timestamp.minutos = now.minutos;
	msg->timestamp.segundos = now.segundos;
	
	messagePool_pushOutputQueue(msg);
	
	// Tiempo que se van a retener los eventos. Si este tiempo pasa,
	// los eventos en el buffer se eliminan.
	softTimer_init(&timerBuffer, TIMER_BUFFER);
}


uint32_t imClient_messagePendingToRead (uint32_t flow)
{
	return messagePool_pendingInputMessage(flow);
}


imMessage_t* imClient_getMessageToRead (uint32_t flow)
{
	return messagePool_peekInputQueue(flow);
}


void imClient_removeMessageToRead (uint32_t flow)
{
	messagePool_popInputQueue(flow);
	
}



void imClient_handler (void)
{
	imMessage_t* msg;
	
	
	if (softTimer_expired(&timerBuffer)) {
		if (messagePool_pendingOutputMessage() > 0) {
			// Si hay mensajes que no se pudieron enviar, se los borra
			messagePool_flushOutputQueue();
			
			//TO-DO: hay que señalizar de alguna forma esto??
		}
	}
	
	
	switch (fsmState) {
		
		case FSM_INIT:
			socketManager_loadConfigDefaults(&socketConfig);
			//socketConfig.ip = MAIN_WIFI_M2M_IM_SERVER_IP;
			//socketConfig.port = MAIN_WIFI_M2M_IM_SERVER_PORT;
			socketConfig.callback_connect = socket_connect;
			socketConfig.callback_received = socket_received;
			socketConfig.callback_sent = socket_sent;
			socketConfig.callback_closed = socket_closed;
			socketConfig.isSecure = 1;
			socketConfig.bufferIn = buffIn;
			socketConfig.bufferOut = buffOut;
			socketConfig.bufferIn_len = sizeof(buffIn);
			socketConfig.bufferOut_len = sizeof(buffOut);
			
			softTimer_init(&timerAux, 0);
			softTimer_init(&timerKeepalive, TIMER_KEEPALIVE);
			
			sendingError = 0;
			
			fsmState = FSM_GET_SOCKET;
			
			break;
		
		case FSM_GET_SOCKET:
		
			if (softTimer_expired(&timerAux)) {
				if (socketManager_createSocket(&socketIm, &socketConfig) < 0) {
					softTimer_init(&timerAux, 2000);
				}
				else {
					fsmState = FSM_IDLE;
				}
			}
			
			break;
			
			
		case RESOLVE_DNS:
			if (wifiManager_isWiFiConnected() != WIFI_MANAGER_WIFI_CONNECTED || 
				wifiManager_isProvisioningEnable() == 1 ||
				socketManager_isConnected(&socketIm) == 1) {
					
				fsmState = FSM_IDLE;
			}
			else {
				socketManager_resolveHostName((uint8*)&(pgaData[PGA_BROKER_URL]));
			
				softTimer_init(&timerAux, 5000);
				fsmState = WAIT_DNS_RESPONSE;
			}
			
			break;
			
			
		case WAIT_DNS_RESPONSE:
			if (socketManager_isDnsResolved()) {
				uint32_t ipAddr = socketManager_getDnsResolution();
				struct sockaddr_in strAddr;
				uint32_t port;
				
				socketManager_dnsResolvedDismiss();
				
				port = (pgaData[PGA_BROKER_PORT + 1] << 8) | pgaData[PGA_BROKER_PORT];
			
				socketIm.ip = _ntohl(ipAddr);
				socketIm.port = port;
				
				fsmState = FSM_CONNECT_SOCKET;
			}
			else if (softTimer_expired(&timerAux)) {
				fsmState = FSM_IDLE;
			}
			break;
			
		case FSM_CONNECT_SOCKET:
		
			if (wifiManager_isWiFiConnected() == WIFI_MANAGER_WIFI_CONNECTED) {
				if (socketManager_isConnected(&socketIm) == 0) {
					if (wifiManager_isProvisioningEnable() == 0) {
						if (socketManager_connect(&socketIm) < 0) {
							socketManager_close(&socketIm);
							fsmState = FSM_IDLE;
						}
						else {
							softTimer_init(&timerAux, 10000);
							fsmState = FSM_WAIT_CONNECTION;
						}
					}
					else
						fsmState = FSM_IDLE;
				}
				else
					fsmState = FSM_IDLE;
			}
			else fsmState = FSM_IDLE;
			
			
			
			break;
			
			
		case FSM_IDLE:
		
			if (softTimer_expired(&timerCheckConnection)) {
				softTimer_init(&timerCheckConnection, 15000);
				
				fsmState = RESOLVE_DNS;
			}
			else if (softTimer_expired(&timerKeepalive)) {
				softTimer_restart(&timerKeepalive);
				
				if (imClient_isClientConnected() == 1) {
					msg = messagePool_getFreeSlot();
				
					if (msg != NULL) {
						toAux[0] = 0x00;
						toAux[1] = 0x00;
						toAux[2] = 0x00;
						toAux[3] = 0x00;
						imClient_send(msg, toAux, 1, IM_CLIENT_CMD_KEEPALIVE, 0, 0, 0);
					}
				}
			}
			else if (messagePool_pendingOutputMessage() == 1) {
				if (wifiManager_isWiFiConnected() == WIFI_MANAGER_WIFI_CONNECTED) {
					if (imClient_isClientConnected() == 1) {
						fsmState = FSM_MSG_SEND;
					}
				}
			}
			else if (resetConnection) {
				resetConnection = false;
				
				socketManager_close(&socketIm);
				fsmState = FSM_GET_SOCKET;
			}
			
			break;
			
			
		case FSM_MSG_SEND:
		
			if (wifiManager_isWiFiConnected() == WIFI_MANAGER_WIFI_CONNECTED) {
				if (socketManager_isConnected(&socketIm) == 1) {
					msg = messagePool_peekOutputQueue();
					generateFrameToSend(msg);
					
					softTimer_init(&timerResponseTimeout, 2000);
				}
			}
			
			fsmState = FSM_WAIT_ACK;
			
			break;
			
			
		case FSM_WAIT_ACK:
			
			if (softTimer_expired(&timerResponseTimeout)) {
				// No llegó la respuesta a tiempo
				if (inLoginProcess) {
					socketManager_close(&socketIm);
					inLoginProcess = false;
				}
				else {
					sendingError ++;
					if (sendingError >= 4) {
						socketManager_close(&socketIm);
					}
				}

				fsmState = FSM_IDLE;
			}
			else if (messagePool_pendingInputMessage(MESSAGE_POOL_FLOW_IM_CLIENT)) {
				// Llegó una respuesta
				msg = messagePool_peekInputQueue(MESSAGE_POOL_FLOW_IM_CLIENT);
				
				if (msg->cmd == IM_CLIENT_CMD_ACK) {
					messagePool_popOutputQueue();
					sendingError = 0;
					
					if (inLoginProcess) {
						softTimer_init(&timerResponseTimeout, 3000);
						messagePool_releaseSlot(msg);
						fsmState = FSM_WAIT_LOGIN_RESPONSE;
					}
					else {
						fsmState = FSM_IDLE;
					}
				}
				else {
					// No llegó el ACK.
					if (inLoginProcess) {
						socketManager_close(&socketIm);
						inLoginProcess = false;
						messagePool_releaseSlot(msg);
					}
					else {
						sendingError ++;
						if (sendingError >= 4) {
							socketManager_close(&socketIm);
						}
						
					}
					
					fsmState = FSM_IDLE;
				}
				
				messagePool_popInputQueue(MESSAGE_POOL_FLOW_IM_CLIENT);
			}
			
			
			break;
			
			
		case FSM_WAIT_CONNECTION:
		
			if (socketManager_isConnected(&socketIm) == 1) {
				uint8_t dataToEncrypt[8];
				dateTime_t now;
				
				
				msg = messagePool_getFreeSlot();
				
				if (msg != NULL) {
					needToSendLoginMessage = false;
					
					dateTime_now(&now);
					
					for (int i = 0; i < 4; i++) {
						msg->to[i] = 0;
					}
					
					msg->flow = MESSAGE_POOL_FLOW_IM_CLIENT;
					msg->cmd = IM_CLIENT_CMD_LOGIN;
					msg->reg = 0;
					msg->part = 0;
					msg->qos = 0;
					msg->timestamp.dia = now.dia;
					msg->timestamp.mes = now.mes;
					msg->timestamp.anio = now.anio;
					msg->timestamp.horas = now.horas;
					msg->timestamp.minutos = now.minutos;
					msg->timestamp.segundos = now.segundos;
					
					generateFrameToSend(msg);
					
					inLoginProcess = true;
					
					softTimer_init(&timerResponseTimeout, 2000);
					fsmState = FSM_WAIT_ACK;
				}
				else
					fsmState = FSM_IDLE;
			}
			else if (softTimer_expired(&timerAux)) {
				socketManager_close(&socketIm);
				fsmState = FSM_IDLE;
			}
			
			break;
			
			
		case FSM_WAIT_LOGIN_RESPONSE:
		
			if (softTimer_expired(&timerResponseTimeout)) {
				// No llegó la respuesta a tiempo
				inLoginProcess = false;
				authenticated = false;
				socketManager_close(&socketIm);

				fsmState = FSM_IDLE;
			}
			else if (messagePool_pendingInputMessage(MESSAGE_POOL_FLOW_IM_CLIENT)) {
				// Llegó una respuesta
				msg = messagePool_peekInputQueue(MESSAGE_POOL_FLOW_IM_CLIENT);
			
				if (msg->cmd == IM_CLIENT_CMD_RESP_LOGIN) {
					sendingError = 0;
					inLoginProcess = false;
					authenticated = (msg->payload[0] == 1)? true : false;

					fsmState = FSM_IDLE;
				}
				else {
					// No llegó la Respuesta al LOGIN.
					inLoginProcess = false;
					authenticated = false;
					socketManager_close(&socketIm);

					fsmState = FSM_IDLE;
				}
			
				messagePool_popInputQueue(MESSAGE_POOL_FLOW_IM_CLIENT);
			}
		
		
			break;
	}
}




static void socket_connect (SOCKET id, tstrSocketConnectMsg* msg)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	
	if (wifiManager_isProvisioningEnable() == 0) {

		if (msg && msg->s8Error >= 0) {
#ifdef DEBUG_PRINTF
			printf("imClient - socket: connect success!\r\n");
#endif
			socketManager_startReceiving(sock);
		}
		else {
#ifdef DEBUG_PRINTF
			printf("imClient - socket: connect error!\r\n");
#endif
			socketManager_close(sock);
		}
	}
}


static void socket_sent (SOCKET id)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	
#ifdef DEBUG_PRINTF
	printf("imClient - socket: send success!\r\n");
#endif
	
	socketManager_flushBufferOut (sock);
}


static void socket_received (SOCKET id, tstrSocketRecvMsg* msg)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	uint32_t sum = 0;
	
	
	if (msg && msg->s16BufferSize > 0) {
#ifdef DEBUG_PRINTF
		printf("imClient - socket: recv success!\r\n");
#endif
	
		// Se chequea si se terminó de recibir el mensaje
		if (msg->pu8Buffer[msg->s16BufferSize - 1] == 0x85) {
			
			// Se chequea el checksum
			for (int i = 0; i < (msg->s16BufferSize - 1); i++) {
				sum += msg->pu8Buffer[i];
			}
							
			if ((sum & 0x000000ff) == 0xff) {
				// Está bien el checksum
				parseMessageIn(msg->pu8Buffer);	
				
			}
			else {
				// Hay un error en el checksum
				
			}
			
			socketManager_flushBufferIn(sock);
		}
			
		socketManager_startReceiving(sock);
	}
}


static void socket_closed (SOCKET id)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);

	sendingError = 0;
	
#ifdef DEBUG_PRINTF
	printf("imClient - socket: socket closed!\r\n");
#endif
}



static void generateFrameToSend (imMessage_t* msg)
{
	uint16_t len = msg->payload_ptr;
	
	socketManager_bufferOutPutBytes(&socketIm, myId, 4);												// origen
	socketManager_bufferOutPutBytes(&socketIm, msg->to, 4);												// destino
	socketManager_bufferOutPutByte(&socketIm, msg->flow);												// flow
	socketManager_bufferOutPutByte(&socketIm, msg->cmd);												// comando
	socketManager_bufferOutPutBytes(&socketIm, (uint8_t*)&(msg->reg), 2);								// registro
	socketManager_bufferOutPutByte(&socketIm, msg->part);												// particion
	socketManager_bufferOutPutByte(&socketIm, msg->qos);												// QoS
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.dia);										// Fecha: dia
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.mes);										// Fecha: mes
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.anio);										// Fecha: anio
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.horas);									// Hora: horas
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.minutos);									// Hora: minutos
	socketManager_bufferOutPutByte(&socketIm, msg->timestamp.segundos);									// Hora: segundos
	socketManager_bufferOutPutBytes(&socketIm, (uint8_t*)&(len), 2);									// largo
	socketManager_bufferOutPutBytes(&socketIm, msg->payload, msg->payload_ptr);							// payload
	socketManager_bufferOutPutByte(&socketIm, socketManager_bufferOutGetChecksum(&socketIm));			// checksum
	socketManager_bufferOutPutByte(&socketIm, 0x85);
	
	//msg->ready = 0;
	//msg->payload_ptr = 0;
	
	
	socketManager_send(&socketIm);
}



static void parseMessageIn (uint8_t* data) 
{
	imMessage_t* msg;
	uint32_t index = 0;
	
	
	
	msg = messagePool_getFreeSlot();
	

	if (msg != NULL) {
		
		for (index = 0; index < 4; index++) {
			msg->from[index] = data[index];
		}
		
		for (; index < 8; index++) {
			msg->to[index - 4] = data[index];
		}
		
		msg->flow = data[index];
		index ++;
		
		msg->cmd = data[index];
		index ++;
		
		((uint8_t*)&(msg->reg))[0] = data[index];
		index ++;
		((uint8_t*)&(msg->reg))[1] = data[index];
		index ++;
		
		msg->part = data[index];
		index++;
		
		msg->qos = data[index];
		index ++;
		
		msg->timestamp.dia = data[index];
		index ++;
		
		msg->timestamp.mes = data[index];
		index ++;
		
		msg->timestamp.anio = data[index];
		index ++;
		
		msg->timestamp.horas = data[index];
		index ++;
		
		msg->timestamp.minutos = data[index];
		index ++;
		
		msg->timestamp.segundos = data[index];
		index ++;
		
		((uint8_t*)&(msg->len))[0] = data[index];
		index ++;
		((uint8_t*)&(msg->len))[1] = data[index];
		index ++;
		
		for (int i = 0; i < MESSAGE_POOL_MESSAGE_MAX_LEN && i < msg->len; i++) {
			msg->payload[i] = data[i + 22];
			index ++;
		}
		
		
		messagePool_pushInputQueue(msg->flow, msg);
	}
}
