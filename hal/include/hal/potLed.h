// potLed.h
// Module to drive the LED using PWM and POT readings
//

#ifndef _POT_LED_H_
#define _POT_LED_H_

#include <stdbool.h>

// Begin/end the background thread which samples light levels.
void PotLed_init(void);
void PotLed_cleanup(void);

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
int PotLed_getPOTReading(void);

// Get the number of samples collected during the previous complete second.
int PotLed_getFrequency(void);


#endif