#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include "softTimers.h"


typedef struct  
{
	uint32_t start;
	uint32_t stop;
	uint32_t state;
}stopWatch_t;


void stopWatch_start (stopWatch_t* watch);
void stopWatch_stop (stopWatch_t* watch);
void stopWatch_clear (stopWatch_t* watch);
uint32_t stopWatch_elapsedTime (stopWatch_t* watch);
uint32_t stopWatch_currentElapsedTime (stopWatch_t* watch);


#endif /* STOPWATCH_H_ */