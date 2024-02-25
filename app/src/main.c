// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "hal/periodTimer.h"
#include "hal/sampler.h"
#include "hal/potLed.h"
#include "hal/sigDisplay.h"
#include "network.h"

pthread_mutex_t mutexMain;
pthread_cond_t condVarFinished;

int main()
{
    // Initialize main mutex and cond var

    pthread_mutex_init(&mutexMain, NULL);
    pthread_cond_init(&condVarFinished, NULL);

    // Initialize all modules; HAL modules first

    Period_init();
    Sampler_init();
    PotLed_init();
    SigDisplay_init();
    Network_init(&condVarFinished);
    
    // Wait on condition variable until signalled by networking thread

    pthread_mutex_lock(&mutexMain);
    pthread_cond_wait(&condVarFinished, &mutexMain);
    pthread_mutex_unlock(&mutexMain);

    // Cleanup all modules (HAL modules last)

    Network_cleanup();
    Sampler_cleanup();
    PotLed_cleanup();
    SigDisplay_cleanup();
    Period_cleanup();
    
    // Free mutex and cond var 
    
    pthread_mutex_destroy(&mutexMain);
    pthread_cond_destroy(&condVarFinished);
    
}