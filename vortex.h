#ifndef VORTEX_H
#define VORTEX_H

#include <ucontext.h>

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_FINISHED
} ThreadState;

// Thread Control Block structure
typedef struct ThreadControlBlock {
    int id;                        // Thread ID
    char *stack;                   // Stack Pointer (dynamically allocated)
    ucontext_t context;            // Execution Context for context switching
    ThreadState state;             // Thread State (READY, RUNNING, FINISHED)
    int time_slice;                // Scheduling time-slice data
    void (*func)(void*);           // Function pointer for the thread execution
    void *arg;                     // Arguments to pass to the function
    struct ThreadControlBlock *next; // Pointer to the next TCB in the run queue
} TCB;

// Initialize the Vortex-RT library
void vortex_init(void);

// Create a new thread and enqueue it. Returns the thread ID.
int vortex_create(void (*func)(void*), void *arg);

// Voluntarily yield the CPU to the next thread
void vortex_yield(void);

// Automatically called when a thread's function returns
void vortex_exit(void);

// Wait for all initialized threads to finish execution
void vortex_wait_all(void);

// Display the live run queue visualizer
void vortex_print_queue_status(void);

#endif // VORTEX_H
