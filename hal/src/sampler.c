#include "hal/sampler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define NUM_SAMPLES 1000

#define A2D_FILE_VOLTAGE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095
#define EXPONENTIAL_SMOOTHING_PREV_WEIGHT 0.999

static bool isRunning = true;
static double historyBuffer[NUM_SAMPLES] = {0};
static double currentBuffer[NUM_SAMPLES] = {0};
static bool is_initialized = false;
static long long numSamplesTaken = 0;
static int historySize = 0;
static int currentSize = 0;
static double avgLightReading = 0;

static void sleepForMs(long long delayInMs);
static long long getTimeInMs(void);
static void* sampleLightLevels();
static int getVoltage1Reading();
static void* swapHistoryPeriodic();
static double a2dToVoltage(int a2dReading);

static pthread_t threads[2];
pthread_mutex_t mutexHistory;

// Begin/end the background thread which samples light levels.
void Sampler_init(void)
{
    assert(!is_initialized);
    is_initialized = true;

    pthread_mutex_init(&mutexHistory, NULL);
    avgLightReading = a2dToVoltage(getVoltage1Reading());

    //start the thread - will sample light level every 1ms
    pthread_create(&threads[0], NULL, sampleLightLevels, NULL);
    pthread_create(&threads[1], NULL, swapHistoryPeriodic, NULL);
    printf("init complete\n");
    // pthread_join(*samplerThread, NULL);
}

void Sampler_cleanup(void)
{
    printf("sampler cleanup start\n");
    //free memory, close files
    assert(is_initialized);
    is_initialized = false;
    isRunning = false;
    //join thread
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    printf("sampler cleanup done\n");
}

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void Sampler_moveCurrentDataToHistory(void)
{
    assert(is_initialized);
    pthread_mutex_lock(&mutexHistory);
    memcpy(historyBuffer, currentBuffer, sizeof(double) * NUM_SAMPLES);
    historySize = currentSize;
    currentSize = 0;
    pthread_mutex_unlock(&mutexHistory);
    // printf("history size: %d\n", historySize);
}

// Get the number of samples collected during the previous complete second.
int Sampler_getHistorySize(void)
{
    assert(is_initialized);
    return historySize;
}

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* Sampler_getHistory(int* size) //TODO - check previously int *size parameter - change to static?
{
    assert(is_initialized);
    double* historyCopy = (double*)malloc((*size) * sizeof(double));
    memcpy(historyCopy, historyBuffer, sizeof(double) * (*size));
    return historyCopy;
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

//TODO - put in its own module
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

static void* sampleLightLevels()
{
    while (isRunning) {
        pthread_mutex_lock(&mutexHistory);
        double voltageReading = a2dToVoltage(getVoltage1Reading());
        currentBuffer[currentSize] = voltageReading;
        numSamplesTaken++;
        currentSize++;
        avgLightReading = (EXPONENTIAL_SMOOTHING_PREV_WEIGHT * avgLightReading) + ((1 - EXPONENTIAL_SMOOTHING_PREV_WEIGHT) * voltageReading);
        pthread_mutex_unlock(&mutexHistory);
        sleepForMs(1);
    }
    pthread_exit(NULL);
}

static void* swapHistoryPeriodic()
{
    long long startTime = getTimeInMs();
    while (isRunning) {
        long long currentTime = getTimeInMs();
        if (currentTime - startTime >= 1000){
            Sampler_moveCurrentDataToHistory();
            startTime = currentTime;
        }
    }
    pthread_exit(NULL);
}

// From A2D guide - to be modified for error handling
static int getVoltage1Reading()
{
    // Open file
    FILE *f = fopen(A2D_FILE_VOLTAGE1, "r");
    if (!f) {
        printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    // Get reading
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0) {
    printf("ERROR: Unable to read values from voltage input file.\n");
        exit(-1);
    }
    // Close file
    fclose(f);
    return a2dReading;
}

//TODO - put in its own module
static long long getTimeInMs(void){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000
    + nanoSeconds / 1000000;
    return milliSeconds;
}

static double a2dToVoltage(int a2dReading)
{
    return ((double)a2dReading / (double)A2D_MAX_READING) * (double)A2D_VOLTAGE_REF_V;
}

pthread_mutex_t* Sampler_getHistoryMutexRef(void)
{
    return &mutexHistory;
}