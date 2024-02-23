#include "hal/button.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;

void button_init(void) 
{
    printf("Button - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    srand(time(0));
}

bool button_is_button_pressed(void)
{
    assert(is_initialized);

    // Read the button state (no, don't just randomly get a value!)
    bool is_pressed = rand() % 2;
    return is_pressed;

}
void button_cleanup(void)
{
    // Free any memory, close files, ...
    printf("Button - Cleanup\n");
    assert(is_initialized);
    is_initialized = false;
}