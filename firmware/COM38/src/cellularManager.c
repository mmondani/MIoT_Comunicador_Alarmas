#include "inc/cellularManager.h"
#include "inc/softTimers.h"
#include "inc/BG96.h"
#include "inc/mainFsm.h"
#include "inc/BlinkingLed.h"
#include "inc/BlinkingPatterns.h"


static uint32_t internet_connected;
static cellularManager_errors moduleError = cellularManager_no_error;
static struct usart_module* bg96_uart_module;
static bool catMEnabled;
static bool bands4gEnabled;
static uint8_t* simPin;

static int32_t pdpContextId;
static int32_t sslContextId;

static blinkingLed_t blinkingLedVerde;

static void bg96Callback (bg96_module_events evt, void* payload);


/**************************************************************/
// FSM para conexión a Internet
/**************************************************************/
typedef enum    {
	fsmCellular_init = 0,
	fsmCellular_desconectado,
	fsmCellular_registrado,
	fsmCellular_conectado,
	fsmCellular_error
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
	
	bg96_registerModuleCallback(bg96Callback);
	pdpContextId = bg96_getPdpContext("","","");
	sslContextId = bg96_getSslContext("ca-cert.pem");
	/*
	bg96_init(bg96_uart_module, catMEnabled, bands4gEnabled, simPin);
	bg96_registerModuleCallback(bg96Callback);
	bg96_initModule();
	
	
	pdpContextId = bg96_getPdpContext("","","");
	sslContextId = bg96_getSslContext("ca-cert.pem");
	*/
	
	blinkingLed_init(&blinkingLedVerde, LED_CELULAR_PIN);
	
	cellularFsm_gotoState(fsmCellular_init);
}


void cellularManager_deinit (void) {
	// TODO
}


uint32_t cellularManager_isInternetConnected (void) {
	return internet_connected;
}


cellularManager_errors cellularManager_getError (void) {
	return moduleError;
}


void cellularManager_handler (void) {
	
	if (!mainFsm_estaEnPruebaFabrica()) {
		blinkingLed_handler(&blinkingLedVerde);
	}
	
	switch(fsmState) {
		case fsmCellular_init:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

				// 2 segundos de demora esperando a ver si llega el inicio de la prueba de fábrica
				softTimer_init(&fsmTimer, 2000);
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_INIT, BLINKING_CELULAR_INIT_LEN, BLINKING_CELULAR_INIT_BASE);
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

				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_DESCONECTADO, BLINKING_CELULAR_DESCONECTADO_LEN, BLINKING_CELULAR_DESCONECTADO_BASE);
            }

            //**********************************************************************************************
			if (bg96_isRegistred()) 
				cellularFsm_gotoState(fsmCellular_registrado);
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
            }
			
			break;
			
		
		case fsmCellular_registrado:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;

				if (!bg96_isPdpContextOpened(pdpContextId))
					bg96_openPdpContext(pdpContextId);
					
				if (!bg96_isSslContextOpened(sslContextId))
					bg96_openSslContext(sslContextId);
					
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_REGISTRADO, BLINKING_CELULAR_REGISTRADO_LEN, BLINKING_CELULAR_REGISTRADO_BASE);
            }

            //**********************************************************************************************
			if (bg96_isPdpContextOpened(pdpContextId) && bg96_isSslContextOpened(sslContextId))
				cellularFsm_gotoState(fsmCellular_conectado);
			else if (!bg96_isRegistred())
				cellularFsm_gotoState(fsmCellular_desconectado);
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
				
				internet_connected = CELLULAR_MANAGER_INTERNET_CONNECTED;
				
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_CONECTADO, BLINKING_CELULAR_CONECTADO_LEN, BLINKING_CELULAR_CONECTADO_BASE);
            }

            //**********************************************************************************************
			if (!bg96_isRegistred()) 
				cellularFsm_gotoState(fsmCellular_desconectado);
			else if (!bg96_isPdpContextOpened(pdpContextId))
				cellularFsm_gotoState(fsmCellular_registrado);
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				internet_connected = CELLULAR_MANAGER_INTERNET_DISCONNECTED;
            }
			
			break;
			
			
		case fsmCellular_error:
			if (stateIn)
            {
                stateIn = false;
                stateOut = false;
            }

            //**********************************************************************************************
			if (bg96_isRegistred()) 
				cellularFsm_gotoState(fsmCellular_registrado);
				
			if (moduleError == cellularManager_error_sim_pin_device_no_pin)
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_NO, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_NO_LEN, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_NO_BASE);
			else if (moduleError == cellularManager_error_sim_no_pin_device_pin)
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_ERROR_SIM_NO_DEVICE_PIN, BLINKING_CELULAR_ERROR_SIM_NO_DEVICE_PIN_LEN, BLINKING_CELULAR_ERROR_SIM_NO_DEVICE_PIN_BASE);
			else if (moduleError == cellularManager_error_sim_and_device_different_pins)
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_PIN, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_PIN_LEN, BLINKING_CELULAR_ERROR_SIM_PIN_DEVICE_PIN_BASE);
			else if (moduleError == cellularManager_error_sim_error)
				blinkingLed_setPattern(&blinkingLedVerde, BLINKING_CELULAR_ERROR_SIM_ERROR, BLINKING_CELULAR_ERROR_SIM_ERROR_LEN, BLINKING_CELULAR_ERROR_SIM_ERROR_BASE);
            //**********************************************************************************************
            if (stateOut)
            {
                stateIn = true;
                stateOut = false;
				
				moduleError = cellularManager_no_error;
            }
			
			break;
	}
}


