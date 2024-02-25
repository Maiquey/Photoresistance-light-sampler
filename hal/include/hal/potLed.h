// potLed.h
// Module to drive the LED using PWM and POT readings
//

#ifndef _POT_LED_H_
#define _POT_LED_H_

#include <stdbool.h>

// Begin/end the background thread which drives the LED with PWM
// Also sets necessary config pins
void PotLed_init(void);
void PotLed_cleanup(void);

// Interface function for reading the potentiometer
int PotLed_getPOTReading(void);

// Interface function for getting the frequency of the LED
int PotLed_getFrequency(void);


#endif