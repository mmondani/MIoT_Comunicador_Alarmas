#ifndef NODESMANAGER_H_
#define NODESMANAGER_H_

#include "asf.h"
#include "imClient.h"


void nodesManager_init (void);
void nodesManager_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits);
void nodesManager_handler (void);
void nodesManager_timers1s_handler (void);
void nodesManager_timers1m_handler (void);
bool nodesManager_hayQueMandarPorMpxh (void);
void nodesManager_mandarPorMpxh (void);

bool nodesManager_analizarIm (imMessage_t* msg);
void nodesManager_determinarFinProcesamiento(void);
bool nodesManager_procesandoMensaje(void);

#endif