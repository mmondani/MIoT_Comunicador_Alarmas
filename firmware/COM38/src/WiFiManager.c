#include "../inc/WiFiManager.h"
#include "../inc/project.h"
#include "../inc/softTimers.h"
#include "../inc/debounce.h"
#include "../inc/BlinkingLed.h"
#include "../inc/stopWatch.h"
#include "../inc/socketManager.h"
#include "../inc/project.h"
#include "../inc/mainFsm.h"


static tenuM2mConnState wifi_connected;
static uint32_t wps_enable;
static uint32_t ap_enable;
static debouncePin_t* pushButton;
static blinkingLed_t blinkingLedVerde;

/**************************************************************/
// FSM para Provisioning WiFi
/**************************************************************/
typedef enum    {
	fsmProvisioning_init = 0,
	fsmProvisioning_chequearWiFiPrevio,
	fsmProvisioning_desconectado,
	fsmProvisioning_conectado,
	fsmProvisioning_escuchandoConexiones,
	fsmProvisioning_esperandoDatosWiFi,
	fsmProvisioning_modoWps,
	fsmProvisioning_conectando,
	fsmProvisioning_noSeConecto
}fsmProvisioning_state_t;
static fsmProvisioning_state_t fsmState;
static fsmProvisioning_state_t fsmState_previous;

static bool stateIn = true;
static bool stateOut = false;
static tstrM2MAPConfig apConfig;
static SoftTimer_t fsmTimer;
static stopWatch_t stopWatchPushButton;
static tstrM2MProvisionInfo provisioningInfo;
static bool provisioningInfoAvailable = false;
static bool chequearWiFiPrevio;
static bool pushButtonPressed5s;
static socket_t serverSocket;
static uint8_t apClientsCount;
static uint8_t apClientsCountPrev;

static void provisioningFsm_gotoState (fsmProvisioning_state_t nextState);
/**************************************************************/


/**************************************************************/
// Server socket
/**************************************************************/
uint8_t buffServerIn[50];
uint8_t buffServerOut[50];
uint8_t buffAux[50];

static void createServerSocket (void);
static void deleteServerSocket (void);

static void socket_connection_accepted (SOCKET id);
static void socket_client_sent (SOCKET id);
static void socket_client_received (SOCKET id, tstrSocketRecvMsg* msg);
static void socket_client_closed (SOCKET id);
/**************************************************************/


static void set_dev_name_to_mac(uint8 *name, uint8 *mac_addr);
static void wifi_cb(uint8_t u8MsgType, void *pvMsg);
static uint32_t isApEnable (void);
static void enableAp (tstrM2MAPConfig *config);
static void disableAp (void);


void wifiManager_init (debouncePin_t* button)
{
	tstrWifiInitParam param;
	int8_t ret;
	uint8_t mac_addr[6];
	uint8_t u8IsMacAddrValid;
	
	
	wifi_connected = WIFI_MANAGER_WIFI_DISCONNECTED;
	wps_enable = 0;
	ap_enable = false;
	
	fsmState = fsmProvisioning_init;
	fsmState_previous = fsmProvisioning_init;
	chequearWiFiPrevio = true;
	
	
	pushButton = button;
	
	
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
	
	
	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret) {
#ifdef DEBUG_PRINTF
		printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
#endif
		errores1.bits.errorModuloWifi = 1;
		return;
	}
	
	/* Connect to router. */
	m2m_wifi_get_otp_mac_address(mac_addr, &u8IsMacAddrValid);
	if (!u8IsMacAddrValid) {
		m2m_wifi_set_mac_address(gau8MacAddr);
	}

	m2m_wifi_get_mac_address(gau8MacAddr);
	
	// COMENTAR ESTA LÍNEA CUANDO SE USE UN DNS PUBLICO
	//m2m_wifi_enable_dhcp(0); 
}


void wifiManager_deinit (void)
{
	if(wifiManager_isWiFiConnected()) {
		m2m_wifi_disconnect();
	}
	
	wps_enable = 0;
	ap_enable = false;
	
	m2m_wifi_deinit(0);
}


