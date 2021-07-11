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


#endif /* PROJECT_H_ */