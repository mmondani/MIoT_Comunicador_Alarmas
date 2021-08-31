
#ifndef IMCLIENT_H_
#define IMCLIENT_H_

#include "MessagePoolManager.h"


#define BUFFER_IN_N_MESSAGES	5
#define BUFFER_OUT_N_MESSAGES	5
#define TIMER_KEEPALIVE			20000		// en milisegundos
#define TIMER_BUFFER			10*60*1000





void imClient_init (uint8_t* id);
void imClient_handler (void);
uint32_t imClient_isClientConnected (void);
void imClient_resetConnection (void);

imMessage_t* imClient_getFreeMessageSlot (void);
void imClient_releaseMessageSlot (imMessage_t* msg);

uint32_t imClient_putPayloadByte (imMessage_t* msg, uint8_t data);
uint32_t imClient_putPayloadBytes (imMessage_t* msg, uint8_t* data, uint32_t len);
void imClient_send (imMessage_t* msg, uint8_t* to, uint32_t flow, uint8_t cmd, uint16_t reg, uint8_t part, uint8_t qos);

uint32_t imClient_messagePendingToRead (uint32_t flow);
imMessage_t* imClient_getMessageToRead (uint32_t flow);
void imClient_removeMessageToRead (uint32_t flow);

#endif /* IMCLIENT_H_ */