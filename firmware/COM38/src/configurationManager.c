#include "inc/configurationManager.h"
#include "inc/project.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/pga.h"
#include "inc/PGA_Map.h"
#include "inc/EEPROM_MAP.h"
#include "inc/alarmMonitor.h"
#include "inc/dateTime.h"
#include "inc/imClient.h"


static void procesarSetTiempo(uint8_t* payload, uint8_t payloadLen);
static void procesarSetRobo(uint8_t* payload, uint8_t payloadLen);
static void procesarSetNombreCom(uint8_t* payload, uint8_t payloadLen);
static void procesarSetMonitoreo(uint8_t* payload, uint8_t payloadLen);
static void procesarSetSocketBroker(uint8_t* payload, uint8_t payloadLen);

static void armarPayloadGetNombreCom (bool esEvento);
static void armarPayloadGetConfiguracionTiempo (bool esEvento);
static void armarPayloadGetConfiguracionRobo (bool esEvento);
static void armarPayloadGetConfiguracionMonitoreo (bool esEvento);
static void armarPayloadGetSocketBroker (bool esEvento);




void configurationManager_init (void)
{
	
}


bool configurationManager_analizarIm (imMessage_t* msg)
{
	bool ret = false;
	
	if (msg->cmd == IM_CLIENT_CMD_GET) {
		if (msg->reg == IM_CLIENT_REG_CONFIGURACION_TIEMPO) {
			configurationManager_armarGetConfiguracionTiempo();
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_ROBO) {
			configurationManager_armarGetConfiguracionRobo();
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_NOMBRE_COM) {
			configurationManager_armarGetNombreCom();
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_MONITOREO) {
			configurationManager_armarGetConfiguracionMonitoreo();
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_SOCKET_BROKER) {
			configurationManager_armarGetSocketBroker();
			imClient_removeMessageToRead(0);
			ret = true;
		}
	}
	else if (msg->cmd == IM_CLIENT_CMD_SET) {
		if (msg->reg == IM_CLIENT_REG_CONFIGURACION_TIEMPO) {
			procesarSetTiempo(msg->payload, msg->len);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_ROBO) {
			procesarSetRobo(msg->payload, msg->len);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_NOMBRE_COM) {
			procesarSetNombreCom(msg->payload, msg->len);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_CONFIGURACION_MONITOREO) {
			procesarSetMonitoreo(msg->payload, msg->len);
			imClient_removeMessageToRead(0);
			ret = true;
		}
		else if (msg->reg == IM_CLIENT_REG_SOCKET_BROKER) {
			procesarSetSocketBroker(msg->payload, msg->len);
			imClient_removeMessageToRead(0);
			ret = true;
		}
	}
	
	return ret;
}


void configurationManager_armarGetConfiguracionTiempo (void)
{
	armarPayloadGetConfiguracionTiempo(false);
}


void configurationManager_armarGetConfiguracionRobo (void)
{
	armarPayloadGetConfiguracionRobo(false);
}


void configurationManager_armarGetNombreCom (void)
{
	armarPayloadGetNombreCom(false);
}


void configurationManager_armarGetConfiguracionMonitoreo (void)
{
	armarPayloadGetConfiguracionMonitoreo(false);
}

void configurationManager_armarGetSocketBroker (void)
{
	armarPayloadGetSocketBroker(false);
}


void configurationManager_armarEventoConfiguracionTiempo (void)
{
	armarPayloadGetConfiguracionTiempo(true);
}


void configurationManager_armarEventoConfiguracionRobo (void)
{
	armarPayloadGetConfiguracionRobo(true);
}


void configurationManager_armarEventoNombreCom (void)
{
	armarPayloadGetNombreCom(true);
}


void configurationManager_armarEventoConfiguracionMonitoreo (void)
{
	armarPayloadGetConfiguracionMonitoreo(true);
}

void configurationManager_armarEventoSocketBroker (void)
{
	armarPayloadGetSocketBroker(true);
}



void procesarSetTiempo(uint8_t* payload, uint8_t payloadLen)
{
	if (payloadLen != 2)
	return;
	
	if (payload[0] < 2)
	dateTime_setSincronizarInternet(payload[0]);
	
	dateTime_setRegionCode(payload[1]);
}


void procesarSetRobo(uint8_t* payload, uint8_t payloadLen)
{
	if (payloadLen != 8)
	return;
	
	for (uint8_t layer = 0; layer < 8; layer++) {
		if (payload[layer] > 0 && payload[layer] <= 30)
		alarmMonitor_setRetardo(layer, payload[layer]);
	}
}


