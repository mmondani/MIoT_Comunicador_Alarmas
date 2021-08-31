#ifndef DATETIME_H_
#define DATETIME_H_

#include "asf.h"
#include "imClient.h"
#include "DateTime_t.h"


void dateTime_init (void);
void dateTime_now (dateTime_t* dateTime);
void dateTime_setSincronizarInternet (uint8_t sincronizar);
uint8_t dateTime_getSincronizarInternet (void);
void dateTime_setRegionCode (uint8_t code);
uint8_t dateTime_getRegionCode (void);
void dateTime_setTimeDate (dateTime_t* newTimeDate);
void dateTime_setTime (dateTime_t* newTime);
void dateTime_setDate (dateTime_t* newDate);
void dateTime_getTimeDate (dateTime_t* timeDate);
void dateTime_handler (void);
void dateTime_timeKeepingHandler (void);
bool dateTime_hasValidTime(void);
bool dateTime_soyPatron(void);

void dateTime_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits);
void dateTime_fsmPatronTiempo (void);

void dateTime_timers1s_handler (void);
void dateTime_timers1h_handler (void);

bool dateTime_analizarIm (imMessage_t* msg);
void dateTime_armarGetFecha (void);
void dateTime_armarGetHora (void);
void dateTime_armarGetFechaHora (void);
void dateTime_determinarFinProcesamiento(void);
bool dateTime_procesandoMensaje(void);

void dateTime_armarEventoFecha (void);
void dateTime_armarEventoHora (void);
void dateTime_armarEventoFechaHora (void);


#endif /* DATETIME_H_ */