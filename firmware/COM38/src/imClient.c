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
#include "inc/connectivityManager.h"
#include "inc/BG96.h"


enum{
	FSM_WAITING_INTERFACE = 0,
	FSM_INIT,
	FSM_GET_SOCKET,
	FSM_IDLE,
	FSM_MSG_SEND,
	RESOLVE_DNS,
	WAIT_DNS_RESPONSE,
	FSM_CONNECT_BROKER,
	FSM_WAIT_ACK,
	FSM_WAIT_CONNECTION,
	FSM_WAIT_STATUS_OFFLINE_ACK,
	FSM_WAIT_CLOSE,
	FSM_WAIT_SUBSCRIBE,
	FSM_WAIT_STATUS_ONLINE_ACK,
	FSM_ERROR
	};
uint32_t fsmState;	


static struct mqtt_module mqtt_inst;
static uint8_t clientId[10];
static uint8_t password[20];
static bool published;
static bool subscribed;
static uint8_t subscribeTopic[15];
static uint8_t lastWillTopic[15];
static uint8_t lastWillMessage[15];
static int wifiMqttClientErrors;

static SoftTimer_t timerAux;
static SoftTimer_t timerCheckConnection;
static SoftTimer_t timerResponseTimeout;
static SoftTimer_t timerBuffer;
static uint8_t sendingError;
static bool needToSendLoginMessage;
static bool authenticated;
static bool inLoginProcess;
static bool resetConnection;
static uint32_t connectToBrokerTries;


static uint8_t buffIn[300];
static uint8_t buffOut[300];
static uint8_t toAux[4];
static uint8_t auxBuffer[30];
static uint8_t auxBufferRx[50];

static connectivityManager_events connectivityManager_event = connectivityManager_event_none;
static connectivityManager_interfaces connectivityManager_interface = connectivityManager_interface_none;
static connectivityManager_interfaces connectivityManager_nextInterface = connectivityManager_interface_none;


static bool interface_isConnected (void);
static int client_init (void);
static bool client_isConnected (void);
static void client_connect (void);
static void client_subscribe (void);
static void client_publish (char* topic, char* msg, uint32_t len, uint8_t qos, uint8_t retain);
static void client_disconnect (void);

static void callback_connect (bool result);
static void callback_disconnect (void);
static void callback_connection_error (void);
static void callback_pingreq_error (void);
static void callback_subscribe (void);
static void callback_unsubscribe (void);
static void callback_publish (void);
static void callback_receive (MQTTString* topic, MQTTMessage* msg);
static void generateFrameToSend (imMessage_t* msg);
static void parseMessageIn (uint8_t* data);
static void connectivity_onChangeCallback (connectivityManager_events evt, connectivityManager_interfaces ifc);
static void bg96MqttCallback (bg96_mqtt_events evt, void* payload);


void imClient_init (uint8_t* id, uint8_t* pass)
{
	fsmState = FSM_WAITING_INTERFACE;
	
	messagePool_init();
	
	softTimer_init(&timerCheckConnection, 1000);
	
	for (int i = 0; i < 4; i++) {
		clientId[2*i] = traducirHexaACaracter((id[i] & 0xF0) >> 4);
		clientId[2*i+1] = traducirHexaACaracter(id[i] & 0x0F);	
	}
	clientId[8] = '\0';
	
	for (int i = 0; i < 16; i++)
		password[i] = pass[i];
		
	// Last will
	sprintf(lastWillTopic, "%s/status", clientId);
	sprintf(lastWillMessage, "offline");
	
	// Suscripciones
	sprintf(subscribeTopic, "%s/cmd", clientId);
		
	
	needToSendLoginMessage = false;
	authenticated = false;
	inLoginProcess = false;
	resetConnection = false;
	
	wifiMqttClientErrors = 0;
	
	// El systick es usado por el cliente de MQTT del WINC1500
	if (SysTick_Config(system_cpu_clock_get_hz() / 1000))
	{
		errores1.bits.errorMqtt = 1;
	}
	
	connectivityManager_setChangeCallback(connectivity_onChangeCallback);
}


uint32_t imClient_isClientConnected (void)
{
	if (client_isConnected())
		return 1;
		
	return 0;
}


void imClient_resetConnection (void)
{
	resetConnection = true;
}


