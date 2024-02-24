// sigDisplay.h
// Module to drive the LED using PWM and POT readings
//

#ifndef _SIG_DISPLAY_H_
#define _SIG_DISPLAY_H_

#include <stdbool.h>

// Begin/end the background thread which samples light levels.
void SigDisplay_init(void);
void SigDisplay_cleanup(void);

void SigDisplay_setNumber(int newValue);

#endif