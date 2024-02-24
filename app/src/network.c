#include "network.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
// #include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "hal/sampler.h"

#define HELP_MSG "\nAccepted command examples:\ncount      -- get the total number of samples taken.\nlength     -- get the number of samples taken in the previously completed second.\ndips       -- get the number of dips in the previously completed second.\nhistory    -- get all the samples in the previously completed second.\nstop       -- cause the server program to end.\n<enter>    -- repeat last command.\n"
#define MAX_LEN 1500
#define MAX_WRITABLE_HISTORY 1498
#define PORT 12345

static pthread_cond_t* mainCondVar;
static pthread_mutex_t* sampleHistoryMutex;

static bool isRunning = true;
static bool is_initialized = false;
static struct sockaddr_in sin;
static int socketDescriptor;
static bool firstMessage = true;
static char lastMessage[MAX_LEN];

static pthread_t threads[1]; //TODO - fix this (make it not in an array)
// static pthread_mutex_t mutexHistory;

static void* receiveData();
static void processRx(char* messageRx, int bytesRx, struct sockaddr_in sinRemote, unsigned int sin_len);

// Begin/end the background thread which processes incoming data.
void Network_init(pthread_cond_t* stopCondVar)
{
    assert(!is_initialized);
    is_initialized = true;

    mainCondVar = stopCondVar;
    sampleHistoryMutex = Sampler_getHistoryMutexRef();

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
    bind(socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));
    pthread_create(&threads[0], NULL, receiveData, NULL);
}

void Network_cleanup(void)
{
    printf("network cleanup\n");
    assert(is_initialized);
    is_initialized = false;
    isRunning = false;
    close(socketDescriptor);
    pthread_join(threads[0], NULL);
    printf("network cleanup done\n");
}

static void* receiveData()
{
    assert(is_initialized);
    struct sockaddr_in sinRemote;
    char messageRx[MAX_LEN];

    while(isRunning){
        unsigned int sin_len = sizeof(sinRemote);
        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1, 0, (struct sockaddr*) &sinRemote, &sin_len);
        messageRx[bytesRx] = 0; //null-terminate
        processRx(messageRx, bytesRx, sinRemote, sin_len);

    }

    pthread_exit(NULL);
}

static void processRx(char* messageRx, int bytesRx, struct sockaddr_in sinRemote, unsigned int sin_len)
{
    char messageTx[MAX_LEN];

    if (!firstMessage && bytesRx == 1 && messageRx[0]){
        messageRx = lastMessage;
    }
    // Generated with some help from chatGPT for efficiency
    if (strncmp(messageRx, "help", strlen("help")) == 0 || strncmp(messageRx, "?", strlen("?")) == 0){
        snprintf(messageTx, MAX_LEN, HELP_MSG);
    }
    else if (strncmp(messageRx, "count", strlen("count")) == 0){
        snprintf(messageTx, MAX_LEN, "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
    }
    else if (strncmp(messageRx, "length", strlen("length")) == 0){
        snprintf(messageTx, MAX_LEN, "# samples taken last second: %d\n", Sampler_getHistorySize());
    }
    else if (strncmp(messageRx, "dips", strlen("dips")) == 0){
        snprintf(messageTx, MAX_LEN, "# Dips: %d\n", 0);
    }
    else if (strncmp(messageRx, "history", strlen("history")) == 0){
        // snprintf(messageTx, MAX_LEN, "unsupported command - history\n");
        pthread_mutex_lock(sampleHistoryMutex);
        int historySize = Sampler_getHistorySize();
        double* history = Sampler_getHistory(&historySize);
        pthread_mutex_unlock(sampleHistoryMutex);
        int offset = 0;
        int bytesWritten = 0;
        for (int i = 0; i < historySize; i++){
            if ((i+1) % 10 == 0 || i == historySize - 1){
                bytesWritten = snprintf(messageTx + offset, MAX_LEN - offset, "%.3f,\n", history[i]);
            } else {
                bytesWritten = snprintf(messageTx + offset, MAX_LEN - offset, "%.3f, ", history[i]);
            }
            offset += bytesWritten;
            printf("offset = %d\n", offset);
            if (offset == MAX_WRITABLE_HISTORY){
                sendto(socketDescriptor, messageTx, strlen(messageTx), 0, (struct sockaddr*) &sinRemote, sin_len);
                memset(messageTx, 0, sizeof(messageTx));
                offset = 0;
                bytesWritten = 0;
            }
        }
    }
    else if (strncmp(messageRx, "stop", strlen("stop")) == 0){
        snprintf(messageTx, MAX_LEN, "Program terminating.\n");
    }
    else{
        snprintf(messageTx, MAX_LEN, "unknown command\n");
    }

    sendto(socketDescriptor, messageTx, strlen(messageTx), 0, (struct sockaddr*) &sinRemote, sin_len);
    if (strncmp(messageRx, "stop", strlen("stop")) == 0){
        printf("signalling main\n");
        pthread_cond_signal(mainCondVar);
        isRunning = false;
    }
    firstMessage = false;
    memcpy(lastMessage, messageRx, sizeof(char) * bytesRx);
}