connectivityManager_interfaces imClient_currentInterface (void) 
{
	return connectivityManager_interface;
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
	int rc;
	
	
	if(mqttClient_isConnected() && connectivityManager_interface == connectivityManager_interface_wifi)
		rc = mqttClient_yield();
	
	if (softTimer_expired(&timerBuffer)) {
		if (messagePool_pendingOutputMessage() > 0) {
			// Si hay mensajes que no se pudieron enviar, se los borra
			messagePool_flushOutputQueue();
			
			//TO-DO: hay que señalizar de alguna forma esto??
		}
	}
	
	
	switch (fsmState) {
		
		case FSM_WAITING_INTERFACE:
			connectivityManager_interface = connectivityManager_nextInterface;
			break;
			
			
		case FSM_INIT:
		{
			connectivityManager_interface = connectivityManager_nextInterface;
			
			int result = 0;
			result = client_init ();
			
			connectToBrokerTries = 0;
			wifiMqttClientErrors = 0;
			
			if (errores1.bits.errorMqtt == 1 || result < 0) {
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
		
			if (interface_isConnected()) {
				if (!client_isConnected()) {
					client_connect();
					
					softTimer_init(&timerAux, 20000);
					fsmState = FSM_WAIT_CONNECTION;
				}
				else
					fsmState = FSM_IDLE;
			}
			else 
				fsmState = FSM_IDLE;
			
			break;
			
		case FSM_WAIT_CONNECTION:
				
			if (client_isConnected()) {
				// Se pudo conectar al broker, se va a suscribir al tópico por el que recibe los comandos
				
				subscribed = false;
				client_subscribe();
				
				
				fsmState = FSM_WAIT_SUBSCRIBE;
			}
			else if (softTimer_expired(&timerAux)) {
				connectToBrokerTries ++;
				
				if (connectToBrokerTries < 2)
					fsmState = FSM_CONNECT_BROKER;
				else {
					// Se reporta sal connectivity manager que hay un problema con la interfaz
					// actual de comunicación
					connectivityManager_interfaceError(connectivityManager_interface);	
					fsmState = FSM_IDLE;				
				}
				
			}
				
			break;
			
			
		case FSM_IDLE:
		
			if (resetConnection) {
				resetConnection = false;
				
				published = false;
				client_publish(
					lastWillTopic,
					"offline",
					7,
					1,
					1);
				
				softTimer_init(&timerResponseTimeout, 2000);
				fsmState = FSM_WAIT_STATUS_OFFLINE_ACK;
			}
			else if (softTimer_expired(&timerCheckConnection)) {
				softTimer_init(&timerCheckConnection, 15000);
				
				fsmState = FSM_CONNECT_BROKER;
			}
			else if (messagePool_pendingOutputMessage() == 1) {
				if (interface_isConnected()) {
					if (imClient_isClientConnected() == 1) {
						fsmState = FSM_MSG_SEND;
					}
				}
			}
			
			break;
			
			
		case FSM_MSG_SEND:
		
			if (interface_isConnected()) {
				if (client_isConnected()) {
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
				// TODO hacer algo si falla 2 veces en enviarlo. Informar a connectivityManager?
				sendingError ++;
				if (sendingError >= 2) {
					connectivityManager_interfaceError(connectivityManager_interface);	
				}

				fsmState = FSM_IDLE;
			}
			else if (published) {
				// Llegó un PUBACK		
				messagePool_popOutputQueue();
				
				sendingError = 0;
				
				fsmState = FSM_IDLE;
			}
			
			
			break;
			
			
		case FSM_WAIT_STATUS_OFFLINE_ACK:
			if (softTimer_expired(&timerResponseTimeout)) {
				// No llegó el PUBACK a tiempo
				sendingError ++;
				if (sendingError >= 2) {
					connectivityManager_interfaceError(connectivityManager_interface);	
				}

				fsmState = FSM_CONNECT_BROKER;
			}
			else if (published) {
				// Llegó un PUBACK
				client_disconnect();
				
				fsmState = FSM_WAIT_CLOSE;
			}
			
			break;	
			
		
		case FSM_WAIT_CLOSE:
		
			if (!client_isConnected()) {
				connectToBrokerTries = 0;
				fsmState = FSM_INIT;
			}
			
			break;
			
			
		case FSM_WAIT_SUBSCRIBE:
		
			if (subscribed) {
				published = false;
				
				client_publish(
					lastWillTopic,
					"online",
					6,
					1,
					1);
					
				softTimer_init(&timerResponseTimeout, 2000);
				fsmState = FSM_WAIT_STATUS_ONLINE_ACK;
			}
			
			break;
			
			
		case FSM_WAIT_STATUS_ONLINE_ACK:
		
			if (softTimer_expired(&timerResponseTimeout)) {
				// No llegó el PUBACK a tiempo
				sendingError ++;
				if (sendingError >= 2) {
					connectivityManager_interfaceError(connectivityManager_interface);	
				}

				fsmState = FSM_IDLE;
			}
			else if (published) {
				// Llegó un PUBACK
				fsmState = FSM_IDLE;
			}
			
			break;
			
			
		case FSM_ERROR:
		
			break;
	}
}


static void generateFrameToSend (imMessage_t* msg)
{
	uint16_t len = msg->payload_ptr;
	
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_bufferFlush();

		mqttClient_bufferPutByte(msg->cmd);													// comando
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

		mqttClient_bufferConvertToString();

		// TODO definir el QoS
		if (msg->cmd == IM_CLIENT_CMD_RESP_GET)
			sprintf(auxBuffer, "%s/resp", clientId);
		else
			sprintf(auxBuffer, "%s/event", clientId);


		client_publish (
			auxBuffer,
			mqttClient_getBuffer(),
			mqttClient_getBufferLen(),
			1,
			0);
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_bufferFlush();

		bg96_mqtt_bufferPutByte(msg->cmd);												// comando
		bg96_mqtt_bufferPutBytes((uint8_t*)&(msg->reg), 2);								// registro
		bg96_mqtt_bufferPutByte(msg->part);												// particion
		bg96_mqtt_bufferPutByte(msg->timestamp.dia);									// Fecha: dia
		bg96_mqtt_bufferPutByte(msg->timestamp.mes);									// Fecha: mes
		bg96_mqtt_bufferPutByte(msg->timestamp.anio);									// Fecha: anio
		bg96_mqtt_bufferPutByte(msg->timestamp.horas);									// Hora: horas
		bg96_mqtt_bufferPutByte(msg->timestamp.minutos);								// Hora: minutos
		bg96_mqtt_bufferPutByte(msg->timestamp.segundos);								// Hora: segundos
		bg96_mqtt_bufferPutBytes((uint8_t*)&(len), 2);									// largo
		bg96_mqtt_bufferPutBytes(msg->payload, msg->payload_ptr);						// payload
		bg96_mqtt_bufferPutByte(bg96_mqtt_bufferGetChecksum());							// checksum
		
		bg96_mqtt_bufferConvertToString();

		// TODO definir el QoS
		if (msg->cmd == IM_CLIENT_CMD_RESP_GET)
			sprintf(auxBuffer, "%s/resp", clientId);
		else
			sprintf(auxBuffer, "%s/event", clientId);


		client_publish (
			auxBuffer,
			bg96_mqtt_getBuffer(),
			bg96_mqtt_getBufferLen(),
			1,
			0);
	}
	
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


void callback_connection_error (void) {
	// Se intenta volver a conectar
	fsmState = FSM_CONNECT_BROKER;
}


void callback_pingreq_error (void) {
	connectivityManager_interfaceError(connectivityManager_interface);	
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
	
	for (int i = 0, j = 0; i < msg->payloadlen; j ++) {
		auxBufferRx[j] = convertHexStringToNumber(&(msg->payload[i]));
		i += 2;
	}
	
	// Se chequea el checksum
	for (int i = 0; i < msg->payloadlen / 2; i++) {
		sum += auxBufferRx[i];
	}
	
	if ((sum & 0x000000ff) == 0xff) {
		// Está bien el checksum
		parseMessageIn(auxBufferRx);
	}
	else {
		// Hay un error en el checksum
	}
}


void connectivity_onChangeCallback (connectivityManager_events evt, connectivityManager_interfaces ifc) {
	switch (evt) {
		case connectivityManager_event_change:
			connectivityManager_nextInterface = ifc;
			if (ifc == connectivityManager_interface_none) 
				fsmState = FSM_WAITING_INTERFACE;
			else			
				fsmState = FSM_INIT;
			break;
			
		case connectivityManager_event_disconnect_and_change:
			connectivityManager_nextInterface = ifc;
			resetConnection = true;
			break;
	}
}



bool interface_isConnected (void) {
	bool result = false;
	
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		if (wifiManager_isWiFiConnected() == M2M_WIFI_CONNECTED)
		result = true;
		else
		result = false;
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		result = bg96_isPdpContextOpened(cellularFsm_getPdpContext());
	}
	
	return result;
}



int client_init (void) {
	int result = 0;
	
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_config_t mqtt_conf;
		

		mqttClient_loadDefaults(&mqtt_conf);
		mqtt_conf.server = (uint8_t*)&pgaData[PGA_BROKER_URL];
		mqtt_conf.port = (pgaData[PGA_BROKER_PORT + 1] << 8) | pgaData[PGA_BROKER_PORT];
		mqtt_conf.isSecure = true;
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
		mqtt_conf.callback_connection_error = callback_connection_error;
		mqtt_conf.callback_pingreq_error = callback_pingreq_error;
		mqtt_conf.callback_subscribe = callback_subscribe;
		mqtt_conf.callback_unsubscribe = callback_unsubscribe;
		mqtt_conf.callback_publish = callback_publish;
		mqtt_conf.callback_receive = callback_receive;
		
		result = mqttClient_init(&mqtt_conf);
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_configuration_t mqttConfig;
		uint32_t i;
		
		bg96_mqtt_loadConfigurationDefault(&mqttConfig);
		
		for (i = 0; i < pgaData[PGA_BROKER_URL + i] != '\0' && i < 50; i++)
			mqttConfig.brokerUrl[i] = pgaData[PGA_BROKER_URL + i];
		mqttConfig.brokerUrl[i] = '\0';
		
		mqttConfig.brokerPort = (pgaData[PGA_BROKER_PORT + 1] << 8) | pgaData[PGA_BROKER_PORT];
		
		mqttConfig.pdpContextId = cellularFsm_getPdpContext();
		mqttConfig.ssl = true;
		mqttConfig.sslContextId = cellularFsm_getSslContext();
		
		mqttConfig.cleanSession = true;
		mqttConfig.keepaliveTime = 30;
		
		for (i = 0; i < clientId[i] != '\0' && i < 10; i++)
			mqttConfig.clientId[i] = clientId[i];
		mqttConfig.clientId[i] = '\0';
		
		for (i = 0; i < clientId[i] != '\0' && i < 10; i++)
			mqttConfig.user[i] = clientId[i];
		mqttConfig.user[i] = '\0';
		
		for (i = 0; i < password[i] != '\0' && i < 10; i++)
			mqttConfig.password[i] = password[i];
		mqttConfig.password[i] = '\0';
		
		for (i = 0; i < lastWillTopic[i] != '\0' && i < 20; i++)
			mqttConfig.will_topic[i] = lastWillTopic[i];
		mqttConfig.will_topic[i] = '\0';
		
		for (i = 0; i < lastWillMessage[i] != '\0' && i < 20; i++)
			mqttConfig.will_msg[i] = lastWillMessage[i];
		mqttConfig.will_msg[i] = '\0';
		
		bg96_mqtt_init(&mqttConfig);
		
		bg96_mqtt_registerMqttCallback(bg96MqttCallback);
		
		result = 0;
	}
}


bool client_isConnected (void) {
	bool result = false;
	
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		result = mqttClient_isConnected();
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		result = bg96_mqtt_isConnected();
	}
	
	return result;
	
}


