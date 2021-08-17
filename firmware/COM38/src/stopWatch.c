#include "../inc/stopWatch.h"

#define STATE_CLEAR		0
#define STATE_STOPPED	1
#define STATE_RUNNING	2


void stopWatch_start (stopWatch_t* watch)
{
	watch->start = softTimer_getTicks();
	watch->stop = 0;
	watch->state = STATE_RUNNING;
}


void stopWatch_stop (stopWatch_t* watch)
{
	watch->stop = softTimer_getTicks();
	watch->state = STATE_STOPPED;
}


void stopWatch_clear (stopWatch_t* watch)
{
	watch->start = 0;
	watch->stop = 0;
	watch->state = STATE_CLEAR;
}


uint32_t stopWatch_elapsedTime (stopWatch_t* watch)
{
	if (watch->state == STATE_STOPPED) {
		return (watch->stop - watch->start);
	}
	else {
		return 0;
	}
}

uint32_t stopWatch_currentElapsedTime (stopWatch_t* watch)
{
	if (watch->state == STATE_RUNNING) {
		return (softTimer_getTicks() - watch->start);
	}
	else {
		return 0;
	}
}