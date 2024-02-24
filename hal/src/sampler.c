#include "hal/sampler.h"
#include "hal/timing.h"
#include "hal/potLed.h"
#include "hal/sigDisplay.h"
#include "hal/periodTimer.h"
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
#define DIP_THRESHOLD 0.1
#define DIP_HYSTERESIS_THRESHOLD 0.07

static bool isRunning = true;
static double historyBuffer[NUM_SAMPLES] = {0};
static double currentBuffer[NUM_SAMPLES] = {0};
static bool is_initialized = false;
static long long numSamplesTaken = 0;
static int historySize = 0;
static int currentSize = 0;
static int historyDips = 0;
static double avgLightReading = 0;
static int numDips = 0;
static bool dipAllowed = true;
Period_statistics_t *pStats;

static void* sampleLightLevels();
static int getVoltage1Reading();
static void* swapHistoryPeriodic();
static double a2dToVoltage(int a2dReading);
static void outputDataToTerminal();

static pthread_t threads[2];
pthread_mutex_t mutexHistory;

// Begin/end the background thread which samples light levels.
void Sampler_init(void)
{
    assert(!is_initialized);
    is_initialized = true;

    pthread_mutex_init(&mutexHistory, NULL);
    pStats = (Period_statistics_t*)malloc(sizeof(Period_statistics_t));
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
    historyDips = numDips;
    currentSize = 0;
    numDips = 0;
    dipAllowed = true;
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

// Get the number of dips measured during the previous complete second.
int Sampler_getHistoryNumDips(void)
{
    return historyDips;
}


static void* sampleLightLevels()
{
    while (isRunning) {
        pthread_mutex_lock(&mutexHistory);
        double voltageReading = a2dToVoltage(getVoltage1Reading());
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        currentBuffer[currentSize] = voltageReading;
        if (dipAllowed){
            if (voltageReading <= avgLightReading - DIP_THRESHOLD){
                numDips++;
                dipAllowed = false;
            }
        } else {
            if (voltageReading >= avgLightReading - DIP_HYSTERESIS_THRESHOLD){
                dipAllowed = true;
            }
        }
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
            SigDisplay_setNumber(historyDips);
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
            outputDataToTerminal();
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


static double a2dToVoltage(int a2dReading)
{
    return ((double)a2dReading / (double)A2D_MAX_READING) * (double)A2D_VOLTAGE_REF_V;
}

pthread_mutex_t* Sampler_getHistoryMutexRef(void)
{
    return &mutexHistory;
}

static void outputDataToTerminal()
{
    printf("#Smpl/s = %3d    POT @ %4d => %2dHz   avg = %.3fV    dips =  %2d    Smpl ms[ %.3f,  %.3f] avg %.3f/%3d    \n",
            historySize, PotLed_getPOTReading(), PotLed_getFrequency(), avgLightReading, historyDips, pStats->minPeriodInMs, pStats->maxPeriodInMs, pStats->avgPeriodInMs, pStats->numSamples);
    int numSamples = 10;
    int scalingFactor = (historySize-1) / numSamples;
    if (historySize < numSamples) {
        numSamples = historySize;
        scalingFactor = 1;
    }
    for (int i = 0; i < numSamples; i++){
        if (i == 0){
            printf("  %d:%.3f", i*scalingFactor, historyBuffer[i]);
        } else {
            printf("  %3d:%.3f", i*scalingFactor, historyBuffer[i]);
        }
    }
    printf("\n");
}