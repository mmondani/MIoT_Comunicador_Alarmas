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
#include "inc/mqttClient.h"
#include "inc/utilities.h"


enum{
	FSM_INIT = 0,
	FSM_GET_SOCKET,
	FSM_IDLE,
	FSM_MSG_SEND,
	RESOLVE_DNS,
	WAIT_DNS_RESPONSE,
	FSM_CONNECT_BROKER,
	FSM_WAIT_ACK,
	FSM_WAIT_CONNECTION,
	FSM_WAIT_CLOSE,
	FSM_WAIT_SUBSCRIBE,
	FSM_ERROR
	};
uint32_t fsmState;	


static struct mqtt_module mqtt_inst;
static uint8_t clientId[10];
static uint8_t password[20];
static bool published;
static bool subscribed;
static uint8_t subscribeTopic[15];

static SoftTimer_t timerAux;
static SoftTimer_t timerCheckConnection;
static SoftTimer_t timerResponseTimeout;
static SoftTimer_t timerBuffer;
static uint8_t sendingError;
static bool needToSendLoginMessage;
static bool authenticated;
static bool inLoginProcess;
static bool resetConnection;


static uint8_t buffIn[300];
static uint8_t buffOut[300];
static uint8_t toAux[4];
static uint8_t auxBuffer[30];


static void callback_connect (bool result);
static void callback_disconnect (void);
static void callback_subscribe (void);
static void callback_unsubscribe (void);
static void callback_publish (void);
static void callback_receive (MQTTString* topic, MQTTMessage* msg);
static void generateFrameToSend (imMessage_t* msg);
static void parseMessageIn (uint8_t* data);


void imClient_init (uint8_t* id, uint8_t* pass)
{
	fsmState = FSM_INIT;
	
	messagePool_init();
	
	softTimer_init(&timerCheckConnection, 1000);
	
	for (int i = 0; i < 4; i++) {
		clientId[2*i] = traducirHexaACaracter((id[i] & 0xF0) >> 4);
		clientId[2*i+1] = traducirHexaACaracter(id[i] & 0x0F);	
	}
	clientId[8] = '\0';
	
	for (int i = 0; i < 16; i++)
		password[i] = pass[i];
		
	
	needToSendLoginMessage = false;
	authenticated = false;
	inLoginProcess = false;
	resetConnection = false;
	
	// El systick es usado por el cliente de MQTT del WINC1500
	if (SysTick_Config(system_cpu_clock_get_hz() / 1000))
	{
		errores1.bits.errorMqtt = 1;
	}
}


