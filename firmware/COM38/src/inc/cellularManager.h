#ifndef CELLULARMANAGER_H_
#define CELLULARMANAGER_H_


#include "asf.h"


#define CELLULAR_MANAGER_INTERNET_DISCONNECTED		0
#define CELLULAR_MANAGER_INTERNET_CONNECTED			1

typedef enum {
	cellularManager_no_error = 0,
	cellularManager_error_sim_pin_device_no_pin ,
	cellularManager_error_sim_no_pin_device_pin,
	cellularManager_error_sim_and_device_different_pins,
	cellularManager_error_sim_error
}cellularManager_errors;


void cellularManager_init (struct usart_module* uart, bool catM, bool bands4g, uint8_t* pin);
void cellularManager_deinit (void);
uint32_t cellularManager_isInternetConnected (void);
cellularManager_errors cellularManager_getError (void);
void cellularManager_handler (void);
void cellularManager_reset (void);


#endif /* CELLULARMANAGER_H_ */