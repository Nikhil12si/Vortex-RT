#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "vortex.h"

// Constants
#define NUM_PHILOSOPHERS 5

vortex_sem_t chopsticks[NUM_PHILOSOPHERS];

void *philosopher(void *arg) {
    int id = (int)(intptr_t)arg;
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;
    
    // Asymmetric Deadlock Avoidance Strategy
    // The final philosopher picks up the right stick first, explicitly 
    // eliminating circular wait deadlocks!
    int first = (id == NUM_PHILOSOPHERS - 1) ? right : left;
    int second = (id == NUM_PHILOSOPHERS - 1) ? left : right;
    
    for (int i = 0; i < 3; i++) {
        // THINKING Phase
        printf("[SYS-TRACE] \033[1;37mPhilosopher %d\033[0m | State: THINKING | Duration: 300ms\n", id);
        usleep(300000); 
        
        // GRABBING Chopsticks
        printf("[SYS-TRACE] \033[1;37mPhilosopher %d\033[0m | Action: Acquiring locks (Chopsticks %d -> %d)\n", id, first, second);
        vortex_sem_wait(&chopsticks[first]);
        vortex_sem_wait(&chopsticks[second]);
        
        // EATING Phase
        printf("[SYS-TRACE] \033[1;32mPhilosopher %d\033[0m | State: EATING   | Metrics: Resources successfully acquired\n", id);
        usleep(400000); 
        
        // DROPPING Chopsticks (Releasing Semaphores)
        vortex_sem_post(&chopsticks[first]);
        vortex_sem_post(&chopsticks[second]);
        
        vortex_yield();
    }
    
    // Return a mathematical computational result for the main thread to Join & harvest!
    long result = (id + 1) * 1000;
    return (void*)(intptr_t)result; 
}

int main(void) {
    // 1. Initialize OS Environment
    vortex_init();
    
    // 2. Initialize Semaphores acting as chopsticks (Count=1 for binary locks)
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        vortex_sem_init(&chopsticks[i], 1); 
    }
    
    printf("\n\033[1;36m========================================================================================\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] SUBSYSTEM: Asymmetric Deadlock Avoidance Simulator\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] ALGORITHM: Structural Circular Wait Prevention\033[0m\n");
    printf("\033[1;36m========================================================================================\033[0m\n\n");
    
    printf("[SYS-INIT] Instantiating 5 Binary Semaphores (Resource: Chopsticks)\n");
    printf("[SYS-INIT] Philosopher N-1 configured for inverted acquisition (Right -> Left).\n");
    printf("[SYS-INIT] Spawning Philosopher thread entities...\n\n");
    
    // 3. Thread Spawning with various Priorities to trigger Starvation MLFQ!
    int p_ids[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int priority = (i % 2 == 0) ? PRIORITY_LOW : PRIORITY_HIGH;
        p_ids[i] = vortex_create(philosopher, (void*)(intptr_t)i, priority);
    }
    
    // 4. THE ULTIMATE JOINING TEST
    // Ordinarily we call `vortex_wait_all()`. Now, we explicitly suspend this Root 
    // Dispatcher thread until exact targets perish!
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        void *ret_payload;
        vortex_join(p_ids[i], &ret_payload);
        printf("[SYS-TEARDOWN] Main OS Dispatcher successfully reaped Philosopher %d. Payload generated: %ld\n", i, (long)(intptr_t)ret_payload);
    }
    
    printf("\n\033[1;32m[SYS-RESULT] Subsystem simulation completed. Zero structural deadlocks detected.\033[0m\n\n");
    return 0;
}
