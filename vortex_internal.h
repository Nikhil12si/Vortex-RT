#ifndef VORTEX_INTERNAL_H
#define VORTEX_INTERNAL_H

#include "vortex.h"

extern TCB *current_thread;

// ASM Context Switcher (Dual ABI)
extern "C" void vortex_swap(void **prev_sp, void **next_sp);

long long get_time_ms(void);

// Advanced OS Registry mapping
void scheduler_register_thread(TCB *thread);
TCB *scheduler_get_thread(int id);

// Queueing
void scheduler_enqueue_ready(TCB *thread);
void scheduler_enqueue_sleep(TCB *thread);

// Advanced Mechanics Checkers
void scheduler_process_sleep_queue(void);
void scheduler_process_aging(void); // Promotes starving threads!

TCB *scheduler_dequeue_ready(void);
int scheduler_ready_empty(void);

extern "C" int scheduler_sleep_empty(void);

void wait_queue_enqueue(TCB **head, TCB **tail, TCB *thread);
TCB *wait_queue_dequeue(TCB **head, TCB **tail);

#endif // VORTEX_INTERNAL_H
