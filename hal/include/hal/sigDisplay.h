// sigDisplay.h
// Module to drive the 14-sig display based on the number of dips observed
//

#ifndef _SIG_DISPLAY_H_
#define _SIG_DISPLAY_H_

#include <stdbool.h>

// Begin/end the background thread which drives the 14-sig display
// Also sets necessary config pins
void SigDisplay_init(void);
void SigDisplay_cleanup(void);

// Setter function for the number displayed by the 14-sig display
void SigDisplay_setNumber(int newValue);

#endif