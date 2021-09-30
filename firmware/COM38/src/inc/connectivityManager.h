#ifndef CONNECTIVITYMANAGER_H_
#define CONNECTIVITYMANAGER_H_


typedef enum {
	connectivityManager_event_none = 0,
	connectivityManager_event_change,
	connectivityManager_event_disconnect_and_change
}connectivityManager_events;

typedef enum {
	connectivityManager_interface_none = 0,
	connectivityManager_interface_wifi,
	connectivityManager_interface_celullar
}connectivityManager_interfaces;


void connectivityManager_init (void);
void connectivityManager_handler (void);
void connectivityManager_setChangeCallback (void (*cb)(connectivityManager_events, connectivityManager_interfaces));
void connectivityManager_setErrorWifi (void);
void connectivityManager_setErrorCelular (void);



#endif /* CONNECTIVITYMANAGER_H_ */