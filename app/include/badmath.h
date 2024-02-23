// Sample application module
// Normally, you might create a module such as this for each module in
// the application that does some high-level task; perhaps has its own thread.

#ifndef _BADMATH_H_
#define _BADMATH_H_

void badmath_init(void);
short badmath_factorial(int n);
void badmath_cleanup(void);

#endif