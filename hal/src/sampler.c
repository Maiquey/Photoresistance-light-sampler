#include "hal/sampler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define NUM_SAMPLES 1000

static int historyBuffer[NUM_SAMPLES] = {0};
static int currentBuffer[NUM_SAMPLES] = {0};
static bool is_initialized = false;
static long long numSamplesTaken = 0;

// Begin/end the background thread which samples light levels.
void Sampler_init(void)
{
    assert(!is_initialized);
    is_initialized = true;

    //start the thread - will sample light level every 1ms
}

void Sampler_cleanup(void)
{
    //free memory, close files
    assert(is_initialized);
    is_initialized = false;
    //join thread
}

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void Sampler_moveCurrentDataToHistory(void)
{
    assert(is_initialized);
}

// Get the number of samples collected during the previous complete second.
int Sampler_getHistorySize(void)
{
    assert(is_initialized);
    return 0;
}

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* Sampler_getHistory(int *size)
{
    assert(is_initialized);
    return 0;
}

// Get the average light level (not tied to the history).
double Sampler_getAverageReading(void)
{
    assert(is_initialized);
    return 0;
}

// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void)
{
    assert(is_initialized);
    return numSamplesTaken;
}

static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

static void sampleLightLevels()
{

}