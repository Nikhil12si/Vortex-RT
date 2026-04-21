#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include "vortex.h"

#define BUFFER_SIZE 3
int buffer[BUFFER_SIZE];
int count = 0;

vortex_mutex_t mutex;
vortex_cond_t cond_full;
vortex_cond_t cond_empty;

// Provide a custom visualization hook to scheduler UI
void render_mutex_dashboard(void) {
    if (mutex.locked) {
        printf("\033[1;31mMutex Status         : [Locked by Thread %d]\033[0m | Blocked: ", mutex.owner_id);
    } else {
        printf("\033[1;32mMutex Status         : [Unlocked]\033[0m | Blocked: ");
    }
    
    TCB *waiter = mutex.wait_queue_head;
    if (!waiter) printf("[None]");
    while (waiter) { printf("[Thread %d]->", waiter->id); waiter = waiter->next; }
    printf("\n");
    
    printf("\033[1;33mCond Waiters (Full)  : \033[0m");
    waiter = cond_full.wait_queue_head;
    if (!waiter) printf("[None]");
    while (waiter) { printf("[Thread %d]->", waiter->id); waiter = waiter->next; }
    printf("\n");

    printf("\033[1;33mCond Waiters (Empty) : \033[0m");
    waiter = cond_empty.wait_queue_head;
    if (!waiter) printf("[None]");
    while (waiter) { printf("[Thread %d]->", waiter->id); waiter = waiter->next; }
    printf("\n\n");

    printf("\033[1;37mShared Buffer Content: [%d / %d]\033[0m\n", count, BUFFER_SIZE);
}
extern void (*custom_dashboard_hook)(void);

void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < 5; i++) {
        vortex_mutex_lock(&mutex);
        
        while (count == BUFFER_SIZE) {
            vortex_cond_wait(&cond_full, &mutex);
        }
        
        buffer[count++] = 1; 
        
        vortex_print_dashboard();
        usleep(300000); 
        
        vortex_cond_signal(&cond_empty);
        vortex_mutex_unlock(&mutex);
        vortex_yield(); 
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < 5; i++) {
        vortex_mutex_lock(&mutex);
        
        while (count == 0) {
            vortex_cond_wait(&cond_empty, &mutex);
        }
        
        count--; 
        
        vortex_print_dashboard();
        usleep(300000); 
        
        vortex_cond_signal(&cond_full);
        vortex_mutex_unlock(&mutex);
        vortex_yield();
    }
    return NULL;
}

int main(void) {
    vortex_init();
    vortex_mutex_init(&mutex);
    vortex_cond_init(&cond_full);
    vortex_cond_init(&cond_empty);
    custom_dashboard_hook = render_mutex_dashboard;
    
    printf("\n\033[1;36m========================================================================================\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] SUBSYSTEM: Bounded-Buffer Condition Matrix\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] ALGORITHM: MLFQ Starvation Aging & Synchronization\033[0m\n");
    printf("\033[1;36m========================================================================================\033[0m\n\n");
    
    printf("[SYS-INIT] Bounded-Buffer Producer-Consumer Module Initialized.\n");
    printf("[SYS-INIT] Condition Variable validation matrices active.\n");
    printf("[SYS-INIT] Spawning active threads at distinct priorities to trigger Multi-Level Aging...\n\n");

    // Spawn producers and consumers at different distinct priority levels
    vortex_create(producer, NULL, PRIORITY_HIGH);
    vortex_create(producer, NULL, PRIORITY_NORMAL);
    vortex_create(consumer, NULL, PRIORITY_NORMAL);
    vortex_create(consumer, NULL, PRIORITY_LOW);
    
    vortex_wait_all();
    
    printf("\n\033[1;32mProducer-Consumer State Matrix Demo Completed Successfully!\033[0m\n\n");
    return 0;
}