static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
		{
			tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
			
			if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
				if (!isApEnable()) {
#ifdef DEBUG_PRINTF
					printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
#endif
					// DESCOMENTAR ESTA LÍNEA CUANDO SE USE UN DNS PUBLICO
					m2m_wifi_request_dhcp_client();


					// COMENTAR ESTE BLOQUE CUANDO SE USE UN DNS PUBLICO
					/*
					wifi_connected = M2M_WIFI_CONNECTED;
					tstrM2MIPConfig ip_client;
					ip_client.u32StaticIP = _htonl(0xc0a80184); //corresponde a 192.168.1.132
					ip_client.u32DNS = _htonl(0xc0a80165); //corresponde a 192.168.1.101
					ip_client.u32SubnetMask = _htonl(0xFFFFFF00); //corresponde a 255.255.255.0
					ip_client.u32Gateway = _htonl(0xc0a80101);  //corresponde a 192.168.1.1
					m2m_wifi_set_static_ip(&ip_client);
					*/
				}
				else {
#ifdef DEBUG_PRINTF
					printf("wifi_cb - AP: server iniciado\n\r");
#endif
					apClientsCount ++;
				}
			} 
			else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
				if (!isApEnable()) {
#ifdef DEBUG_PRINTF
					printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
#endif
					wifi_connected = WIFI_MANAGER_WIFI_DISCONNECTED;
				}
				else {
#ifdef DEBUG_PRINTF
					printf("wifi_cb - AP: cliente desconectado\n\r");
#endif

					if (apClientsCount > 0)
						apClientsCount --;
				}
			}

			break;
		}

		case M2M_WIFI_REQ_DHCP_CONF:
		{
			uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
			
			if (!isApEnable()) {
				wifi_connected = M2M_WIFI_CONNECTED;
#ifdef DEBUG_PRINTF
				printf("wifi_cb: M2M_WIFI_REQ_DHCP_CONF: IP is %u.%u.%u.%u\r\n",
					pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
#endif
			}
			else {
#ifdef DEBUG_PRINTF
				printf("wifi_cb - AP: nuevo cliente, IP asignada %u.%u.%u.%u\r\n", pu8IPAddress[0],
					pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
#endif
			}
			break;
		}

		case M2M_WIFI_REQ_WPS:
		{
			tstrM2MWPSInfo *pstrWps = (tstrM2MWPSInfo*)pvMsg;
			
			if (pstrWps->u8AuthType != 0) {
#ifdef DEBUG_PRINTF
				printf ("wifi_cb: M2M_WIFI_RESP_PROVISION_INFO: SSID: %s\n\r", pstrWps->au8SSID);
#endif

				// Se cargan los datos de aprovisionamiento que van a ser consumidos
				// por la FSM.
				strcpy(provisioningInfo.au8SSID, pstrWps->au8SSID);
				strcpy(provisioningInfo.au8Password, pstrWps->au8PSK);
				provisioningInfo.u8SecType = pstrWps->u8AuthType;
				
				provisioningInfoAvailable = true;
				

				wps_enable = 0;
			}
			break;
		}
		default:
		{
			break;
		}
	}
}


uint32_t wifiManager_isWiFiConnected (void)
{
	return (wifi_connected == WIFI_MANAGER_WIFI_CONNECTED && !isApEnable() && !wifiManager_isProvisioningEnable());
}


uint32_t wifiManager_isProvisioningEnable (void)
{
	return wps_enable;
}


void wifiManager_initWps (u8 wpsTrigger, const u8* pinNumber)
{
	m2m_wifi_wps(wpsTrigger, pinNumber);
	
	wps_enable = 1;
}


void wifiManager_stopWps (void)
{
	m2m_wifi_wps_disable();
	
	wps_enable = 0;
}


