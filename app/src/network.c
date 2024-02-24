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
#define PORT 12345

static pthread_cond_t* mainCondVar;

static bool isRunning = true;
static bool is_initialized = false;
static struct sockaddr_in sin;
static int socketDescriptor;
static bool firstMessage = true;
static char lastMessage[MAX_LEN];

static pthread_t threads[1];
// static pthread_mutex_t mutexHistory;

static void* receiveData();
static void processRx(char* messageRx, int bytesRx, struct sockaddr_in sinRemote, unsigned int sin_len);

// Begin/end the background thread which processes incoming data.
void Network_init(pthread_cond_t* stopCondVar)
{
    assert(!is_initialized);
    is_initialized = true;

    mainCondVar = stopCondVar;

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

    //TODO fix repeated command
    if (!firstMessage && bytesRx == 1 && messageRx[0]){
        messageRx = lastMessage;
    }
    // Generated with help from chatGPT for efficiency
    if (strcmp(messageRx, "help\n") == 0 || strcmp(messageRx, "?\n") == 0){
        snprintf(messageTx, MAX_LEN, HELP_MSG);
    }
    else if (strcmp(messageRx, "count\n") == 0){
        snprintf(messageTx, MAX_LEN, "%lld\n", Sampler_getNumSamplesTaken());
    }
    else if (strcmp(messageRx, "length\n") == 0){
        snprintf(messageTx, MAX_LEN, "%d\n", Sampler_getHistorySize());
    }
    else if (strcmp(messageRx, "dips\n") == 0){
        snprintf(messageTx, MAX_LEN, "unsupported command\n");
    }
    else if (strcmp(messageRx, "history\n") == 0){
        snprintf(messageTx, MAX_LEN, "unsupported command\n");
    }
    else if (strcmp(messageRx, "stop\n") == 0){
        snprintf(messageTx, MAX_LEN, "Program terminating.\n");
    }
    else{
        snprintf(messageTx, MAX_LEN, "unknown command\n");
    }

    sendto(socketDescriptor, messageTx, strlen(messageTx), 0, (struct sockaddr*) &sinRemote, sin_len);
    if (strcmp(messageRx, "stop\n") == 0){
        printf("signalling main\n");
        pthread_cond_signal(mainCondVar);
        isRunning = false;
    }
    firstMessage = false;
    memcpy(lastMessage, messageRx, sizeof(char) * bytesRx);
}