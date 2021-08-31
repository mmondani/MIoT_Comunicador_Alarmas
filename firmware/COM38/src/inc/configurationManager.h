#ifndef CONFIGURATIONMANAGER_H_
#define CONFIGURATIONMANAGER_H_


#include "asf.h"
#include "imClient.h"



void configurationManager_init (void);

bool configurationManager_analizarIm (imMessage_t* msg);
void configurationManager_armarGetConfiguracionTiempo (void);
void configurationManager_armarGetConfiguracionRobo (void);
void configurationManager_armarGetNombreCom (void);
void configurationManager_armarGetConfiguracionMonitoreo (void);
void configurationManager_armarGetSocketBroker (void);

void configurationManager_armarEventoConfiguracionTiempo (void);
void configurationManager_armarEventoConfiguracionRobo (void);
void configurationManager_armarEventoNombreCom (void);
void configurationManager_armarEventoConfiguracionMonitoreo (void);
void configurationManager_armarEventoSocketBroker (void);

#endif /* CONFIGURATIONMANAGER_H_ */