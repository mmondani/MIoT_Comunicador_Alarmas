#include "../inc/ringBuffer.h"


void ringBuffer_init (ringBuffer_t* rb, uint8_t* buffer, uint32_t len)
{
	rb->buffer = buffer;
	rb->len = len;
	rb->count = 0;
	rb->ptrIn = 0;
	rb->ptrOut = 0;
}


uint32_t ringBuffer_getFreeSpace (ringBuffer_t* rb)
{
	return (rb->len - rb->count);
}


uint32_t ringBuffer_getPending (ringBuffer_t* rb)
{
	return rb->count;
}


void ringBuffer_put (ringBuffer_t* rb, uint8_t data)
{
	if (ringBuffer_getFreeSpace(rb) > 0)
	{
		rb->buffer[rb->ptrIn] = data;
		
		rb->count ++;
		rb->ptrIn ++;
		if (rb->ptrIn >= rb->len)
		rb->ptrIn = 0;
	}
}


uint8_t ringBuffer_remove (ringBuffer_t* rb)
{
	uint8_t data = 0;
	
	if (ringBuffer_getPending(rb) > 0)
	{
		data = rb->buffer[rb->ptrOut];
		
		rb->count --;
		rb->ptrOut ++;
		if (rb->ptrOut >= rb->len)
		rb->ptrOut = 0;
	}
	
	return data;
}


uint8_t ringBuffer_peek (ringBuffer_t* rb)
{
	uint8_t data = 0;
	
	if (ringBuffer_getPending(rb) > 0)
	{
		data = rb->buffer[rb->ptrOut];
	}
	
	return data;
}


void ringBuffer_flush (ringBuffer_t* rb)
{
	rb->ptrOut = 0;
	rb->ptrIn = 0;
	rb->count = 0;
}