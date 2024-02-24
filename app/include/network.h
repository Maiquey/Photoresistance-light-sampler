// network.h
// Module to handle incoming udp packets with netcat
//

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdbool.h>

// Begin/end the background thread which samples light levels.
void Network_init(void);
void Network_cleanup(void);

// // Must be called once every 1s.
// // Moves the samples that it has been collecting this second into
// // the history, which makes the samples available for reads (below).
// void Sampler_moveCurrentDataToHistory(void);

// // Get the number of samples collected during the previous complete second.
// int Sampler_getHistorySize(void);

// // Get a copy of the samples in the sample history.
// // Returns a newly allocated array and sets `size` to be the
// // number of elements in the returned array (output-only parameter).
// // The calling code must call free() on the returned pointer.
// // Note: It provides both data and size to ensure consistency.
// double* Sampler_getHistory(int *size);

// // Get the average light level (not tied to the history).
// double Sampler_getAverageReading(void);

// // Get the total number of light level samples taken so far.
// long long Sampler_getNumSamplesTaken(void);

#endif