#define _POSIX_C_SOURCE 199309L
#include "vortex.h"
#include "vortex_internal.h"
#include <stdio.h>
#include <time.h>

TCB *current_thread = NULL;

static TCB *ready_queue_head[3] = {NULL, NULL, NULL};
static TCB *ready_queue_tail[3] = {NULL, NULL, NULL};
TCB *sleep_queue_head = NULL;

// OS Thread Registry Mapping
#define MAX_THREADS 4096
static TCB *thread_registry[MAX_THREADS] = {NULL};

void scheduler_register_thread(TCB *thread) {
    if (thread->id >= 0 && thread->id < MAX_THREADS) {
        thread_registry[thread->id] = thread;
    }
}

TCB *scheduler_get_thread(int id) {
    if (id >= 0 && id < MAX_THREADS) {
        return thread_registry[id];
    }
    return NULL;
}

// Provided hook for Demo to inject its own custom UI
void (*custom_dashboard_hook)(void) = NULL;

long long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void scheduler_enqueue_ready(TCB *thread) {
    int p = thread->priority;
    if (p < PRIORITY_HIGH) p = PRIORITY_HIGH;
    if (p > PRIORITY_LOW) p = PRIORITY_LOW;
    
    // Stamp the moment it entered the queue for starvation detecting
    thread->last_run_time = get_time_ms(); 
    
    thread->next = NULL;
    if (ready_queue_tail[p] == NULL) {
        ready_queue_head[p] = thread;
        ready_queue_tail[p] = thread;
    } else {
        ready_queue_tail[p]->next = thread;
        ready_queue_tail[p] = thread;
    }
}

TCB *scheduler_dequeue_ready(void) {
    for (int p = 0; p < 3; p++) {
        if (ready_queue_head[p] != NULL) {
            TCB *thread = ready_queue_head[p];
            ready_queue_head[p] = thread->next;
            if (ready_queue_head[p] == NULL) {
                ready_queue_tail[p] = NULL;
            }
            return thread;
        }
    }
    return NULL;
}

int scheduler_ready_empty(void) {
    return ready_queue_head[0] == NULL && 
           ready_queue_head[1] == NULL && 
           ready_queue_head[2] == NULL;
}

void scheduler_enqueue_sleep(TCB *thread) {
    thread->next = sleep_queue_head;
    sleep_queue_head = thread;
}

void scheduler_process_sleep_queue(void) {
    if (!sleep_queue_head) return;
    long long now = get_time_ms();
    
    TCB *curr = sleep_queue_head;
    TCB *prev = NULL;
    
    while (curr != NULL) {
        if (now >= curr->wakeup_time) {
            TCB *to_wake = curr;
            if (prev == NULL) sleep_queue_head = curr->next;
            else prev->next = curr->next;
            
            curr = curr->next;
            to_wake->state = THREAD_READY;
            scheduler_enqueue_ready(to_wake);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

// MULTI-LEVEL FEEDBACK QUEUE: STARVATION AGING PROMOTION
void scheduler_process_aging(void) {
    long long now = get_time_ms();
    
    // KERNEL TICK OPTIMIZATION: Stop O(N) infinity scans on millions of context switches
    static long long last_aging_scan = 0;
    if (now - last_aging_scan < 50) return; // 50ms scheduler tick frequency
    last_aging_scan = now;
    
    // Only check Normal (1) and Low (2) queues for starvation
    for (int p = 1; p <= 2; p++) {
        TCB *curr = ready_queue_head[p];
        TCB *prev = NULL;
        
        while (curr != NULL) {
            // Has it been hovering in the queue completely un-run for > AGING THRESHOLD?
            if ((now - curr->last_run_time) >= AGING_THRESHOLD_MS && curr->last_run_time != 0) {
                // SEVERED AND EXTRACTED FROM CURRENT QUEUE
                TCB *upgraded = curr;
                if (prev == NULL) ready_queue_head[p] = curr->next;
                else prev->next = curr->next;
                
                if (ready_queue_tail[p] == upgraded) ready_queue_tail[p] = prev;
                
                curr = curr->next; // Progress pointer strictly before mutation
                
                // MATH: PROMOTION!
                upgraded->priority = PRIORITY_HIGH;
                upgraded->has_been_promoted = 1; 
                upgraded->state = THREAD_READY;
                // Re-enqueue drops it into the HIGH queue specifically!
                scheduler_enqueue_ready(upgraded);
                
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

void wait_queue_enqueue(TCB **head, TCB **tail, TCB *thread) {
    thread->next = NULL;
    if (*tail == NULL) { *head = thread; *tail = thread; }
    else { (*tail)->next = thread; *tail = thread; }
}

TCB *wait_queue_dequeue(TCB **head, TCB **tail) {
    if (*head == NULL) return NULL;
    TCB *thread = *head;
    *head = (*head)->next;
    if (*head == NULL) *tail = NULL;
    return thread;
}

static const char* priority_str(int p) {
    if (p == PRIORITY_HIGH) return "HIGH";
    if (p == PRIORITY_NORMAL) return "NORMAL";
    return "LOW";
}

void vortex_print_dashboard(void) {
    printf("\033[H\033[J");
    printf("\033[1;36m================ VORTEX-RT LIVE DASHBOARD ================\033[0m\n\n");
    
    if (current_thread) {
        printf("CPU Executing : [\033[1;32mThread %d (%s)\033[0m]\n", 
               current_thread->id, priority_str(current_thread->priority));
    } else {
        printf("CPU Executing : [\033[1;30mIDLE\033[0m]\n");
    }
    printf("----------------------------------------------------------\n");
    
    for (int p = 0; p < 3; p++) {
        if (p == 0) printf("\033[1;35mHigh Priority Queue  : \033[0m");
        if (p == 1) printf("\033[1;34mNormal Priority Queue: \033[0m");
        if (p == 2) printf("\033[1;30mLow Priority Queue   : \033[0m");
        
        TCB *curr = ready_queue_head[p];
        if (!curr) printf("[Empty]");
        while (curr) {
            // Visually highlight explicitly promoted threads!
            if (curr->has_been_promoted) {
                 printf("[\033[1;31mThread %d (PROMOTED)\033[0m]->", curr->id);
            } else {
                 printf("[Thread %d]->", curr->id);
            }
            curr = curr->next;
        }
        printf("\n");
    }
    printf("\n");
    
    if (custom_dashboard_hook) {
        custom_dashboard_hook();
        printf("\n");
    }
    
    printf("\033[1;36mSleeping Threads     : \033[0m");
    TCB *s = sleep_queue_head;
    if (!s) printf("[None]");
    long long now = get_time_ms();
    while (s) {
        long long remaining = s->wakeup_time - now;
        if (remaining < 0) remaining = 0;
        printf("[Thread %d (%lldms)] ", s->id, remaining);
        s = s->next;
    }
    printf("\n");
    fflush(stdout);
}