void wifiManager_provideProvisioningInfo (uint8_t* ssid, uint8_t* pass)
{
	strcpy(provisioningInfo.au8SSID, ssid);
	strcpy(provisioningInfo.au8Password, pass);
	provisioningInfo.u8SecType = M2M_WIFI_SEC_WPA_PSK;
				
	provisioningInfoAvailable = true;
}



/***********************************************************************************************************/
// FSM para los distintos modos de aprovisionamiento.
// Diagrama de estados: https://drive.google.com/file/d/12ytE7k4LvQulpFcJuIoqiyKUZ9YphD_Y/view?usp=sharing
/***********************************************************************************************************/
void wifiManager_ProvisioningFsmHandler (void)
{
	
	if (!mainFsm_estaEnPruebaFabrica()) {
		debouncePin_handler(pushButton);
	
		// Se chequea si la teclita se presionó por más de 5 segundos.
		if(debouncePin_getFlanco(pushButton) == DEBOUNCE_PIN_FLANCO_A_ACTIVO) {
			stopWatch_start(&stopWatchPushButton);
		}

		if (debouncePin_getFlanco(pushButton) == DEBOUNCE_PIN_FLANCO_A_PASIVO) {
			stopWatch_clear(&stopWatchPushButton);
		}
	
		debouncePin_clearFlanco(pushButton);
			
			
		if (stopWatch_currentElapsedTime (&stopWatchPushButton) > 5000) {
			stopWatch_clear(&stopWatchPushButton);
					
			pushButtonPressed5s = true;
		}
	}
	
	
	
	switch(fsmState) {
		case fsmProvisioning_init:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

				// 2 segundos de demora esperando a ver si llega el inicio de la prueba de fábrica
				softTimer_init(&fsmTimer, 2000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - init\n\r");
#endif
            }

            //**********************************************************************************************
			if (softTimer_expired(&fsmTimer)) {
				provisioningFsm_gotoState(fsmProvisioning_chequearWiFiPrevio);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
		
		case fsmProvisioning_chequearWiFiPrevio:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				// Se intenta conectar a un WiFi al que ya se había conectado
				// Si está en prueba de fábrica, no intenta conectarse al anterior
				if (!mainFsm_estaEnPruebaFabrica()) {
					m2m_wifi_default_connect();
					softTimer_init(&fsmTimer, 5000);
				}
				else {
					softTimer_init(&fsmTimer, 100);
				}
				
				
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - chequea wifi previo\n\r");
#endif
            }

            //**********************************************************************************************
			if (softTimer_expired(&fsmTimer)) {
				provisioningFsm_gotoState(fsmProvisioning_desconectado);
			}
			
			if (wifiManager_isWiFiConnected()) {
				provisioningFsm_gotoState(fsmProvisioning_conectado);
			}


            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case fsmProvisioning_desconectado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - desconectado\n\r");
#endif
            }

            //**********************************************************************************************
			provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case fsmProvisioning_escuchandoConexiones:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				// Se configuran los parámetros de AP generado por el COM30
				strcpy(apConfig.au8SSID, gacDeviceName);
				apConfig.u8ListenChannel = 1;
				apConfig.u8SecType = M2M_WIFI_SEC_OPEN;
				/*
				apConfig.u8SecType = M2M_WIFI_SEC_WEP; // Set Security to WEP
				apConfig.u8KeyIndx = 0; // Set WEP Key Index
				apConfig.u8KeySz = WEP_40_KEY_STRING_SIZE; // Set WEP Key Size
				strcpy(apConfig.au8WepKey, "1234567890"); // Set WEP Key
				*/
				
				apConfig.u8SsidHide = SSID_MODE_VISIBLE;

				apConfig.au8DHCPServerIP[0]	= 192;
				apConfig.au8DHCPServerIP[1]	= 168;
				apConfig.au8DHCPServerIP[2]	= 1;
				apConfig.au8DHCPServerIP[3]	= 1;

				set_dev_name_to_mac((uint8_t *)gacDeviceName, gau8MacAddr);
				set_dev_name_to_mac((uint8_t *)apConfig.au8SSID, gau8MacAddr);
				m2m_wifi_set_device_name((uint8_t *)gacDeviceName, (uint8_t)m2m_strlen((uint8_t *)gacDeviceName));
				
				// Se inicia el AP con estos parámetros
				apClientsCount = 0;
				apClientsCountPrev = 0;
				enableAp((tstrM2MAPConfig *)&apConfig);
				
				// Se crea el server socket para recibir datos de provisioning de la app
				createServerSocket();
				
				softTimer_init(&fsmTimer, 30000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - escuchando conexiones\n\r");
#endif
            }

            //**********************************************************************************************
			if (softTimer_expired(&fsmTimer) && apClientsCount == 0) {
				if (chequearWiFiPrevio == true) {
					deleteServerSocket();
					disableAp();
					provisioningFsm_gotoState(fsmProvisioning_chequearWiFiPrevio);
				}
			}

			if (pushButtonPressed5s == true) {
				provisioningInfoAvailable = false;
				provisioningFsm_gotoState(fsmProvisioning_modoWps);
			}
			
			if (apClientsCount == 0 && apClientsCountPrev > 0) {
				deleteServerSocket();
				disableAp();
				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);
			}
			
			// Se chequea si se conectó alguna app al AP creado
			if (socketManager_socketServerIsClientConnected(&serverSocket)) {
				provisioningFsm_gotoState(fsmProvisioning_esperandoDatosWiFi);
			}
			
			
			if (provisioningInfoAvailable) {
				// La info de provisioning vino del probador durante la prueba de fábrica
				provisioningInfoAvailable = false;
				
				// Una vez que se reciben los datos, se cierra el socket
				deleteServerSocket();
				
				// Se termina el modo AP
				disableAp();
				
				provisioningFsm_gotoState(fsmProvisioning_conectando);				
			}
			

			apClientsCountPrev = apClientsCount;
			
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				provisioningInfoAvailable = false;
            }

			break;
			
			
		case fsmProvisioning_conectado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				chequearWiFiPrevio = true;
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - conectado\n\r");
#endif
            }

            //**********************************************************************************************
			if (!wifiManager_isWiFiConnected()) {
				provisioningFsm_gotoState(fsmProvisioning_desconectado);
			}
			
			
			if (pushButtonPressed5s == true) {
				chequearWiFiPrevio = false;
				
				m2m_wifi_disconnect();
				
				provisioningFsm_gotoState(fsmProvisioning_desconectado);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case fsmProvisioning_esperandoDatosWiFi:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				
				softTimer_init(&fsmTimer, 300000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - esperando datos wifi\n\r");
#endif
            }

            //**********************************************************************************************
			
			if (provisioningInfoAvailable) {
				provisioningInfoAvailable = false;
				provisioningFsm_gotoState(fsmProvisioning_conectando);
			}
			else if (socketManager_socketServerIsClientConnected(&serverSocket) == 0)
				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);
			else if (softTimer_expired(&fsmTimer))
				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);
				
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				// Una vez que se reciben los datos, se cierra el socket
				deleteServerSocket();
				
				// Se termina el modo AP
				disableAp();
            }

			break;
			
			
		case fsmProvisioning_modoWps:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				deleteServerSocket();
				disableAp();
				
				wifiManager_initWps(WPS_PBC_TRIGGER, NULL);

				softTimer_init(&fsmTimer, 60000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - modo WPS\n\r");
#endif
            }

            //**********************************************************************************************
			// Si en 60 segundos no llegan datos de aprovisionamiento, se termina el modo
			// provisioning HTTP
			if (softTimer_expired(&fsmTimer)) {
				wifiManager_stopWps();
				
				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);
			}
			
			
			if (pushButtonPressed5s == true) {
				wifiManager_stopWps();

				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);
			}


			// Se espera a que lleguen datos de aprovisionamiento
			if (provisioningInfoAvailable) {
				provisioningFsm_gotoState(fsmProvisioning_conectando);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case fsmProvisioning_conectando:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
#ifdef DEBUG_PRINTF
				printf("wifiManager - Recibidos datos de aprovisionamiento\n\r");
				printf ("\tSSID: %s\n\r", provisioningInfo.au8SSID);
				printf("\tPass: %s\n\r", provisioningInfo.au8Password);
				printf("\tSec. type: %d\n\r", provisioningInfo.u8SecType);
#endif
				
				sint8 ret = m2m_wifi_connect((char *)provisioningInfo.au8SSID,
					strlen((char *)provisioningInfo.au8SSID),
					provisioningInfo.u8SecType,
					provisioningInfo.au8Password, M2M_WIFI_CH_ALL);
					
				
				softTimer_init(&fsmTimer, 4000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - conectando\n\r");
#endif
            }

            //**********************************************************************************************
			/*if (wifiManager_isWiFiConnected()) {
				provisioningFsm_gotoState(fsmProvisioning_conectado);
			}*/
			
			
			if (softTimer_expired(&fsmTimer)) {
				if (wifiManager_isWiFiConnected()) {
					provisioningFsm_gotoState(fsmProvisioning_conectado);
				}
				else {
					provisioningFsm_gotoState(fsmProvisioning_noSeConecto);
				}
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case fsmProvisioning_noSeConecto:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

				softTimer_init(&fsmTimer, 5000);
				
#ifdef DEBUG_PRINTF
				printf ("wifiManager - no se conecto\n\r");
#endif
            }

            //**********************************************************************************************
			if (softTimer_expired(&fsmTimer))
				provisioningFsm_gotoState(fsmProvisioning_escuchandoConexiones);

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			break;
		
		default:
			break;
	}
	
	pushButtonPressed5s = false;
}


