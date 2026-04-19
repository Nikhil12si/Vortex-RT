#ifndef VORTEX_INTERNAL_H
#define VORTEX_INTERNAL_H

#include "vortex.h"

// Pointer to the currently executing thread
extern TCB *current_thread;

// Queue operations for the custom round-robin scheduler
void scheduler_enqueue(TCB *thread);
TCB *scheduler_dequeue(void);
int scheduler_queue_empty(void);

#endif // VORTEX_INTERNAL_H
