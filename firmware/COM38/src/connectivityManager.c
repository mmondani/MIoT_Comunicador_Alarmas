#include "inc/connectivityManager.h"
#include "inc/WiFiManager.h"
#include "inc/cellularManager.h"
#include "inc/softTimers.h"

enum{
	FSM_INIT = 0,
	FSM_WAITING,
	FSM_HAY_WIFI,
	FSM_HAY_CELULAR
};
static uint32_t fsmState;

static bool error_wifi;
static bool error_celular;

static SoftTimer_t timerErrorWifi;
static SoftTimer_t timerErrorCelular;


static void (*changeCallback)(connectivityManager_events, connectivityManager_interfaces) = NULL;



void connectivityManager_init (void) {
	fsmState = FSM_INIT;
}


void connectivityManager_handler (void) {
	
	if (softTimer_expired(&timerErrorWifi))
	error_wifi = false;
	
	if (softTimer_expired(&timerErrorCelular))
	error_celular = false;
	
	
	switch(fsmState) {
		case FSM_INIT:
		fsmState = FSM_WAITING;
		break;
		
		
		case FSM_WAITING:
		if (wifiManager_isWiFiConnected() && !error_wifi) {
			fsmState = FSM_HAY_WIFI;
			
			if (changeCallback != NULL)
			changeCallback(connectivityManager_event_change, connectivityManager_interface_wifi);
		}
		else if (cellularManager_isInternetConnected() && !error_celular) {
			fsmState = FSM_HAY_CELULAR;
			
			if (changeCallback != NULL)
			changeCallback(connectivityManager_event_change, connectivityManager_interface_cellular);
		}
		
		break;
		
		case FSM_HAY_WIFI:
		if (wifiManager_isWiFiConnected() == 0 || error_wifi) {
			if (cellularManager_isInternetConnected() && !error_celular) {
				fsmState = FSM_HAY_CELULAR;
				
				softTimer_init(&timerErrorWifi, 1000*60*15);
				
				if (changeCallback != NULL)
				changeCallback(connectivityManager_event_change, connectivityManager_interface_cellular);
			}
			else {
				fsmState = FSM_WAITING;
				
				softTimer_init(&timerErrorWifi, 1000*60*1);
				
				if (changeCallback != NULL)
				changeCallback(connectivityManager_event_change, connectivityManager_interface_none);
			}
		}
		break;
		
		case FSM_HAY_CELULAR:
		if (wifiManager_isWiFiConnected() && !error_wifi) {
			fsmState = FSM_HAY_WIFI;
			
			if (changeCallback != NULL)
			changeCallback(connectivityManager_event_change, connectivityManager_interface_wifi);
		}
		else if (cellularManager_isInternetConnected() == 0 || error_celular) {
			fsmState = FSM_WAITING;
			
			softTimer_init(&timerErrorCelular, 1000*60*1);
			
			if (changeCallback != NULL)
			changeCallback(connectivityManager_event_change, connectivityManager_interface_none);
			
		}
		break;
	}
}


void connectivityManager_setChangeCallback (void (*cb)(connectivityManager_events, connectivityManager_interfaces)) {
	changeCallback = cb;
}


void connectivityManager_setErrorWifi (void) {
	error_wifi = true;
}



void connectivityManager_setErrorCelular (void) {
	error_celular = true;
}
