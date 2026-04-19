#include "vortex.h"
#include "vortex_internal.h"
#include <stdio.h>

TCB *current_thread = NULL;
static TCB *ready_queue_head = NULL;
static TCB *ready_queue_tail = NULL;

// Enqueue thread to the back of the circular linked list tail tracker
void scheduler_enqueue(TCB *thread) {
    thread->next = NULL;
    if (ready_queue_tail == NULL) {
        ready_queue_head = thread;
        ready_queue_tail = thread;
    } else {
        ready_queue_tail->next = thread;
        ready_queue_tail = thread;
    }
}

// Pop a thread from the head of the circular linked list
TCB *scheduler_dequeue(void) {
    if (ready_queue_head == NULL) {
        return NULL;
    }
    TCB *thread = ready_queue_head;
    ready_queue_head = ready_queue_head->next;
    
    // If the queue becomes empty, clear the tail pointer too
    if (ready_queue_head == NULL) {
        ready_queue_tail = NULL;
    }
    return thread;
}

int scheduler_queue_empty(void) {
    return ready_queue_head == NULL;
}

void vortex_print_queue_status(void) {
    // Dynamically clear the current terminal line using carriage return and ANSI erase sequence
    printf("\r\033[K");
    
    // Output currently running thread with an electric GREEN coloring
    if (current_thread) {
        printf("[\033[1;32mThread %d: RUNNING\033[0m]", current_thread->id);
    }
    
    // Loop through the scheduler queue and display the wait line in YELLOW
    TCB *curr = ready_queue_head;
    while (curr != NULL) {
        printf(" -> [\033[1;33mThread %d: READY\033[0m]", curr->id);
        curr = curr->next;
    }
    
    // Flush stdout to guarantee the terminal draws it instantly without trailing newlines
    fflush(stdout);
}
