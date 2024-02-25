// network.h
// Module to handle incoming udp packets and reply based on user commands
// supports commands including help/?, count, length, dips, <enter>, stop

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdbool.h>
#include <pthread.h>

// Begin/end the background thread which processes user commands
// Init is passed in a reference to main's condition variable so it can signal when the 'stop' command is received
void Network_init(pthread_cond_t* stopCondVar);
void Network_cleanup(void);

#endif