static void provisioningFsm_gotoState (fsmProvisioning_state_t nextState) 
{
	fsmState_previous = fsmState;
	fsmState = nextState;
	
	stateIn = false;
	stateOut = true;
}



static uint32_t isApEnable (void)
{
	return ap_enable;
}


static void enableAp (tstrM2MAPConfig *config)
{
	m2m_wifi_enable_ap(config);
	ap_enable = 1;
	
#ifdef DEBUG_PRINTF
	printf("Modo AP iniciado\n\r");
#endif
}


static void disableAp (void)
{
	if (ap_enable == 1) {
		ap_enable = 0;
		
		m2m_wifi_disable_ap();
		
#ifdef DEBUG_PRINTF
		printf("Modo AP detenido\n\r");
#endif
	}
}


static void createServerSocket (void) {
	socket_config_t socketServerConfig;
	
	/*
	Socket server para recibir datos de Provisioning WiFi
	*/
	socketManager_loadConfigDefaults(&socketServerConfig);
	socketServerConfig.server = 1;
	socketServerConfig.port = MAIN_WIFI_M2M_SOCKET_SERVER_PORT;
	socketServerConfig.callback_server_connection_accepted = socket_connection_accepted;
	socketServerConfig.callback_client_closed = socket_client_closed;
	socketServerConfig.callback_received =socket_client_received ;
	socketServerConfig.callback_sent = socket_client_sent;
	socketServerConfig.bufferIn = buffServerIn;
	socketServerConfig.bufferOut = buffServerOut;
	socketServerConfig.bufferIn_len = sizeof(buffServerIn);
	socketServerConfig.bufferOut_len = sizeof(buffServerOut);
	
	if (socketManager_createSocket(&serverSocket, &socketServerConfig) < 0) {
#ifdef DEBUG_PRINTF
		printf("wifiManager - error creating server socket\n\r");
#endif
		errores1.bits.errorServerSocket = 1;
	}
	else {
#ifdef DEBUG_PRINTF
		printf("wifiManager - server socket created\n\r");
#endif
		socketManager_connect(&serverSocket);
	}
	
	
}


