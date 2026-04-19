#include "vortex.h"
#include "vortex_internal.h"
#include <stdlib.h>
#include <stdio.h>

// 64 KB stack size per thread
#define STACK_SIZE (1024 * 64)

// Saved context of the original main dispatcher to jump back to when all threads exit
static ucontext_t main_context;
static int next_thread_id = 1;

void vortex_init(void) {
    // Reset the environment for threading
    current_thread = NULL;
}

// Low-level thread startup envelope.
// Bypasses argument passing in makecontext to ensure full 64-bit portability.
static void thread_wrapper(void) {
    TCB *self = current_thread;
    
    // Execute the target function
    self->func(self->arg);
    
    // The thread has finished running. Call our clean-up logic automatically.
    vortex_exit();
}

int vortex_create(void (*func)(void*), void *arg) {
    // 1. Allocate the Thread Control Block
    TCB *new_thread = (TCB *)malloc(sizeof(TCB));
    if (!new_thread) {
        perror("Failed to allocate TCB");
        exit(EXIT_FAILURE);
    }
    
    // 2. Initialize minimal state tracking
    new_thread->id = next_thread_id++;
    new_thread->state = THREAD_READY;
    new_thread->time_slice = 0;
    new_thread->func = func;
    new_thread->arg = arg;
    
    // 3. Allocate a custom heap stack for this thread
    new_thread->stack = (char *)malloc(STACK_SIZE);
    if (!new_thread->stack) {
        perror("Failed to allocate stack");
        exit(EXIT_FAILURE);
    }
    
    // 4. Initialize the execution context using getcontext
    if (getcontext(&new_thread->context) == -1) {
        perror("getcontext failed");
        exit(EXIT_FAILURE);
    }
    
    // Provide stack pointer and size to the context
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = STACK_SIZE;
    new_thread->context.uc_link = NULL; // We manage exits manually via thread_wrapper
    
    // 5. Link the execution start point to the thread_wrapper 
    makecontext(&new_thread->context, thread_wrapper, 0);
    
    // 6. Push into our tracking run queue
    scheduler_enqueue(new_thread);
    return new_thread->id;
}

void vortex_yield(void) {
    if (scheduler_queue_empty()) {
        return; // No other threads to yield to. Continue running.
    }
    
    TCB *prev_thread = current_thread;
    TCB *next_thread = scheduler_dequeue();
    
    // Only return the current thread to the queue if it's not finished
    if (prev_thread->state != THREAD_FINISHED) {
        prev_thread->state = THREAD_READY;
        scheduler_enqueue(prev_thread);
    }
    
    next_thread->state = THREAD_RUNNING;
    current_thread = next_thread;
    
    // IMPORTANT - Low-Level Mechanics of Context Switching:
    // swapcontext() saves the current CPU state (general-purpose registers, stack pointer, 
    // and process counter/instruction pointer) into prev_thread->context.
    // Simultaneously, it loads into the CPU the state stored inside next_thread->context.
    // Execution will instantly jump to exactly where next_thread previously yielded or started.
    if (swapcontext(&prev_thread->context, &next_thread->context) == -1) {
        perror("swapcontext failed");
        exit(EXIT_FAILURE);
    }
}

void vortex_exit(void) {
    TCB *prev_thread = current_thread;
    prev_thread->state = THREAD_FINISHED;
    // Note: We intentionally avoid free(prev_thread->stack) right here because we are still
    // executing using that very stack! In a production OS this would map to a deferred cleanup queue.
    
    if (scheduler_queue_empty()) {
        // We were the absolute last thread running. Switch back to the main loop context.
        current_thread = NULL;
        if (setcontext(&main_context) == -1) {
            perror("setcontext failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Fetch and run the next thread
        TCB *next_thread = scheduler_dequeue();
        next_thread->state = THREAD_RUNNING;
        current_thread = next_thread;
        
        // We use setcontext instead of swapcontext because the prev_thread's state
        // doesn't need to be saved (it is permanently finished).
        if (setcontext(&next_thread->context) == -1) {
            perror("setcontext failed");
            exit(EXIT_FAILURE);
        }
    }
}

void vortex_wait_all(void) {
    if (scheduler_queue_empty()) {
        return; // Nothing to wait for
    }
    
    // Dequeue the very first thread
    TCB *next_thread = scheduler_dequeue();
    next_thread->state = THREAD_RUNNING;
    current_thread = next_thread;
    
    // Swap away from the main thread's context and launch into the thread loop.
    // main_context is saved so threads can switch back to it when the queue exhausts.
    if (swapcontext(&main_context, &next_thread->context) == -1) {
        perror("swapcontext failed in wait_all");
        exit(EXIT_FAILURE);
    }
}
