#ifndef _PERIOD_TIMER_H_
#define _PERIOD_TIMER_H_

// Module to record and report the timing of periodic events.
//     Written by Brian Fraser
// Usage:
//  1. Define your Period_whichEvent enums of interest
//     (ensure NUM_PERIOD_EVENTS is last).
//  2. Call Period_markEvent() periodically to mark each
//     occurrence of the event. For example, call this function
//     each time you sample the A2D.
//  3. Call getStatisticsAndClear() to get the statistics for
//     an event of interest. Calling this will clear the 
//     data collected for this event (but not others).
//     For example, call this function once a second to get timing
//     information to print to the screen.

// Maximum number of timestamps to record for a given event.
#define MAX_EVENT_TIMESTAMPS (1024*4)

enum Period_whichEvent {
    PERIOD_EVENT_SAMPLE_LIGHT,
    NUM_PERIOD_EVENTS
};

typedef struct {
    int numSamples;
    double minPeriodInMs;
    double maxPeriodInMs;
    double avgPeriodInMs;
} Period_statistics_t;

// Initialize/cleanup the module's data structures.
void Period_init(void);
void Period_cleanup(void);

// Record the current time as a timestamp for the 
// indicated event. This allows later calls to 
// Period_getStatisticsAndClear() to access these timestamps
// and compute the timing statistics for this periodic event.
void Period_markEvent(enum Period_whichEvent whichEvent);

// Fill the `pStats` struct, which must be allocated by the calling
// code, with the statistics about the periodic event `whichEvent`.
// This function is threadsafe, and may be called by any thread.
// Calling this function will, after it computes the timing
// statistics, clear the data stored for this event.
void Period_getStatisticsAndClear(
    enum Period_whichEvent whichEvent,
    Period_statistics_t *pStats
);

#endif