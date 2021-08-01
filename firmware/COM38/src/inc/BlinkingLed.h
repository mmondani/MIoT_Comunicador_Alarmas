#ifndef BLINKINGLED_H_
#define BLINKINGLED_H_

#include "../inc/softTimers.h"
#include "BlinkingPatterns.h"



typedef struct
{
	uint8_t pin;
	uint32_t pattern;
	uint32_t pattenLen;
	uint32_t slot;
	SoftTimer_t baseTimer;
}blinkingLed_t;


void blinkingLed_init (blinkingLed_t* led, uint8_t pin);
void blinkingLed_setPattern (blinkingLed_t* led, uint32_t pattern, uint32_t len, uint32_t timeBase);
void blinkingLed_handler (blinkingLed_t* led);

#endif /* BLINKINGLED_H_ */