void client_connect (void) {
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_clearSubscriptionHandlers();
		mqttClient_connect(lastWillTopic, lastWillMessage, 7, 0, 1);
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_connect();
	}
}


void client_subscribe (void) {
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_subscribe(subscribeTopic, 0);
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_subscribe("5843A135/cmd", 2);
	}
}


void client_publish (char* topic, char* msg, uint32_t len, uint8_t qos, uint8_t retain) {
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_publish(
			topic,
			msg,
			len,
			qos,
			retain);
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_publish(
			topic,
			msg,
			len,
			qos,
			retain);
	}

}


void client_disconnect (void) {
	if (connectivityManager_interface == connectivityManager_interface_wifi) {
		mqttClient_disconnect();
	}
	else if (connectivityManager_interface == connectivityManager_interface_cellular) {
		bg96_mqtt_disconnect();
	}
}



void bg96MqttCallback (bg96_mqtt_events evt, void* payload) {
	switch(evt) {
		case bg96_mqtt_connected:
		
			break;
		
		case bg96_mqtt_disconnected:
		
			break;
		
		case bg96_mqtt_subscribed:
			subscribed = true;
			break;
		
		case bg96_mqtt_msgPublished:
			published = true;
			break;
		
		case bg96_mqtt_msgReceived:
		{
			bg96_mqttMessageReceivedPayload* mqttMsgReceived = (bg96_mqttMessageReceivedPayload*)payload;
			
			uint32_t sum = 0;
			uint8_t* payload = mqttMsgReceived->data;
				
			for (int i = 0, j = 0; i < mqttMsgReceived->length; j ++) {
				auxBufferRx[j] = convertHexStringToNumber(&(payload[i]));
				i += 2;
			}	
				
			// Se chequea el checksum
			for (int i = 0; i < mqttMsgReceived->length / 2; i++) {
				sum += auxBufferRx[i];
			}
				
			if ((sum & 0x000000ff) == 0xff) {
				// Está bien el checksum
				parseMessageIn(auxBufferRx);
			}
			else {
				// Hay un error en el checksum
			}
			
			break;
		}
		
		
		
		case bg96_mqtt_error:
		{
			bg96_mqttErrorPayload* mqttError = (bg96_mqttErrorPayload*)payload;
			
			switch(mqttError->error) {
				case mqtt_error_cant_connect:
					//bg96_resetModule();
					//fsmState = FSM_INIT;
					break;
					
				case mqtt_error_connection_error:
					bg96_resetModule();
					fsmState = FSM_INIT;
					break;
			}
			
			break;
		}
	}
}