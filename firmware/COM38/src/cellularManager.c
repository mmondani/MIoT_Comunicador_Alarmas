#include "inc/cellularManager.h"
#include "inc/softTimers.h"
#include "inc/BG96.h"


static uint32_t internet_connected;
static struct usart_module* bg96_uart_module;
static bool catMEnabled;
static bool bands4gEnabled;
static uint8_t* simPin;

static int32_t pdpContextId;
static int32_t sslContextId;

static void bg96Callback (bg96_module_events evt, void* payload);


/**************************************************************/
// FSM para conexión a Internet
/**************************************************************/
typedef enum    {
	fsmCellular_init = 0,
	fsmCellular_desconectado,
	fsmCellular_conectado,
	fsmCellular_conectando,
	fsmCellular_noSeConecto
}fsmCellular_state_t;
static fsmCellular_state_t fsmState;
static fsmCellular_state_t fsmState_previous;

static bool stateIn = true;
static bool stateOut = false;
static SoftTimer_t fsmTimer;

static void cellularFsm_gotoState (fsmCellular_state_t nextState);
/**************************************************************/




void cellularManager_init (struct usart_module* uart, bool catM, bool bands4g, uint8_t* pin) {
	internet_connected = CELLULAR_MANAGER_INTERNET_DISCONNECTED;
	bg96_uart_module = uart;
	catMEnabled = catM;
	bands4gEnabled = bands4g;
	simPin = pin;
	
	
	bg96_init(&bg96_uart_module, catMEnabled, bands4gEnabled, simPin);
	bg96_registerModuleCallback(bg96Callback);
	bg96_initModule();
	
	
	pdpContextId = bg96_getPdpContext("","","");
	sslContextId = bg96_getSslContext("ca-cert.pem");
}


void cellularManager_deinit (void) {
	
}


uint32_t cellularManager_isInternetConnected (void) {
	
}


void cellularManager_handler (void) {
	bg96_handler();
	
	switch(fsmState) {
		case fsmCellular_init:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

				// 2 segundos de demora esperando a ver si llega el inicio de la prueba de fábrica
				softTimer_init(&fsmTimer, 2000);
            }

            //**********************************************************************************************
			if (softTimer_expired(&fsmTimer)) {
				cellularFsm_gotoState(fsmCellular_desconectado);
			}

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmCellular_desconectado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

            }

            //**********************************************************************************************

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmCellular_conectando:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

            }

            //**********************************************************************************************

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmCellular_conectado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

            }

            //**********************************************************************************************

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmCellular_noSeConecto:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

            }

            //**********************************************************************************************

            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
	}
}




void bg96Callback (bg96_module_events evt, void* payload) {
	switch(evt) {
		case bg96_module_stateChange:
		{
			bg96_moduleStateChangePayload* stateChange = (bg96_moduleStateChangePayload*)payload;
		
			switch (stateChange->state) {
				case module_states_change_registred:
					bg96_openPdpContext(pdpContextId);
					break;
			
				case module_states_change_unregistred:
					internet_connected = CELLULAR_MANAGER_INTERNET_DISCONNECTED;
					break;
			}
			break;
		}
	
		case bg96_module_contextStateChange:
		{
			bg96_contextStateChangePayload* stateChange = (bg96_contextStateChangePayload*)payload;
		
			switch (stateChange->state) {
			
				case module_states_context_opened:
					if (stateChange->isSsl == false && stateChange->contextId == pdpContextId)
						bg96_openSslContext(sslContextId);
					else if (stateChange->isSsl == true && stateChange->contextId == pdpContextId)
						internet_connected = CELLULAR_MANAGER_INTERNET_CONNECTED;
					break;
			
				case module_states_context_closed:
					internet_connected = CELLULAR_MANAGER_INTERNET_DISCONNECTED;
					break;
			}
		
			break;
		}
	
		case bg96_module_error:
		{
			bg96_moduleErrorPayload* moduleError = (bg96_moduleErrorPayload*)payload;
		
			switch(moduleError->error) {
				case module_error_sim_with_pin_but_device_without_pin:
					//blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
					//blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_SIM_PIN_DEVICE_NO, BLINKING_ERROR_SIM_PIN_DEVICE_NO_LEN, BLINKING_ERROR_SIM_PIN_DEVICE_NO_BASE);
					break;
			
				case module_error_sim_without_pin_but_device_with_pin:
					//blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
					//blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_SIM_NO_DEVICE_PIN, BLINKING_ERROR_SIM_NO_DEVICE_PIN_LEN, BLINKING_ERROR_SIM_NO_DEVICE_PIN_BASE);
					break;
			
				case module_error_sim_and_device_different_pin:
					//blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
					//blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_SIM_PIN_DEVICE_PIN, BLINKING_ERROR_SIM_PIN_DEVICE_PIN_LEN, BLINKING_ERROR_SIM_PIN_DEVICE_PIN_BASE);
					break;
			
				case module_error_sim_error:
					//blinkingLed_setPattern(&blinkingLedVerde, BLINKING_NO_PATTERN, BLINKING_NO_PATTERN_LEN, BLINKING_NO_PATTERN_BASE);
					//blinkingLed_setPattern(&blinkingLedRojo, BLINKING_ERROR_SIM_ERROR, BLINKING_ERROR_SIM_ERROR_LEN, BLINKING_ERROR_SIM_ERROR_BASE);
					break;
			
				case module_error_cant_open_pdp_context:
					//bg96_resetModule();
					break;
			
				case module_error_cant_close_pdp_context:
					//bg96_resetModule();
					break;
			
				case module_error_cant_open_ssl_context:
					//bg96_resetModule();
					break;
			}
		
			break;
		}
	}
}


void cellularFsm_gotoState (fsmCellular_state_t nextState) {
	fsmState_previous = fsmState;
	fsmState = nextState;
	
	stateIn = false;
	stateOut = true;
}