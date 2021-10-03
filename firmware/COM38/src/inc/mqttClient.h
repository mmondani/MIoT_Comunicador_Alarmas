#ifndef MQTTCLIENT_H_
#define MQTTCLIENT_H_

#include "MQTTClient/Wrapper/mqtt.h"

typedef struct {
	struct mqtt_module mqtt_inst;
	
	uint8_t* server;
	uint16_t port;
	bool isSecure;
	bool cleanSession;
	bool connected;
	uint16_t keepalive;
	
	uint8_t* clientId;
	uint8_t* user;
	uint8_t* password;
	
	uint8_t* lwTopic;
	uint8_t* lwMessage;
	uint8_t lwMessageLen;
	uint8_t lwQos;
	uint8_t lwRetain;
	
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint32_t bufferIn_len;
	uint32_t bufferOut_len;
	
	uint8_t bufferAux[100];
	uint8_t bufferAux2[200];
	uint32_t bufferAux_len;
	uint32_t bufferAux_ptr;
	
	void (*callback_connect) (bool result);
	void (*callback_disconnect) (void);
	void (*callback_connection_error) (void);
	void (*callback_pingreq_error) (void);
	void (*callback_subscribe) (void);
	void (*callback_unsubscribe) (void);
	void (*callback_publish) (void);
	void (*callback_receive) (MQTTString* topic, MQTTMessage* msg);
}mqttClient_t;


typedef struct {
	uint8_t* server;
	uint16_t port;
	bool isSecure;
	bool cleanSession;
	uint16_t keepalive;
	
	uint8_t* clientId;
	uint8_t* user;
	uint8_t* password;
	
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint32_t bufferIn_len;
	uint32_t bufferOut_len;
	
	void (*callback_connect) (bool result);
	void (*callback_disconnect) (void);
	void (*callback_connection_error) (void);
	void (*callback_pingreq_error) (void);
	void (*callback_subscribe) (void);
	void (*callback_unsubscribe) (void);
	void (*callback_publish) (void);
	void (*callback_receive) (MQTTString* topic, MQTTMessage* msg);
}mqttClient_config_t;


void mqttClient_loadDefaults (mqttClient_config_t* config);
int32_t mqttClient_init (mqttClient_config_t* config);
void mqttClient_connect (char *will_topic, char *will_msg, uint32_t will_msg_len, uint8_t will_qos, uint8_t will_retain);
void mqttClient_disconnect (void);
int mqttClient_getSocketId (void);
void mqttClient_subscribe (char *topic, uint8_t qos);
void mqttClient_unsubscribe (char *topic);
void mqttClient_publish (const char *topic, const char *msg, uint32_t msg_len, uint8_t qos, uint8_t retain);

bool mqttClient_isConnected (void);
int mqttClient_yield (void);
void mqttClient_clearSubscriptionHandlers (void);
int32_t mqttClient_bufferPutByte (uint8_t b);
int32_t mqttClient_bufferPutBytes (uint8_t* bytes, uint32_t len);
uint8_t mqttClient_bufferGetChecksum (void);
void mqttClient_bufferConvertToString (void);
uint8_t* mqttClient_getBuffer (void);
uint32_t mqttClient_getBufferLen (void);
void mqttClient_bufferFlush (void);

#endif /* MQTTCLIENT_H_ */