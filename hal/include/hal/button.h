// Sample button module 
// Part of the Hardware Abstraction Layer (HAL) 

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>

void button_init(void);
bool button_is_button_pressed(void);
void button_cleanup(void);

#endif