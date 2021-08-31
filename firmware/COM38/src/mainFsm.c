#include "asf.h"
#include "inc/project.h"
#include "inc/debounce.h"
#include "inc/mainTimer.h"
#include "inc/softTimers.h"
#include "inc/stopWatch.h"
#include "inc/socketManager.h"
#include "inc/WiFiManager.h"
#include "inc/imClient.h"
#include "inc/mpxh.h"
#include "inc/project.h"
#include "inc/imClient_cmds_regs.h"
#include "inc/MessagePoolManager.h"
#include "inc/BlinkingLed.h"
#include "inc/BlinkingPatterns.h"
#include "inc/uartRB.h"
#include "inc/ext_eeprom.h"
#include "inc/pga.h"
#include "inc/dateTime.h"
#include "inc/displayRAM.h"
#include "inc/fsmProg.h"



static blinkingLed_t blinkingLedVerde;
static blinkingLed_t blinkingLedRojo;

static debouncePin_t pinSw0;


struct usart_module* uart_module;


/**************************************************************/
// FSM principal para el manejo de la lógica del WIFICOM100
/**************************************************************/
typedef enum    {
	mainFsm_init_state = 0,
	mainFsm_desconectado,
	mainFsm_pruebaFabrica,
	mainFsm_esperandoConexion,
	mainFsm_modoWps,
	mainFsm_esperandoServerIm,
	mainFsm_conectadoServerIm,
	mainFsm_programacion,
	mainFsm_error,
	mainFsm_deinit
}mainFsm_state_t;
static mainFsm_state_t fsmState;
static mainFsm_state_t fsmState_previous;

static bool stateIn = true;
static bool stateOut = false;

static SoftTimer_t timerState;

static bool vino_0d2;

static void mainFsm_gotoState (mainFsm_state_t nextState);
static void mainFsm_gotoPreviousState (void);




void mainFsm_init (struct usart_module * uart)
{
	fsmState = mainFsm_init_state;
	fsmState_previous = mainFsm_init_state;
	
	blinkingLed_init(&blinkingLedVerde, LED_VERDE_PIN);
	blinkingLed_init(&blinkingLedRojo, LED_ROJO_PIN);


	uart_module = uart;
}


void mainFsm_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits)
{
	switch (nbits) {
		case MPXH_BITS_17:
			if (dataH == 0x0d && dataL == 0x20)
				vino_0d2 = true;
				
			break;
	}
}