void procesarSetNombreCom(uint8_t* payload, uint8_t payloadLen)
{
	uint8_t i;
	
	for (i = 0; i < payloadLen && payload[i] != '\0' && i < 16 ; i++)
	pgaData[PGA_NOMBRE_DISP + i] = payload[i];
	
	if (i < 16)
	pgaData[PGA_NOMBRE_DISP + i] = 0xFF;
	
	
	pga_enqueueSave(PGA_NOMBRE_DISP, 16, EE_NOMBRE_DISP_ADDR);
	
	
	// Se envía un evento con el cambio de nombre para que todos se enteren
	configurationManager_armarEventoNombreCom();
}


void procesarSetMonitoreo(uint8_t* payload, uint8_t payloadLen)
{
	if (payloadLen != 2)
	return;
	
	pgaData[PGA_MONITOREADA] = payload[0];
	pgaData[PGA_APP] = payload[1];
	
	pga_enqueueSave(PGA_MONITOREADA, 2, EE_MONITOREADA);
	
	// Se envía un evento con el cambio de la configuración para que todos se enteren
	configurationManager_armarEventoConfiguracionMonitoreo();
}

void procesarSetSocketBroker(uint8_t* payload, uint8_t payloadLen)
{
	uint8_t i;
	
	for (i = 0; i < payloadLen && i < 2 ; i++)
	pgaData[PGA_BROKER_PORT + 1 - i] = payload[i];
	
	for (i = 0; (i + 2) < payloadLen && payload[i + 2] != 0xff && i < 70 ; i++)
	pgaData[PGA_BROKER_URL + i] = payload[i + 2];
	
	if (i < 70)
	pgaData[PGA_BROKER_URL + i] = '\0';
	
	
	pga_enqueueSave(PGA_BROKER_PORT, 2, EE_BROKER_PORT);
	pga_enqueueSave(PGA_BROKER_URL, 70, EE_BROKER_URL);
	
	
	// Se envía un evento con el cambio de broker para que todos se enteren
	configurationManager_armarEventoSocketBroker();
	
	// Se reinicia la coneción del imClient para que use los nuevos datos
	imClient_resetConnection();
}


static void armarPayloadGetNombreCom (bool esEvento)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		imClient_putPayloadBytes(msg, &(pgaData[PGA_NOMBRE_DISP]), 16);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		
		if (esEvento)
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_NOMBRE_COM, 0, 0);
		else
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_NOMBRE_COM, 0, 0);
	}
}


static void armarPayloadGetConfiguracionTiempo (bool esEvento)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg,dateTime_getSincronizarInternet());
		imClient_putPayloadByte(msg,dateTime_getRegionCode());
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		
		if (esEvento)
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_CONFIGURACION_TIEMPO, 0, 0);
		else
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_CONFIGURACION_TIEMPO, 0, 0);
	}
}


static void armarPayloadGetConfiguracionRobo (bool esEvento)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		for (uint8_t layer = 0; layer < 8; layer++) {
			imClient_putPayloadByte(msg, alarmMonitor_getRetardo(layer));
		}
		
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		
		if (esEvento)
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_CONFIGURACION_ROBO, 0, 0);
		else
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_CONFIGURACION_ROBO, 0, 0);
	}
}


static void armarPayloadGetConfiguracionMonitoreo (bool esEvento)
{
	uint8_t toId[4];
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg,pgaData[PGA_MONITOREADA]);
		imClient_putPayloadByte(msg,pgaData[PGA_APP]);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		
		if (esEvento)
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_CONFIGURACION_MONITOREO, 0, 0);
		else
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_CONFIGURACION_MONITOREO, 0, 0);
	}
}

static void armarPayloadGetSocketBroker (bool esEvento)
{
	uint8_t toId[4];
	uint32_t i;
	imMessage_t* msg = imClient_getFreeMessageSlot();
	
	if (msg != NULL) {
		imClient_putPayloadByte(msg, pgaData[PGA_BROKER_PORT+1]);
		imClient_putPayloadByte(msg, pgaData[PGA_BROKER_PORT]);
		
		for (i = 0; i < 70 && pgaData[PGA_BROKER_URL + i] != '\0'; i++)
		imClient_putPayloadByte(msg, pgaData[PGA_BROKER_URL + i]);
		
		if (i < 70)
		imClient_putPayloadByte(msg, 0xff);
		
		toId[0] = pgaData[PGA_ID_DISPOSITIVO];
		toId[1] = pgaData[PGA_ID_DISPOSITIVO+1];
		toId[2] = pgaData[PGA_ID_DISPOSITIVO+2];
		toId[3] = pgaData[PGA_ID_DISPOSITIVO+3];
		
		if (esEvento)
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_EVENT, IM_CLIENT_REG_SOCKET_BROKER, 0, 0);
		else
		imClient_send(msg, toId, MESSAGE_POOL_FLOW_WCOM, IM_CLIENT_CMD_RESP_GET, IM_CLIENT_REG_SOCKET_BROKER, 0, 0);
	}
}