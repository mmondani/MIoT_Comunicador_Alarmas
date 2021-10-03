#include "inc/mqttClient.h"
#include "inc/utilities.h"


static mqttClient_t mqttClient;

static void mqtt_callback(struct mqtt_module *module_inst, int type, union mqtt_data *data);
static void msgHandler (MessageData* msg);



void mqttClient_loadDefaults (mqttClient_config_t* config) {
	config->server = NULL;
	config->port = 1883;
	config->isSecure = false;
	config->cleanSession = true;
	config->keepalive = 120;
	
	config->clientId = NULL;
	config->user = NULL;
	config->password = NULL;
	
	config->bufferIn = NULL;
	config->bufferOut = NULL;
	config->bufferIn_len = 0;
	config->bufferOut_len = 0;
	
	config->callback_connect = NULL;
	config->callback_disconnect = NULL;
	config->callback_connection_error = NULL;
	config->callback_pingreq_error = NULL;
	config->callback_subscribe = NULL;
	config->callback_unsubscribe = NULL;
	config->callback_publish = NULL;
	config->callback_receive = NULL;
}


int32_t mqttClient_init (mqttClient_config_t* config) {
	struct mqtt_config mqtt_conf;
	int result;
	
	mqttClient.server = config->server;
	mqttClient.port = config->port;
	mqttClient.connected = false;
	mqttClient.cleanSession = config->cleanSession;
	mqttClient.keepalive = config->keepalive;
	mqttClient.isSecure = config->isSecure;
	
	mqttClient.clientId = config->clientId;
	mqttClient.user = config->user;
	mqttClient.password = config->password;
	
	mqttClient.bufferIn = config->bufferIn;
	mqttClient.bufferOut = config->bufferOut;
	mqttClient.bufferIn_len = config->bufferIn_len;
	mqttClient.bufferOut_len = config->bufferOut_len;
	
	mqttClient.bufferAux_len = 100;
	mqttClient.bufferAux_ptr = 0;
	
	mqttClient.callback_connect = config->callback_connect;
	mqttClient.callback_disconnect = config->callback_disconnect;
	mqttClient.callback_connection_error = config->callback_connection_error;
	mqttClient.callback_pingreq_error = config->callback_pingreq_error;
	mqttClient.callback_subscribe = config->callback_subscribe;
	mqttClient.callback_unsubscribe = config->callback_unsubscribe;
	mqttClient.callback_publish = config->callback_publish;
	mqttClient.callback_receive = config->callback_receive;
	
	
	mqtt_get_config_defaults(&mqtt_conf);
	mqtt_conf.tls = mqttClient.isSecure;
	mqtt_conf.read_buffer = mqttClient.bufferIn;
	mqtt_conf.read_buffer_size = mqttClient.bufferIn_len;
	mqtt_conf.send_buffer = mqttClient.bufferOut;
	mqtt_conf.send_buffer_size = mqttClient.bufferOut_len;
	mqtt_conf.port = mqttClient.port;
	mqtt_conf.keep_alive = mqttClient.keepalive;
	
	
	result = mqtt_init(&mqttClient.mqtt_inst, &mqtt_conf);
	if (result < 0)
	return result;

	result = mqtt_register_callback(&mqttClient.mqtt_inst, mqtt_callback);
	if (result < 0)
	return result;
	
	return 0;
}


void mqttClient_connect (char *will_topic, char *will_msg, uint32_t will_msg_len, uint8_t will_qos, uint8_t will_retain) {
	mqttClient.lwTopic = will_topic;
	mqttClient.lwMessage = will_msg;
	mqttClient.lwMessageLen = will_msg_len;
	mqttClient.lwQos = will_qos;
	mqttClient.lwRetain = will_retain;
	
	mqtt_connect(&mqttClient.mqtt_inst, mqttClient.server);
}


void mqttClient_disconnect (void) {
	mqtt_disconnect(&mqttClient.mqtt_inst, 0);
}


int mqttClient_getSocketId (void) {
	return mqttClient.mqtt_inst.network.socket;
}


void mqttClient_subscribe (char *topic, uint8_t qos) {
	mqtt_subscribe(
	&mqttClient.mqtt_inst,
	topic,
	qos,
	msgHandler);
}


void mqttClient_unsubscribe (char *topic) {
	mqtt_unsubscribe(&mqttClient.mqtt_inst, msgHandler);
}


void mqttClient_publish (const char *topic, const char *msg, uint32_t msg_len, uint8_t qos, uint8_t retain) {
	mqtt_publish(
	&mqttClient.mqtt_inst,
	topic,
	msg,
	msg_len,
	qos,
	retain);
}


bool mqttClient_isConnected (void) {
	return (mqttClient.connected);
}

