// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "periodTimer.h"
#include "hal/sampler.h"

pthread_mutex_t mutexMain;
pthread_cond_t condVarFinished;

int main()
{
    printf("Hello world!\n");
    
    pthread_mutex_init(&mutexMain, NULL);
    pthread_cond_init(&condVarFinished, NULL);

    // Initialize all modules; HAL modules first

    // Main program logic:
    // for (int i = 0; i < 10; i++) {
    //     printf("  -> Reading button time %d = %d\n", i, i);
    // }
    Sampler_init();
    pthread_mutex_lock(&mutexMain);
    pthread_cond_wait(&condVarFinished, &mutexMain);
    pthread_mutex_unlock(&mutexMain);

    // Cleanup all modules (HAL modules last)

    printf("!!! DONE !!!\n"); 

    // Some bad code to try out and see what shows up.
    #if 0
        // Test your linting setup
        //   - You should see a warning underline in VS Code
        //   - You should see compile-time errors when building (-Wall -Werror)
        // (Linting using clang-tidy; see )
        int x = 0;
        if (x = 10) {
        }
    #endif
    #if 0
        // Demonstrate -fsanitize=address (enabled in the root CMakeFiles.txt)
        // Compile and run this code. Should see warning at compile time; error at runtime.
        int data[3];
        data[3] = 10;
        printf("Value: %d\n", data[3]);
    #endif
}