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

static pthread_t thread;

static bool is_initialized = false;
static int potReading = 0;
static int currentFreq = 0;
static bool isRunning = true;
static bool ledOn = false;

static int getVoltage0Reading();
static void* updatePWM();
static void writeValueToFile(const char* filePath, int value);
static void runCommand(char* command);

// intialize/destroy thread(s) for this module
// Also sets config pins
void PotLed_init(void)
{
    assert(!is_initialized);
    is_initialized = true;
    runCommand("config-pin p9_21 pwm");
    pthread_create(&thread, NULL, updatePWM, NULL);
}

// General cleanup
void PotLed_cleanup(void)
{
    assert(is_initialized);
    is_initialized = false;
    isRunning = false;
    writeValueToFile(LED_ENABLE_FILE, 0);
    pthread_join(thread, NULL);
}

// returns potentiometer reading
int PotLed_getPOTReading(void)
{
    assert(is_initialized);
    return potReading;
}

// returns LED frequency
int PotLed_getFrequency(void)
{
    assert(is_initialized);
    return currentFreq;
}

// Main thread function
// Continuosly samples the a2d reading of potentiometer and adjusts frequency of led accordingly
static void* updatePWM()
{
    long long startTime = getTimeInMs() + 100; //start to initially set the frequency
    while (isRunning) {
        long long currentTime = getTimeInMs();
        if (currentTime - startTime >= 100){
                int a2dReading = getVoltage0Reading();
                if (a2dReading != potReading){
                potReading = a2dReading;
                currentFreq = potReading / FREQUENCY_DIV_FACTOR;
                int period = NANOSECONDS_IN_A_SECOND / currentFreq;
                int dutyCycle = period / 2;
                if (currentFreq == 0){
                    writeValueToFile(LED_ENABLE_FILE, 0);
                    ledOn = false;
                } else {
                    writeValueToFile(LED_PERIOD_FILE, period);
                    writeValueToFile(LED_DUTY_CYCLE_FILE, dutyCycle);
                    if (!ledOn){
                        writeValueToFile(LED_ENABLE_FILE, 1);
                        ledOn = true;
                    }
                }
            }
            startTime = currentTime;
        }
    }
    pthread_exit(NULL);
}

// From A2D guide
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

// Write integer value to pwm files for blinking LED
static void writeValueToFile(const char* filePath, int value)
{
    FILE *f = fopen(filePath, "w");
    if (!f) {
        printf("ERROR: Unable to open duty cycle file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    int charWritten = fprintf(f, "%d", value);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);
}

// From Assignment 1
static void runCommand(char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}