int mqttClient_yield (void) {
	int rc = mqtt_yield(&mqttClient.mqtt_inst, 0);
	
	if (rc == CONN_ABORTED) {
		// Hubo un problema con la conexión
		mqttClient.connected = false;
		
		if(mqttClient.callback_connection_error != NULL)
		mqttClient.callback_connection_error();
	}
	else if (rc == PINGREQ_SEND_ERROR) {
		// Hubo errores al enviar los PINGREQ
		if(mqttClient.callback_pingreq_error != NULL)
		mqttClient.callback_pingreq_error();
	}
	
	return rc;
}


void mqttClient_clearSubscriptionHandlers (void) {
	mqtt_clearSubscriptionsHandlers (&mqttClient.mqtt_inst);
}


int32_t mqttClient_bufferPutByte (uint8_t b) {
	
	if (mqttClient.bufferAux_ptr < mqttClient.bufferAux_len) {
		mqttClient.bufferAux[mqttClient.bufferAux_ptr] = b;
		mqttClient.bufferAux_ptr ++;
		
		return 0;
	}
	
	return -1;
}


int32_t mqttClient_bufferPutBytes (uint8_t* bytes, uint32_t len) {
	uint32_t bytesWritten = 0;
	uint32_t dataOffset = mqttClient.bufferAux_ptr;
	
	
	for (uint32_t i = 0; i < len; i++) {
		
		if ( (dataOffset + i) < mqttClient.bufferAux_len) {
			mqttClient.bufferAux[dataOffset + i] = bytes[i];
			mqttClient.bufferAux_ptr ++;
			
			bytesWritten ++;
		}
		else {
			break;
		}
	}
	
	return bytesWritten;
}


uint8_t mqttClient_bufferGetChecksum (void) {
	uint32_t sum = 0;
	uint8_t checksum = 0;
	
	
	for (int i = 0; i < mqttClient.bufferAux_ptr; i++) {
		sum += mqttClient.bufferAux[i];
	}
	
	checksum = sum & 0x000000ff;
	
	return (0xff - checksum);
}


void mqttClient_bufferConvertToString (void) {
	uint8_t bufferAux_ptr2 = 0;
	
	for (int i = 0; i < mqttClient.bufferAux_ptr; i++) {
		convertNumberToHexString(&(mqttClient.bufferAux2[bufferAux_ptr2]), mqttClient.bufferAux[i]);
		
		bufferAux_ptr2 += 2;
	}
	
	// Se actualiza la longitud del mensaje a enviar
	mqttClient.bufferAux_ptr *= 2;
}


uint8_t* mqttClient_getBuffer (void) {
	return mqttClient.bufferAux2;
}


uint32_t mqttClient_getBufferLen (void) {
	return mqttClient.bufferAux_ptr;
}


void mqttClient_bufferFlush (void) {
	mqttClient.bufferAux_ptr = 0;
}



void mqtt_callback(struct mqtt_module *module_inst, int type, union mqtt_data *data) {
	switch (type) {
		case MQTT_CALLBACK_SOCK_CONNECTED:
		{
			// TODO falta agregar el LWT
			if (data->sock_connected.result >= 0) {
				mqtt_connect_broker(
				module_inst,
				mqttClient.cleanSession,
				mqttClient.user,
				mqttClient.password,
				mqttClient.clientId,
				mqttClient.lwTopic,
				mqttClient.lwMessage,
				mqttClient.lwMessageLen,
				mqttClient.lwQos,
				mqttClient.lwRetain);
				} else {
				// Falló la conexión al socket del broker
				mqttClient.connected = false;
				
				if(mqttClient.callback_connect != NULL)
				mqttClient.callback_connect(false);
			}
		}
		break;

		case MQTT_CALLBACK_CONNECTED:
		if (data->connected.result == MQTT_CONN_RESULT_ACCEPT) {
			mqttClient.connected = true;
			
			if(mqttClient.callback_connect != NULL)
			mqttClient.callback_connect(true);
			} else {
			// No se pudo conectar al broker
			mqttClient.connected = false;
			
			if(mqttClient.callback_connect != NULL)
			mqttClient.callback_connect(false);
		}

		break;
		
		case MQTT_CALLBACK_SUBSCRIBED:
		if (mqttClient.callback_subscribe != NULL)
		mqttClient.callback_subscribe();
		break;
		
		case MQTT_CALLBACK_UNSUBSCRIBED:
		if (mqttClient.callback_unsubscribe != NULL)
		mqttClient.callback_unsubscribe();
		break;
		
		case MQTT_CALLBACK_PUBLISHED:
		if (mqttClient.callback_publish != NULL)
		mqttClient.callback_publish();
		break;

		case MQTT_CALLBACK_DISCONNECTED:
		mqttClient.connected = false;
		
		if(mqttClient.callback_disconnect != NULL)
		mqttClient.callback_disconnect();
		break;
	}
}


void msgHandler (MessageData* msg) {
	if (mqttClient.callback_receive != NULL)
	mqttClient.callback_receive(msg->topicName, msg->message);
}