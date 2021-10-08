#ifndef ALARMMONITOR_H_
#define ALARMMONITOR_H_

#include "asf.h"
#include "imClient.h"


#define BATERIA_BIEN		0
#define BATERIA_DUDOSA		1
#define BATERIA_BAJA		2

#define ESTADO_ALARMA_DESACTIVADA	1
#define ESTADO_ALARMA_ACTIVADA		2
#define ESTADO_ALARMA_ACT_ESTOY		3
#define ESTADO_ALARMA_ACT_ME_VOY	4
#define ESTADO_ALARMA_ACT_PARCIAL	5
#define ESTADO_ALARMA_PROGRAMACION	6

#define DISPARO_ROBO				1
#define DISPARO_ASALTO				2
#define DISPARO_INCENDIO			3
#define DISPARO_INCENDIO_MANUAL		4
#define DISPARO_TAMPER				5
#define DISPARO_EMERGENCIA_MEDICA	6
#define DISPARO_PANICO				7

#define TIPO_CENTRAL_NINGUNO		0
#define TIPO_CENTRAL_N4				1
#define TIPO_CENTRAL_N8				2
#define TIPO_CENTRAL_N16			3
#define TIPO_CENTRAL_N32			4



void alarmMonitor_init (void);
void alarmMonitor_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits);
uint8_t alarmMonitor_esDia (void);
uint8_t alarmMonitor_hayLineaTelefonica (void);
uint8_t alarmMonitor_hayTlcd (void);
uint8_t alarmMonitor_hayRed (void);
uint32_t alarmMonitor_estadoBateria (void);
uint8_t alarmMonitor_estadoMpxh (void);
uint8_t alarmMonitor_existeLayer (uint8_t layer);
uint8_t alarmMonitor_estadoCentral (uint8_t layer);
uint8_t alarmMonitor_cantidadZonasCentral (void);
uint8_t alarmMonitor_tipoAlarma (void);
bool alarmMonitor_hayPerifericoEnProgramacion (uint8_t layer);
void alarmMonitor_resetTimerPerifericoEnProgramacion (uint8_t layer);
void alarmMonitor_clearPerifericoEnProgramacion (uint8_t layer);
void alarmMonitor_timers1s_handler (void);
void alarmMonitor_timers1m_handler (void);
void alarmMonitor_timers1h_handler (void);

void alarmMonitor_setRetardo(uint8_t layer, uint8_t retardo);
uint8_t alarmMonitor_getRetardo(uint8_t layer);

bool alarmMonitor_analizarIm (imMessage_t* msg);
void alarmMonitor_determinarFinProcesamiento(void);
bool alarmMonitor_procesandoMensaje(void);

void alarmMonitor_armarGetEstado (uint8_t layer);
void alarmMonitor_armarEventEstado (uint8_t layer);
void alarmMonitor_armarEventoOpenClose (uint8_t layer);
void alarmMonitor_armarEventoRed (void);
void alarmMonitor_armarEventoBateria (void);
void alarmMonitor_armarEventoEstadoMpxh (void);
void alarmMonitor_armarGetSonandoReady (void);
void alarmMonitor_armarEventoSonandoReady (void);
void alarmMonitor_armarGetInclusion (uint8_t layer);
void alarmMonitor_armarEventoInclusion (uint8_t layer);
void alarmMonitor_armarGetMemoria (uint8_t layer);
void alarmMonitor_armarEventoMemoria (uint8_t layer);
void alarmMonitor_armarGetEstadoZonas (uint8_t layer);
void alarmMonitor_armarEventoEstadoZonas (uint8_t layer);
void alarmMonitor_armarEventoDisparo (uint8_t layer, uint8_t disparo);





#endif