void cellularManager_reset (void) {
	bg96_resetModule();
	
	internet_connected = CELLULAR_MANAGER_INTERNET_DISCONNECTED;
	
	cellularFsm_gotoState(fsmCellular_desconectado);
}




void bg96Callback (bg96_module_events evt, void* payload) {
	switch(evt) {
		case bg96_module_stateChange:
		{
			bg96_moduleStateChangePayload* stateChange = (bg96_moduleStateChangePayload*)payload;
		
			switch (stateChange->state) {
				case module_states_change_registred:
					break;
			
				case module_states_change_unregistred:
					break;
			}
			break;
		}
	
		case bg96_module_contextStateChange:
		{
			bg96_contextStateChangePayload* stateChange = (bg96_contextStateChangePayload*)payload;
		
			switch (stateChange->state) {
			
				case module_states_context_opened:
					break;
			
				case module_states_context_closed:
					break;
			}
		
			break;
		}
	
		case bg96_module_error:
		{
			bg96_moduleErrorPayload* moduleError = (bg96_moduleErrorPayload*)payload;
		
			switch(moduleError->error) {
				case module_error_sim_with_pin_but_device_without_pin:
					moduleError = cellularManager_error_sim_pin_device_no_pin;
					cellularFsm_gotoState(fsmCellular_error);
					break;
			
				case module_error_sim_without_pin_but_device_with_pin:
					moduleError = cellularManager_error_sim_no_pin_device_pin;
					cellularFsm_gotoState(fsmCellular_error);
					break;
			
				case module_error_sim_and_device_different_pin:
					moduleError = cellularManager_error_sim_and_device_different_pins;
					cellularFsm_gotoState(fsmCellular_error);
					break;
			
				case module_error_sim_error:
					moduleError = cellularManager_error_sim_error;
					cellularFsm_gotoState(fsmCellular_error);
					break;
			
				case module_error_cant_open_pdp_context:
					cellularManager_reset();
					break;
			
				case module_error_cant_close_pdp_context:
					cellularManager_reset();
					break;
			
				case module_error_cant_open_ssl_context:
					cellularManager_reset();
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


int32_t cellularFsm_getPdpContext (void) {
	return pdpContextId;
}

int32_t cellularFsm_getSslContext (void) {
	return sslContextId;
}