#include "hal/sigDisplay.h"
#include "hal/timing.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <string.h>

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"

#define LEFT_DIGIT_GPIO_PATH "/sys/class/gpio/gpio61/"
#define RIGHT_DIGIT_GPIO_PATH "/sys/class/gpio/gpio44/"

#define LEFT_VALUE LEFT_DIGIT_GPIO_PATH "value"
#define LEFT_DIRECTION LEFT_DIGIT_GPIO_PATH "direction"
#define RIGHT_VALUE RIGHT_DIGIT_GPIO_PATH "value"
#define RIGHT_DIRECTION RIGHT_DIGIT_GPIO_PATH "direction"

#define I2C_DEVICE_ADDRESS 0x20

#define REG_DIRA 0x02
#define REG_DIRB 0x03
#define REG_OUTA 0x00
#define REG_OUTB 0x01

static pthread_t thread;

static bool is_initialized = false;
static bool isRunning = true;
static int i2cFileDesc;
static int currentNumber = 0;

static void* displayNumber();
static int initI2cBus(char* bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void writeToFile(const char* filePath, const char* input);
static void configureLeftDigit(bool isLeft);
static void runCommand(char* command);

// initialize bus and registers, 
// set config pins, 
// set gpio direction, 
// spawn thread
void SigDisplay_init(void)
{
    assert(!is_initialized);
    is_initialized = true;

    runCommand("config-pin p9_18 i2c");
    runCommand("config-pin p9_17 i2c");
    
    i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    
    writeToFile(LEFT_DIRECTION, "out");
    writeToFile(RIGHT_DIRECTION, "out");

    pthread_create(&thread, NULL, displayNumber, NULL);

}

// General Cleanup
void SigDisplay_cleanup(void)
{
    assert(is_initialized);
    is_initialized = false;
    isRunning = false;
    writeToFile(LEFT_VALUE, "0");
    writeToFile(RIGHT_VALUE, "0");
    pthread_join(thread, NULL);
    close(i2cFileDesc);
}

// From I2C Guide
static int initI2cBus(char* bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0) {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

// From I2C Guide
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2) {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

static void* displayNumber()
{
    while (isRunning) {
        writeToFile(LEFT_VALUE, "0");
        writeToFile(RIGHT_VALUE, "0");
        configureLeftDigit(true);
        writeToFile(LEFT_VALUE, "1");
        sleepForMs(5);
        writeToFile(LEFT_VALUE, "0");
        writeToFile(RIGHT_VALUE, "0");
        configureLeftDigit(false);
        writeToFile(RIGHT_VALUE, "1");
        sleepForMs(5);
    }
    pthread_exit(NULL);
}

static void writeToFile(const char* filePath, const char* input)
{
    FILE *f = fopen(filePath, "w");
    if (!f) {
        printf("ERROR: Unable to file.\n");
        exit(-1);
    }
    int charWritten = fprintf(f, "%s", input);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);
}

static void configureLeftDigit(bool isLeft)
{
    int digitDisplayed;
    if (currentNumber > 99){
        digitDisplayed = 99;
    } else {
        digitDisplayed = currentNumber;
    }
    
    if (isLeft) {
        digitDisplayed = digitDisplayed / 10; //get 10s place
    } else {
        digitDisplayed = digitDisplayed % 10; //get 1s place
    }

    switch (digitDisplayed) {
        case 0:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xD0);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0xA1);
            break;
        case 1:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xC0);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x04);
            break;
        case 2:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0x98);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x83);
            break;
        case 3:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xD8);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x01);
            break;
        case 4:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xC8);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x22);
            break;
        case 5:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0x58);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x23);
            break;
        case 6:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0x58);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0xA3);
            break;
        case 7:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0x02);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x05);
            break;
        case 8:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xD8);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0xA3);
            break;
        case 9:
            writeI2cReg(i2cFileDesc, REG_OUTA, 0xC8);
            writeI2cReg(i2cFileDesc, REG_OUTB, 0x23);
            break;
        default:
            // Code for default case if value is out of range
            printf("ERROR OCCURRED WITH 14-SIG DISPLAY\n");
            exit(-1);
            break;
    }

}

void SigDisplay_setNumber(int newValue)
{
    currentNumber = newValue;
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