void mainFsm_handler (void)
{

	// En la prueba de fábrica los leds son controlados por la FSM de la prueba
	if (fsmState != mainFsm_pruebaFabrica) {
		blinkingLed_handler(&blinkingLedVerde);
		blinkingLed_handler(&blinkingLedRojo);
	}
	
	
	if (fsmState == mainFsm_init_state || fsmState == mainFsm_desconectado ||
		fsmState == mainFsm_esperandoConexion || fsmState == mainFsm_modoWps ||
		fsmState == mainFsm_esperandoServerIm || fsmState == mainFsm_conectadoServerIm) {
			if (errores1.w != 0)
				mainFsm_gotoState(mainFsm_error);
			else if (fsmProg_enProgramacion())
				mainFsm_gotoState(mainFsm_programacion);
		}
	
	
	switch (fsmState) {
		case mainFsm_init_state:
			if (stateIn)
            {
				uint8_t idBuffer[4];
					
                stateIn = false;
                stateOut = false;
				
				if (fsmState_previous != mainFsm_pruebaFabrica) {
					nm_bsp_init();
					debouncePin_init(&pinSw0, DEBOUNCE_PIN_BAJO_ACTIVO, BUTTON_0_PIN);
				
					
					wifiManager_init(&pinSw0);
					socketManager_init();
				
					// Se chequea la EEPROM externa para determinar si hay que inicializarla o leerla
					ext_eeprom_init();
					pga_init();
					pga_checkEeprom();
					
					displayRAM_init();

					idBuffer[0] = pgaData[PGA_ID_DISPOSITIVO];
					idBuffer[1] = pgaData[PGA_ID_DISPOSITIVO+1];
					idBuffer[2] = pgaData[PGA_ID_DISPOSITIVO+2];
					idBuffer[3] = pgaData[PGA_ID_DISPOSITIVO+3];
					imClient_init(idBuffer);
					
					dateTime_init();
				}
				
				vino_0d2 = false;
				
				softTimer_init(&timerState, 2000);
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
				blinkingLed_setPattern(&blinkingLedRojo, BLINKING_INIT, BLINKING_INIT_LEN, BLINKING_INIT_BASE);
            }

            //**********************************************************************************************
			// Espero el mensaje de mpxh 0D2 para entrar en prueba de fábrica
			if (vino_0d2) {
				mainFsm_gotoState(mainFsm_pruebaFabrica);
			}
			
			if (softTimer_expired(&timerState)) {
				mainFsm_gotoState(mainFsm_desconectado);
			}
			

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedRojo, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
            }

			break;
			
		case mainFsm_desconectado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				softTimer_init(&timerState, 5000);
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_DESCONECTADO, BLINKING_DESCONECTADO_LEN, BLINKING_DESCONECTADO_BASE);
            }

            //**********************************************************************************************
			if (wifiManager_isWiFiConnected()) {
				mainFsm_gotoState(mainFsm_esperandoServerIm);
			}
			else if (softTimer_expired(&timerState)) {
				mainFsm_gotoState(mainFsm_esperandoConexion);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_pruebaFabrica:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				bit_set(mandarPiruPiru,0);
				
				//softTimer_init(&timerState, 10000);
				
            }

            //**********************************************************************************************
			
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_esperandoConexion:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_ESCUCHANDO, BLINKING_ESCUCHANDO_LEN, BLINKING_ESCUCHANDO_BASE);
            }

            //**********************************************************************************************
			if (wifiManager_isProvisioningEnable()) {
				mainFsm_gotoState(mainFsm_modoWps);
			}
			else if (wifiManager_isWiFiConnected()) {
				mainFsm_gotoState(mainFsm_esperandoServerIm);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_modoWps:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_MODO_WPS, BLINKING_MODO_WPS_LEN, BLINKING_MODO_WPS_BASE);
            }

            //**********************************************************************************************
			if (wifiManager_isWiFiConnected()) {
				mainFsm_gotoState(mainFsm_esperandoServerIm);
			}
			else if (!wifiManager_isProvisioningEnable()) {
				mainFsm_gotoState(mainFsm_esperandoConexion);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_esperandoServerIm:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_ESPERANDO_IM_VERDE, BLINKING_ESPERANDO_IM_VERDE_LEN, BLINKING_ESPERANDO_IM_VERDE_BASE);
				blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ESPERANDO_IM_ROJO, BLINKING_ESPERANDO_IM_ROJO_LEN, BLINKING_ESPERANDO_IM_ROJO_BASE);
            }

            //**********************************************************************************************
			if (!wifiManager_isWiFiConnected()) {
				mainFsm_gotoState(mainFsm_esperandoConexion);
			}
			else if (imClient_isClientConnected()) {
				mainFsm_gotoState(mainFsm_conectadoServerIm);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedRojo, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
            }

			break;
			
			
		case mainFsm_conectadoServerIm:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CONECTADO, BLINKING_CONECTADO_LEN, BLINKING_CONECTADO_BASE);
            }

            //**********************************************************************************************
			if (!wifiManager_isWiFiConnected()) {
				mainFsm_gotoState(mainFsm_esperandoConexion);
			}
			else if (imClient_isClientConnected() == 0) {
				mainFsm_gotoState(mainFsm_esperandoServerIm);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_programacion:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedRojo, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_PROGRAMACION, BLINKING_PROGRAMACION_LEN, BLINKING_PROGRAMACION_BASE);
            }

            //**********************************************************************************************
			if (!fsmProg_enProgramacion())
				mainFsm_gotoPreviousState();

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_error:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
				
				if (errores1.bits.errorEeprom)
					blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_EEPROM, BLINKING_ERROR_EEPROM_LEN, BLINKING_ERROR_EEPROM_BASE);
				else if (errores1.bits.errorModuloWifi)
					blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_MODULO_WIFI, BLINKING_ERROR_MODULO_WIFI_LEN, BLINKING_ERROR_MODULO_WIFI_BASE);
            }

            //**********************************************************************************************
			// TO-DO gestión de errores

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
			
			
		case mainFsm_deinit:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
				
				wifiManager_deinit();
				nm_bsp_deinit();
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
            }

            //**********************************************************************************************
			mainFsm_gotoState(mainFsm_init_state);

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }

			break;
	}
}


void mainFsm_gotoState (mainFsm_state_t nextState)
{
	fsmState_previous = fsmState;
	fsmState = nextState;
	
	stateIn = false;
	stateOut = true;
}


void mainFsm_gotoPreviousState (void)
{
	mainFsm_gotoState(fsmState_previous);
}


bool mainFsm_hayErrorCritico (void) 
{
	return (errores1.bits.errorEeprom || errores1.bits.errorModuloWifi);
}


bool mainFsm_estaEnPruebaFabrica (void)
{
	return (fsmState == mainFsm_pruebaFabrica);
}