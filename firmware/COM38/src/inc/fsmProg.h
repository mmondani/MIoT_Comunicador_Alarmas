#ifndef FSMPROG_H_
#define FSMPROG_H_

#include "asf.h"
#include "inc/imClient.h"


void fsmProg_init(void);
void fsmProg_handler(void);

bool fsmProg_analizarIm (imMessage_t* msg);

void fsmProg_analizarMpxh (uint8_t dataH, uint8_t dataL, uint8_t layer, uint8_t nbits);
void fsmProg_timers1s_handler (void);

bool fsmProg_enProgramacion (void);



#endif