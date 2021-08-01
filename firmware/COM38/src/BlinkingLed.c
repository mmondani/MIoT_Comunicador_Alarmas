#include "asf.h"
#include "../inc/BlinkingLed.h"




void blinkingLed_init (blinkingLed_t* led, uint8_t pin)
{
	led->pin = pin;
	led->pattern = BLINKING_NO_PATTERN;
	led->pattenLen = BLINKING_NO_PATTERN_LEN;
	led->slot = 0x00000001;
	softTimer_init(&(led->baseTimer), BLINKING_NO_PATTERN_BASE);
}



void blinkingLed_setPattern (blinkingLed_t* led, uint32_t pattern, uint32_t len, uint32_t timeBase)
{
	led->pattern = pattern;
	led->pattenLen = len;
	led->slot = 0x00000001;
	softTimer_init(&(led->baseTimer), timeBase);
}

void blinkingLed_handler (blinkingLed_t* led)
{
	if (softTimer_expired(&(led->baseTimer))) {
		softTimer_restart(&(led->baseTimer));
		
		//if (led->pattern != BLINKING_NO_PATTERN) {
			if ((led->pattern & led->slot) != 0) {
				port_pin_set_output_level(led->pin, true);
			}
			else {
				port_pin_set_output_level(led->pin, false);
			}
		
			led->slot = led->slot << 1;
		
			if (led->slot > led->pattenLen) {
				led->slot = 0x00000001;
			}
		//}
	}
}