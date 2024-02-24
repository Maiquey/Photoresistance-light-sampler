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

#define MAX_LEN 1500
#define PORT 12345

static bool is_initialized = false;
static struct sockaddr_in sin;
static int socketDescriptor;

static pthread_t threads[1];
// static pthread_mutex_t mutexHistory;

static void* receiveData();
static void processRx(char* messageRx, int bytesRx, struct sockaddr_in sinRemote, unsigned int sin_len);

// Begin/end the background thread which processes incoming data.
void Network_init(void)
{
    assert(!is_initialized);
    is_initialized = true;

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
    assert(is_initialized);
    is_initialized = false;
    close(socketDescriptor);
    pthread_join(threads[0], NULL);
}

static void* receiveData()
{
    assert(is_initialized);
    struct sockaddr_in sinRemote;
    char messageRx[MAX_LEN];

    while(1){
        unsigned int sin_len = sizeof(sinRemote);
        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1, 0, (struct sockaddr*) &sinRemote, &sin_len);
        messageRx[bytesRx] = 0; //null-terminate
        processRx(messageRx, bytesRx, sinRemote, sin_len);

    }
}

static void processRx(char* messageRx, int bytesRx, struct sockaddr_in sinRemote, unsigned int sin_len)
{
    char messageTx[MAX_LEN];
    snprintf(messageTx, MAX_LEN, "%d bytes received: %s\n", bytesRx, messageRx);
    sendto(socketDescriptor, messageTx, strlen(messageTx), 0, (struct sockaddr*) &sinRemote, sin_len);


}