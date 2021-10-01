#ifndef BG96_H_
#define BG96_H_

#include <asf.h>

#define BG96_POWER_KEY		BG96_POWER_KEY_PIN
#define BG96_POWER			BG96_POWER_PIN
#define BG96_RESET			BG96_RESET_PIN
#define BG96_STATUS			BG96_STATUS_PIN
#define BG96_DTR			BG96_DTR_PIN
#define BG96_RTS			BG96_RTS_PIN


#define BG96_PDP_CONTEXTS		1
#define BG96_SSL_CONTEXTS		1
#define BG96_TCP_SOCKETS		1


/*************************************************************************************
Eventos generados por el módulo. Para procesarlos se debe pasar un callback
mediante la función bg96_registerModuleCallback
**************************************************************************************/
typedef enum {
	bg96_module_stateChange = 0,
	bg96_module_contextStateChange,
	bg96_module_error
}bg96_module_events;


typedef enum {
	module_states_change_registred = 0,
	module_states_change_unregistred
}bg96_module_states_change;

typedef struct {
	bg96_module_states_change state;
}bg96_moduleStateChangePayload;


typedef enum {
	module_states_context_opened = 0,
	module_states_context_closed
}bg96_module_context_change;

typedef struct {
	bg96_module_context_change state;
	uint8_t contextId;
	bool isSsl;
}bg96_contextStateChangePayload;


typedef enum {
	module_error_sim_with_pin_but_device_without_pin = 0,
	module_error_sim_without_pin_but_device_with_pin,
	module_error_sim_and_device_different_pin,
	module_error_sim_error,
	module_error_cant_open_pdp_context,
	module_error_cant_close_pdp_context,
	module_error_cant_open_ssl_context
}bg96_module_errors;

typedef struct {
	bg96_module_errors error;
	uint8_t id;
}bg96_moduleErrorPayload;



/*************************************************************************************
Eventos generados por los sockets. Para procesarlos se debe pasar un callback
mediante la función bg96_registerSocketCallback
**************************************************************************************/
typedef enum {
	bg96_socket_connected = 0,
	bg96_socket_closed,
	bg96_socket_dataSent,
	bg96_socket_dataReceived,
	bg96_socket_error
}bg96_socket_events;

typedef struct {
	bool result;
}bg96_socketConnectedPayload;


typedef struct {
	uint32_t length;
	uint8_t* data;
}bg96_socketDataReceivedPayload;

typedef enum {
	socket_error_cant_open = 0,
	socket_error_cant_close,
	socket_error_cant_send,
	socket_error_cant_read
}bg96_socket_errors;

typedef struct {
	bg96_socket_errors error;
}bg96_socketErrorPayload;


/*************************************************************************************
MQTT
**************************************************************************************/
typedef struct {
	bool ssl;
	uint8_t sslContextId;
	uint8_t pdpContextId;
	bool cleanSession;
	uint32_t keepaliveTime;
	uint8_t will_topic[20];
	uint8_t will_msg[20];
	uint8_t will_qos;
	bool will_retain;
	uint8_t brokerUrl[50];
	uint32_t brokerPort;
	uint8_t clientId[10];
	uint8_t user[10];
	uint8_t password[10];
}bg96_mqtt_configuration_t;

typedef enum {
	bg96_mqtt_connected = 0,
	bg96_mqtt_disconnected,
	bg96_mqtt_subscribed,
	bg96_mqtt_msgPublished,
	bg96_mqtt_msgReceived,
	bg96_mqtt_error
}bg96_mqtt_events;


typedef enum {
	mqtt_error_cant_connect = 0,
	mqtt_error_cant_disconnect,
	mqtt_error_connection_error
}bg96_mqtt_errors;

typedef struct {
	bg96_mqtt_errors error;
}bg96_mqttErrorPayload;

typedef struct {
	uint8_t* topic;
	uint8_t* data;
	uint32_t length;
}bg96_mqttMessageReceivedPayload;


/*************************************************************************************
Funciones generales
**************************************************************************************/
void bg96_init (struct usart_module* uart, bool catM, bool bands4g, uint8_t* simPin);
void bg96_initModule (void);
void bg96_resetModule (void);
void bg96_setEnableCatM (bool catm);
void bg96_setAll4gBands (bool bands);
int8_t bg96_getPdpContext (uint8_t* customApn, uint8_t* customApnUser, uint8_t* customApnPassword);
int32_t bg96_openPdpContext (uint8_t contextId);
int32_t bg96_closePdpContext (uint8_t contextId);
int8_t bg96_getSslContext (uint8_t* caCertName);
int32_t bg96_openSslContext (uint8_t contextId);
void bg96_handler (void);
bool bg96_isBusy (void);
bool bg96_isRegistred (void);
bool bg96_isPdpContextOpened (uint8_t contextId);
bool bg96_isSslContextOpened (uint8_t contextId);
void bg96_registerModuleCallback (void (*cb)(bg96_module_events, void*));

// Se la debe llamar cada 4ms
void bg96_statusSignal (void);


/*************************************************************************************
Sockets
**************************************************************************************/
int32_t bg96_getSocket (bool isSecure, uint32_t pdpConectextNumber, uint32_t sslConectextNumber, uint8_t* address, uint32_t port);
void bg96_registerSocketCallback (void (*cb)(uint32_t, bg96_socket_events, void*));
int32_t bg96_openSocket (uint32_t id);
int32_t bg96_closeSocket (uint32_t id);
bool bg96_isSocketConnected (uint32_t id);
void bg96_socketSendData (uint32_t id, uint8_t* data, uint32_t len);


/*************************************************************************************
MQTT
**************************************************************************************/
void bg96_mqtt_loadConfigurationDefault (bg96_mqtt_configuration_t* config);
void bg96_mqtt_init (bg96_mqtt_configuration_t* config);
void bg96_mqtt_registerMqttCallback (void (*cb)(bg96_mqtt_events, void*));
void bg96_mqtt_connect (void);
void bg96_mqtt_disconnect (void);
bool bg96_mqtt_isConnected (void);
void bg96_mqtt_subscribe (uint8_t* topic, uint8_t qos);
void bg96_mqtt_unsubscribe (uint8_t* topic);
void bg96_mqtt_publish (uint8_t* topic, uint8_t* data, uint32_t len, uint8_t qos, bool retain);
int32_t bg96_mqtt_bufferPutByte (uint8_t b);
int32_t bg96_mqtt_bufferPutBytes (uint8_t* bytes, uint32_t len);
uint8_t bg96_mqtt_bufferGetChecksum (void);
void bg96_mqtt_bufferConvertToString (void);
uint8_t* bg96_mqtt_getBuffer (void);
uint32_t bg96_mqtt_getBufferLen (void);
void bg96_mqtt_bufferFlush (void);


#endif /* BG96_H_ */