static void deleteServerSocket (void) {
	if (socketManager_isListening(&serverSocket)) {
#ifdef DEBUG_PRINTF
		printf("wifiManager - server socket closed\n\r");
#endif
		
		socketManager_close(&serverSocket);
		socketManager_deleteSocket(&serverSocket);
	}
}


static void socket_connection_accepted (SOCKET id)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	
#ifdef DEBUG_PRINTF
	printf("wifiManager - new connection!\r\n");
#endif
	
	
	socketManager_startReceiving(sock);
}


static void socket_client_sent (SOCKET id)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	
#ifdef DEBUG_PRINTF
	printf("wifiManager - send success!\r\n");
#endif
	
	socketManager_flushBufferOut (sock);
}


static void socket_client_received (SOCKET id, tstrSocketRecvMsg* msg)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	uint32_t sum = 0;
	
	
	if (msg && msg->s16BufferSize > 0) {
#ifdef DEBUG_PRINTF
		printf("wifiManager - socket: recv success!\r\n");
#endif
		
		for (int i = 0; i < msg->s16BufferSize; i++) {
			buffAux[i] = msg->pu8Buffer[i];
		}
		
		socketManager_bufferOutPutBytes(sock, buffAux, msg->s16BufferSize);
		socketManager_send(sock);
		
		/*
		Se cargan los datos de Provisioning.
		Los datos que manda la app son: SSID&password&seguridad
		Seguridad:
			- 0: open
			- 1: WEP
			- 2: WPA/WPA2 Personal
			- 3: WPA/WPA2 Enterprise
		*/
		int  j = 0;
		int i = 0;
		for  (i = 0; buffAux[i] != '&' && i < msg->s16BufferSize; i++, j++) {
			provisioningInfo.au8SSID[j] = buffAux[i];
		}
		provisioningInfo.au8SSID[j] = '\0';
		
		if (i >= msg->s16BufferSize) {
			socketManager_flushBufferIn(sock);
			return;
		}
		i++;
		
		j = 0;
		for  (; buffAux[i] != '&' && i < msg->s16BufferSize; i++, j++) {
			provisioningInfo.au8Password[j] = buffAux[i];
		}
		provisioningInfo.au8Password[j] = '\0';
		
		if (i >= msg->s16BufferSize) {
			socketManager_flushBufferIn(sock);
			return;
		}
		i++;
		
		
		switch (buffAux[i] - 0x30) {
			case 0:
				provisioningInfo.u8SecType = M2M_WIFI_SEC_OPEN;
				break;
				
			case 1:
				provisioningInfo.u8SecType = M2M_WIFI_SEC_WEP;
				break;
				
			case 2:
				provisioningInfo.u8SecType = M2M_WIFI_SEC_WPA_PSK;
				break;
				
			case 3:
				provisioningInfo.u8SecType = M2M_WIFI_SEC_802_1X;
				break;
				
			default:
				provisioningInfo.u8SecType = M2M_WIFI_SEC_OPEN;
				break;
		}

		
		
		provisioningInfoAvailable = true;
		
		
		socketManager_flushBufferIn(sock);
	}
}


static void socket_client_closed (SOCKET id)
{
	socket_t* sock = socketManager_getSocketBySocketId(id);
	
#ifdef DEBUG_PRINTF
	printf("wifiManager - client socket closed!\r\n");
#endif
}



#define HEX2ASCII(x) (((x) >= 10) ? (((x) - 10) + 'A') : ((x) + '0'))
static void set_dev_name_to_mac(uint8 *name, uint8 *mac_addr)
{
	/* Name must be in the format WINC1500_00:00 */
	uint16 len;

	len = m2m_strlen(name);
	if (len >= 5) {
		name[len - 1] = HEX2ASCII((mac_addr[5] >> 0) & 0x0f);
		name[len - 2] = HEX2ASCII((mac_addr[5] >> 4) & 0x0f);
		name[len - 4] = HEX2ASCII((mac_addr[4] >> 0) & 0x0f);
		name[len - 5] = HEX2ASCII((mac_addr[4] >> 4) & 0x0f);
	}
}