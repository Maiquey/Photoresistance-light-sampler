#include "hal/potLed.h"
#include "hal/timing.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define FREQUENCY_DIV_FACTOR 40
#define A2D_FILE_VOLTAGE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095
#define NANOSECONDS_IN_A_SECOND 1000000000
#define INPUT_MAX_LEN 10

#define LED_PERIOD_FILE "/dev/bone/pwm/0/b/period"
#define LED_DUTY_CYCLE_FILE "/dev/bone/pwm/0/b/duty_cycle"
#define LED_ENABLE_FILE "/dev/bone/pwm/0/b/enable"

static pthread_t threads[1];

static bool is_initialized = false;
static int potReading = 0;
static int currentFreq = 0;
static bool isRunning = true;
static bool ledOn = false;

static int getVoltage0Reading();
static void* updatePWM();
// static void writeToFile(const char* filePath, const char* input);
static void updatePeriod();
static void updateDutyCycle();
static void enableLed(bool enable);

//intialize/destroy thread(s) for this module
void PotLed_init(void)
{
    assert(!is_initialized);
    is_initialized = true;
    
    pthread_create(&threads[0], NULL, updatePWM, NULL);
}

void PotLed_cleanup(void)
{
    assert(is_initialized);
    is_initialized = false;
    isRunning = false;
    enableLed(false);
    pthread_join(threads[0], NULL);
}

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
int PotLed_getPOTReading(void)
{
    assert(is_initialized);
    return potReading;
}

// Get the number of samples collected during the previous complete second.
int PotLed_getFrequency(void)
{
    assert(is_initialized);
    return currentFreq;
}

static void* updatePWM()
{
    long long startTime = getTimeInMs() + 100; //start to initially set the frequency
    // writeToFile(LED_ENABLE_FILE, "1");
    while (isRunning) {
        long long currentTime = getTimeInMs();
        if (currentTime - startTime >= 100){
                int a2dReading = getVoltage0Reading();
                // printf("%d\n", a2dReading);
                if (a2dReading != potReading){
                potReading = a2dReading;
                currentFreq = potReading / FREQUENCY_DIV_FACTOR;
                // char period[INPUT_MAX_LEN];
                // char dutyCycle[INPUT_MAX_LEN];
                // snprintf(period, INPUT_MAX_LEN, "%d", NANOSECONDS_IN_A_SECOND / currentFreq);
                // snprintf(dutyCycle, INPUT_MAX_LEN, "%d", (NANOSECONDS_IN_A_SECOND / currentFreq) / 2);
                // writeToFile(LED_PERIOD_FILE, period);
                // writeToFile(LED_DUTY_CYCLE_FILE, dutyCycle);
                // updateLed();
                if (currentFreq == 0){
                    enableLed(false);
                } else {
                    updatePeriod();
                    updateDutyCycle();
                    if (!ledOn){
                        enableLed(true);
                    }
                }
            }
            startTime = currentTime;
        }
    }
    pthread_exit(NULL);
}

// From A2D guide - to be modified for error handling
static int getVoltage0Reading()
{
    // Open file
    FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
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

// static void writeToFile(const char* filePath, const char* input)
// {
//     // int period = NANOSECONDS_IN_A_SECOND / currentFreq;
//     // int dutyCycle = period / 2;
//     // int enable = 1;
//     // set period
//     FILE *f = fopen(filePath, "w");
//     if (!f) {
//         printf("ERROR: Unable to open pwm input file. Cape loaded?\n");
//         printf(" Check /boot/uEnv.txt for correct options.\n");
//         exit(-1);
//     }
//     int charWritten = fprintf(f, "%s", input);
//     if (charWritten <= 0) {
//         printf("ERROR WRITING DATA");
//         exit(1);
//     }
//     fclose(f);
//     // set duty cycle
//     // f = fopen(LED_DUTY_CYCLE_FILE, "w");
//     // if (!f) {
//     //     printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
//     //     printf(" Check /boot/uEnv.txt for correct options.\n");
//     //     exit(-1);
//     // }
//     // charWritten = fprintf(f, "%d", period);
//     // if (charWritten <= 0) {
//     //     printf("ERROR WRITING DATA");
//     //     exit(1);
//     // }
//     // close(f)
// }

static void updatePeriod()
{
    int period = NANOSECONDS_IN_A_SECOND / currentFreq;
    // set period
    FILE *f = fopen(LED_PERIOD_FILE, "w");
    if (!f) {
        printf("ERROR: Unable to open period file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    int charWritten = fprintf(f, "%d", period);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);

}

static void updateDutyCycle()
{
    int period = NANOSECONDS_IN_A_SECOND / currentFreq;
    int dutyCycle = period / 2;
    // set duty cycle
    FILE *f = fopen(LED_DUTY_CYCLE_FILE, "w");
    if (!f) {
        printf("ERROR: Unable to open duty cycle file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    int charWritten = fprintf(f, "%d", dutyCycle);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);
}

static void enableLed(bool enable)
{
    // set enable file
    FILE *f = fopen(LED_ENABLE_FILE, "w");
    if (!f) {
        printf("ERROR: Unable to open enable file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    int charWritten;
    if (enable){
        charWritten = fprintf(f, "1");
        ledOn = true;
    } else {
        charWritten = fprintf(f, "0");
        ledOn = false;
    }
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);
}