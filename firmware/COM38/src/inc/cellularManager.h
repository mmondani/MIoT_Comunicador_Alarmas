#ifndef CELLULARMANAGER_H_
#define CELLULARMANAGER_H_


#include "asf.h"


#define CELLULAR_MANAGER_INTERNET_DISCONNECTED		0
#define CELLULAR_MANAGER_INTERNET_CONNECTED			1



void cellularManager_init (struct usart_module* uart, bool catM, bool bands4g, uint8_t* pin);
void cellularManager_deinit (void);
uint32_t cellularManager_isInternetConnected (void);
void cellularManager_handler (void);


#endif /* CELLULARMANAGER_H_ */