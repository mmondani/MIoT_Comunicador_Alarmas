#ifndef PROJECT_H_
#define PROJECT_H_


#include "asf.h"

#define DEBUG_PRINTF						// Comentar para producción.
// Cambiar el #define CONF_WINC_DEBUG

/** Wi-Fi */
#define MAIN_M2M_DEVICE_NAME				"COM38_00:00"
#define MAIN_MAC_ADDRESS                     {0xf8, 0xf0, 0x05, 0x45, 0xD4, 0x84}

/** Servidor Provisioning */
#define MAIN_WIFI_M2M_SOCKET_SERVER_PORT	(6666)

/** Opciones de producto */
#define PROJECT_FIRMWARE_VERSION_MAYOR		0
#define PROJECT_FIRMWARE_VERSION_MENOR		1


static uint8_t gau8MacAddr[] = MAIN_MAC_ADDRESS;
static int8_t gacDeviceName[] = MAIN_M2M_DEVICE_NAME;



union Errores1{
	uint32_t w;
	struct {
		uint8_t errorModuloWifi:1;
		uint8_t errorServerSocket:1;
		uint8_t errorEeprom:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} errores1;

union Comnados1{
	uint8_t byte;
	struct {
		uint8_t pedir_fyh:1;
		uint8_t mandar_fyh:1;
		uint8_t mandar_097:1;
		uint8_t mandar_626:1;
		uint8_t mandar_623:1;
		uint8_t mandar_624:1;
		uint8_t mandar_0a6:1;
		uint8_t mandar_0a7:1;
	} bits;
} comandos1;

union Comnados2{
	uint8_t byte;
	struct {
		uint8_t mandar_0a8:1;
		uint8_t mandar_625:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
	} bits;
} comandos2;


uint8_t pedirStatusCentral;
uint8_t pedirReplay;
uint8_t mandarBorrarMemoria;
uint8_t pedirEventos;
uint8_t mandarPanico;
uint8_t mandarIncendioManual;
uint8_t mandarEmergenciaMedica;
uint8_t pedirNombreZonas;
uint8_t mandarApagarTodo;
uint8_t mandarEnProgramacion;

uint8_t mandarTeclas;
uint8_t bufferTeclas[50];
uint8_t ptrTeclas;
uint8_t lenTeclas;

uint8_t mandarMensajeMpxh;
uint8_t mensajeMpxh_dataH;
uint8_t mensajeMpxh_dataL;

uint8_t bufferTlcd[34];
uint8_t bufferTlcd_puntero;
uint8_t mandarBufferTlcd;

uint8_t mandarAgudoIncondicional;
uint8_t mandarPiru;
uint8_t mandarPiruPiru;
uint8_t mandarGracias;
uint8_t mandarError;
uint8_t mandarGrave;
uint8_t mandarProgramacion;
uint8_t mandarAvanzada;

#endif /* PROJECT_H_ */