uint32_t imClient_isClientConnected (void)
{
	if (mqttClient_isConnected()) {
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
	
	msg->cmd = cmd;
	msg->reg = reg;
	msg->part = part;
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
	
	if(mqttClient_isConnected())
		mqttClient_yield();
	
	if (softTimer_expired(&timerBuffer)) {
		if (messagePool_pendingOutputMessage() > 0) {
			// Si hay mensajes que no se pudieron enviar, se los borra
			messagePool_flushOutputQueue();
			
			//TO-DO: hay que señalizar de alguna forma esto??
		}
	}
	
	
	switch (fsmState) {
		
		case FSM_INIT:
		{
			mqttClient_config_t mqtt_conf;
			int result;

			mqttClient_loadDefaults(&mqtt_conf);
			mqtt_conf.server = (uint8_t*)&pgaData[PGA_BROKER_URL];
			mqtt_conf.port = (pgaData[PGA_BROKER_PORT + 1] << 8) | pgaData[PGA_BROKER_PORT];
			mqtt_conf.isSecure = false;
			mqtt_conf.cleanSession = true;
			mqtt_conf.keepalive = 30;
			
			mqtt_conf.clientId = &clientId;
			mqtt_conf.user = &clientId;
			mqtt_conf.password = &password;
			
			mqtt_conf.bufferIn = buffIn;
			mqtt_conf.bufferOut = buffOut;
			mqtt_conf.bufferIn_len = 300;
			mqtt_conf.bufferOut_len = 300;
			
			mqtt_conf.callback_connect = callback_connect;
			mqtt_conf.callback_disconnect = callback_disconnect;
			mqtt_conf.callback_subscribe = callback_subscribe;
			mqtt_conf.callback_unsubscribe = callback_unsubscribe;
			mqtt_conf.callback_publish = callback_publish;
			mqtt_conf.callback_receive = callback_receive;
			
			if (errores1.bits.errorMqtt == 1 || mqttClient_init(&mqtt_conf) < 0) {
				errores1.bits.errorMqtt = 1;
				fsmState = FSM_ERROR;
			}
			else {
				sendingError = 0;
				
				fsmState = FSM_CONNECT_BROKER;
			}
			
			break;
		}
		
		case FSM_CONNECT_BROKER:
		
			if (wifiManager_isWiFiConnected()) {
				if (!mqttClient_isConnected()) {
					if (wifiManager_isProvisioningEnable() == 0) {
						mqttClient_clearSubscriptionHandlers();
						
						// TODO configurar un LWT
						mqttClient_connect(NULL, NULL, 0, 0, 0);
						
						softTimer_init(&timerAux, 20000);
						fsmState = FSM_WAIT_CONNECTION;
					}
					else
						fsmState = FSM_IDLE;
				}
				else
					fsmState = FSM_IDLE;
			}
			else 
				fsmState = FSM_IDLE;
			
			break;
			
		case FSM_WAIT_CONNECTION:
				
			if (mqttClient_isConnected()) {
				// Se pudo conectar al broker, se va a suscribir al tópico por el que recibe los comandos
				sprintf(subscribeTopic, "%s/cmd", clientId);
				
				
				
				subscribed = false;
				mqttClient_subscribe(subscribeTopic, 0);
				
				fsmState = FSM_WAIT_SUBSCRIBE;
			}
			else if (softTimer_expired(&timerAux))
				fsmState = FSM_IDLE;
				
			break;
			
			
		case FSM_IDLE:
		
			if (softTimer_expired(&timerCheckConnection)) {
				softTimer_init(&timerCheckConnection, 15000);
				
				fsmState = FSM_CONNECT_BROKER;
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
				
				mqttClient_disconnect();
				
				fsmState = FSM_WAIT_CLOSE;
			}
			
			break;
			
			
		case FSM_MSG_SEND:
		
			if (wifiManager_isWiFiConnected() == WIFI_MANAGER_WIFI_CONNECTED) {
				if (mqttClient_isConnected()) {
					msg = messagePool_peekOutputQueue();
					
					published = false;
					generateFrameToSend(msg);

					softTimer_init(&timerResponseTimeout, 2000);
				}
			}
			
			fsmState = FSM_WAIT_ACK;
			
			break;
			
			
		case FSM_WAIT_ACK:
			
			if (softTimer_expired(&timerResponseTimeout)) {
				// No llegó el PUBACK a tiempo
				// TODO hacer algo si falla 4 veces en enviarlo
				sendingError ++;
				if (sendingError >= 4) {
					//socketManager_close(&socketIm);
				}

				fsmState = FSM_IDLE;
			}
			else if (published) {
				// Llegó un PUBACK		
				messagePool_popOutputQueue();
				
				fsmState = FSM_IDLE;
			}
			
			
			break;
			
		
		case FSM_WAIT_CLOSE:
		
			if (!mqttClient_isConnected())
				fsmState = FSM_CONNECT_BROKER;
			
			break;
			
			
		case FSM_WAIT_SUBSCRIBE:
		
			if (subscribed)
				fsmState = FSM_IDLE;
			
			break;
			
			
		case FSM_ERROR:
		
			break;
	}
}


static void generateFrameToSend (imMessage_t* msg)
{
	uint16_t len = msg->payload_ptr;
	
	mqttClient_bufferFlush();
	
	mqttClient_bufferPutByte(msg->cmd);												// comando
	mqttClient_bufferPutBytes((uint8_t*)&(msg->reg), 2);								// registro
	mqttClient_bufferPutByte(msg->part);												// particion
	mqttClient_bufferPutByte(msg->timestamp.dia);										// Fecha: dia
	mqttClient_bufferPutByte(msg->timestamp.mes);										// Fecha: mes
	mqttClient_bufferPutByte(msg->timestamp.anio);										// Fecha: anio
	mqttClient_bufferPutByte(msg->timestamp.horas);										// Hora: horas
	mqttClient_bufferPutByte(msg->timestamp.minutos);									// Hora: minutos
	mqttClient_bufferPutByte(msg->timestamp.segundos);									// Hora: segundos
	mqttClient_bufferPutBytes((uint8_t*)&(len), 2);										// largo
	mqttClient_bufferPutBytes(msg->payload, msg->payload_ptr);							// payload
	mqttClient_bufferPutByte(mqttClient_bufferGetChecksum());							// checksum

	// TODO definir el QoS
	if (msg->cmd == IM_CLIENT_CMD_RESP_GET)
		sprintf(auxBuffer, "%s/resp", clientId); 
	else
		sprintf(auxBuffer, "%s/event", clientId);
	
	mqttClient_publish(
		auxBuffer,
		mqttClient_getBuffer(),
		mqttClient_getBufferLen(),
		0,
		0);
}



static void parseMessageIn (uint8_t* data) 
{
	imMessage_t* msg;
	uint32_t index = 0;
	
	
	
	msg = messagePool_getFreeSlot();
	

	if (msg != NULL) {
		
		msg->cmd = data[index];
		index ++;
		
		((uint8_t*)&(msg->reg))[0] = data[index];
		index ++;
		((uint8_t*)&(msg->reg))[1] = data[index];
		index ++;
		
		msg->part = data[index];
		index++;
		
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
			msg->payload[i] = data[i + 12];
			index ++;
		}
		
		
		messagePool_pushInputQueue(0, msg);
	}
}


void callback_connect (bool result) {
	
}


void callback_disconnect (void) {
	
}


void callback_subscribe (void) {
	subscribed = true;
}


void callback_unsubscribe (void) {
	
}


void callback_publish (void) {
	published = true;
}


void callback_receive (MQTTString* topic, MQTTMessage* msg) {
	uint32_t sum = 0;
	uint8_t* payload = (uint8_t*)msg->payload;
	
	// Se chequea el checksum
	for (int i = 0; i < msg->payloadlen; i++) {
		sum += payload[i];
	}
	
	if ((sum & 0x000000ff) == 0xff) {
		// Está bien el checksum
		parseMessageIn(msg->payload);
	}
	else {
		// Hay un error en el checksum
	}
}