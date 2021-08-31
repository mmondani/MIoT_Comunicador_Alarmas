#include "inc/MessagePoolManager.h"


imMessage_t messagePool[MESSAGE_POOL_POOL_LEN];
imMessage_t* outputQueue[MESSAGE_POOL_POOL_LEN];
imMessage_t* inputQueues[MESSAGE_POOL_NUMBER_OF_FLOWS][MESSAGE_POOL_POOL_LEN];



void messagePool_init (void)
{
	for (int i = 0; i < MESSAGE_POOL_POOL_LEN; i++) {
		messagePool[i].free = 1;
		messagePool[i].payload_ptr = 0;
	}
	
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN; i++) {
		outputQueue[i] = NULL;
	}
	
	for (int i = 0; i < MESSAGE_POOL_NUMBER_OF_FLOWS; i++) {
		for (int j = 0; j < MESSAGE_POOL_INPUT_QUEUE_LEN; j++) {
			inputQueues[i][j] = NULL;
		}
		
	}
}


imMessage_t* messagePool_getFreeSlot(void)
{
	imMessage_t* ret = NULL;
	
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN; i++) {
		if (messagePool[i].free == 1) {
			messagePool[i].free = 0;
			
			ret = &(messagePool[i]);
			
			break;
		}
	}
	
	return ret;
}


void messagePool_releaseSlot (imMessage_t* msg)
{
	if (msg != NULL) {
		msg->payload_ptr = 0;
		msg->free = 1;
	}
}


uint32_t messagePool_pendingOutputMessage (void)
{
	uint32_t ret = 0;
	
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN; i++) {
		if (outputQueue[i] != NULL) {
			ret = 1;
			break;
		}
	}
	
	return ret;
}


void messagePool_pushOutputQueue (imMessage_t* msg)
{
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN; i++) {
		if (outputQueue[i] == NULL) {
			outputQueue[i] = msg;
			break;
		}
	}
}


imMessage_t* messagePool_peekOutputQueue (void)
{
	return (outputQueue[0]);
}


void messagePool_popOutputQueue (void)
{
	messagePool_releaseSlot(outputQueue[0]);
	
	
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN - 1; i++) {
		outputQueue[i] = outputQueue[i+1];
	}
	
	outputQueue[MESSAGE_POOL_OUTPUT_QUEUE_LEN - 1] = NULL;
}


void messagePool_flushOutputQueue (void)
{
	for (int i = 0; i < MESSAGE_POOL_OUTPUT_QUEUE_LEN; i++) {
		outputQueue[i] = NULL;
	}
}



uint32_t messagePool_pendingInputMessage (uint32_t flow)
{
	uint32_t ret = 0;
	
	if (flow < MESSAGE_POOL_NUMBER_OF_FLOWS) {
		
		for (int i = 0; i < MESSAGE_POOL_INPUT_QUEUE_LEN; i++) {
			if (inputQueues[flow][i] != NULL) {
				ret = 1;
				break;
			}
		}
	}

	
	return ret;
}


void messagePool_pushInputQueue (uint32_t flow, imMessage_t* msg)
{
	uint32_t ret = 0;
	
	if (flow < MESSAGE_POOL_NUMBER_OF_FLOWS) {
		for (int i = 0; i < MESSAGE_POOL_INPUT_QUEUE_LEN; i++) {
			if (inputQueues[flow][i] == NULL) {
				inputQueues[flow][i] = msg;
				break;
			}
		}
	}
}


imMessage_t* messagePool_peekInputQueue (uint32_t flow)
{
	if (flow < MESSAGE_POOL_NUMBER_OF_FLOWS) {
		return (inputQueues[flow][0]);
	}
	
	return NULL;
}


void messagePool_popInputQueue (uint32_t flow)
{
	if (flow < MESSAGE_POOL_NUMBER_OF_FLOWS) {
		messagePool_releaseSlot(inputQueues[flow][0]);
		
		for (int i = 0; i < MESSAGE_POOL_INPUT_QUEUE_LEN - 1; i++) {
			inputQueues[flow][i] = inputQueues[flow][i+1];
		}
		
		inputQueues[flow][MESSAGE_POOL_OUTPUT_QUEUE_LEN - 1] = NULL;
	}
}