#define _POSIX_C_SOURCE 199309L
#include "vortex.h"
#include "vortex_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define STACK_SIZE (1024 * 64)

static int next_thread_id = 0; // The Main Thread defaults functionally to ID: 0

void vortex_init(void) {
    // Elevate the Root Master OS Process (main) into a natively managed ThreadControlBlock!
    TCB *main_thread = (TCB *)malloc(sizeof(TCB));
    if (!main_thread) { perror("malloc main"); exit(1); }
    
    main_thread->id = next_thread_id++;
    main_thread->priority = PRIORITY_NORMAL; // Main sits safely at Normal priority
    main_thread->state = THREAD_RUNNING;
    main_thread->wakeup_time = 0;
    main_thread->last_run_time = get_time_ms();
    main_thread->has_been_promoted = 0;
    
    main_thread->func = NULL;
    main_thread->arg = NULL;
    main_thread->retval = NULL;
    main_thread->joining_thread = NULL;
    main_thread->stack = NULL; // Uses the hardware root OS stack naturally!
    
    // Inject directly without makecontext, since it is literally what is running right now.
    scheduler_register_thread(main_thread);
    current_thread = main_thread;
}

static void thread_wrapper(void) {
    TCB *self = current_thread;
    void *result = self->func(self->arg); // Store exact return matrix payload securely
    vortex_exit(result);
}

int vortex_create(void* (*func)(void*), void *arg, int priority) {
    TCB *new_thread = (TCB *)malloc(sizeof(TCB));
    if (!new_thread) { perror("malloc"); exit(1); }
    
    new_thread->id = next_thread_id++;
    new_thread->priority = priority;
    new_thread->state = THREAD_READY;
    new_thread->wakeup_time = 0;
    new_thread->last_run_time = get_time_ms();
    new_thread->has_been_promoted = 0;
    
    new_thread->func = func;
    new_thread->arg = arg;
    new_thread->retval = NULL;
    new_thread->joining_thread = NULL;
    
    new_thread->stack = (char *)malloc(STACK_SIZE);
    
    if (getcontext(&new_thread->context) == -1) { perror("getcontext"); exit(1); }
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = STACK_SIZE;
    new_thread->context.uc_link = NULL; // Manual hooking
    
    makecontext(&new_thread->context, thread_wrapper, 0);
    
    scheduler_register_thread(new_thread);
    scheduler_enqueue_ready(new_thread);
    return new_thread->id;
}

void vortex_yield(void) {
    scheduler_process_sleep_queue();
    scheduler_process_aging(); // Triggers MLFQ shifts prior to picking highest priority
    
    while (scheduler_ready_empty() && current_thread && current_thread->state != THREAD_RUNNING) {
        struct timespec req = {0, 1000 * 1000}; 
        nanosleep(&req, NULL);
        scheduler_process_sleep_queue();
        scheduler_process_aging();
    }
    
    if (scheduler_ready_empty()) {
        // Update execution timestamp tracking even if we maintain solo control
        if (current_thread) current_thread->last_run_time = get_time_ms();
        return; 
    }

    TCB *prev_thread = current_thread;
    TCB *next_thread = scheduler_dequeue_ready();

    if (prev_thread->state == THREAD_RUNNING) {
        prev_thread->state = THREAD_READY;
        scheduler_enqueue_ready(prev_thread);
    }
    
    prev_thread->last_run_time = get_time_ms();
    next_thread->last_run_time = get_time_ms();
    
    next_thread->state = THREAD_RUNNING;
    current_thread = next_thread;
    
    if (swapcontext(&prev_thread->context, &next_thread->context) == -1) {
        perror("swapcontext"); exit(1);
    }
}

void vortex_sleep(int ms) {
    if (!current_thread) return;
    current_thread->wakeup_time = get_time_ms() + ms;
    current_thread->state = THREAD_SLEEPING;
    scheduler_enqueue_sleep(current_thread);
    vortex_yield(); 
}

void vortex_exit(void *retval) {
    TCB *prev_thread = current_thread;
    prev_thread->retval = retval;
    prev_thread->state = THREAD_ZOMBIE; // Converted cleanly to a Zombie Thread
    
    // Core OS Join Trigger: Awaken the specific tracking thread blocking on our lifecycle
    if (prev_thread->joining_thread != NULL) {
        prev_thread->joining_thread->state = THREAD_READY;
        scheduler_enqueue_ready(prev_thread->joining_thread);
        prev_thread->joining_thread = NULL;
    }
    
    scheduler_process_sleep_queue();
    scheduler_process_aging();
    
    TCB *next_thread = scheduler_dequeue_ready();
    
    // Idle OS Core Fallback Protocol
    if (next_thread == NULL) {
        extern TCB *sleep_queue_head;
        while (scheduler_ready_empty() && sleep_queue_head != NULL) {
            struct timespec req = {0, 1000 * 1000}; 
            nanosleep(&req, NULL);
            scheduler_process_sleep_queue();
            scheduler_process_aging();
        }
        next_thread = scheduler_dequeue_ready();
        
        if (next_thread == NULL) {
            // Nothing left! The application has fully terminated organically.
            exit(0);
        }
    }
    
    next_thread->state = THREAD_RUNNING;
    current_thread = next_thread;
    setcontext(&next_thread->context);
}

int vortex_join(int thread_id, void **retval) {
    TCB *target = scheduler_get_thread(thread_id);
    if (!target) return -1; 
    
    if (target->state == THREAD_ZOMBIE) {
        // Instantly harvest mathematically
        if (retval) *retval = target->retval;
        target->state = THREAD_FINISHED;
        return 0;
    } else if (target->state != THREAD_FINISHED) {
        // Suspend the caller flawlessly regardless if caller is a basic thread or OS Main Loop
        current_thread->state = THREAD_WAITING_JOIN;
        target->joining_thread = current_thread;
        
        vortex_yield(); // Completely drops CPU until natively resumed by target death
        
        if (retval) *retval = target->retval;
        target->state = THREAD_FINISHED;
        return 0;
    }
    return -1;
}

void vortex_wait_all(void) {
    extern TCB *sleep_queue_head;
    while(1) {
        scheduler_process_sleep_queue();
        scheduler_process_aging();
        
        if (scheduler_ready_empty() && sleep_queue_head == NULL) {
            break;
        }
        
        if (!scheduler_ready_empty()) {
            vortex_yield(); // Join rotation correctly mapping down through queues
        } else {
            struct timespec req = {0, 1000 * 1000}; 
            nanosleep(&req, NULL);
        }
    }
}
