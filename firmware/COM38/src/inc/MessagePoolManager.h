/*
 * MessagePoolManager.h
 *
 * Created: 26/07/2019 11:11:31
 *  Author: mmondani
 */ 


#ifndef MESSAGEPOOLMANAGER_H_
#define MESSAGEPOOLMANAGER_H_

#include "asf.h"
#include "DateTime_t.h"


#define		MESSAGE_POOL_POOL_LEN			100
#define		MESSAGE_POOL_OUTPUT_QUEUE_LEN	MESSAGE_POOL_POOL_LEN
#define		MESSAGE_POOL_INPUT_QUEUE_LEN	MESSAGE_POOL_POOL_LEN
#define		MESSAGE_POOL_NUMBER_OF_FLOWS	1
#define		MESSAGE_POOL_MESSAGE_MAX_LEN	100

#define		MESSAGE_POOL_FLOW_IM_CLIENT		1
#define		MESSAGE_POOL_FLOW_WCOM			2
#define		MESSAGE_POOL_FLOW_WCOM_STRINGS	3



typedef struct
{
	uint8_t free;			// Indica si está libre para ser usado o no
	uint32_t cmd;
	uint16_t reg;
	uint8_t part;
	dateTime_t timestamp;
	uint16_t len;
	uint8_t payload[MESSAGE_POOL_MESSAGE_MAX_LEN];
	uint32_t payload_ptr;
}imMessage_t;




void messagePool_init (void);

imMessage_t* messagePool_getFreeSlot(void);
void messagePool_releaseSlot (imMessage_t* msg);

uint32_t messagePool_pendingOutputMessage (void);
void messagePool_pushOutputQueue (imMessage_t* msg);
imMessage_t* messagePool_peekOutputQueue (void);
void messagePool_popOutputQueue (void);
void messagePool_flushOutputQueue (void);

uint32_t messagePool_pendingInputMessage (uint32_t flow);
void messagePool_pushInputQueue (uint32_t flow, imMessage_t* msg);
imMessage_t* messagePool_peekInputQueue (uint32_t flow);
void messagePool_popInputQueue (uint32_t flow);


#endif /* MESSAGEPOOLMANAGER_H_ */