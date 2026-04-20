#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include "vortex.h"

#define NUM_THREADS 500
#define NUM_LOCKS   5000

double time_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

volatile int shared_counter = 0;

// ----- SCENARIO A: KERNEL PTHREADS -----
pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;
void *pthread_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_LOCKS; i++) {
        pthread_mutex_lock(&p_mutex);
        shared_counter++;
        pthread_mutex_unlock(&p_mutex);
    }
    return NULL;
}

// ----- SCENARIO B: VORTEX MUTEX -----
vortex_mutex_t v_mutex;
void *vortex_mutex_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_LOCKS; i++) {
        vortex_mutex_lock(&v_mutex);
        shared_counter++;
        vortex_mutex_unlock(&v_mutex);
    }
    return NULL;
}

// ----- SCENARIO C: VORTEX SPINLOCK -----
vortex_spinlock_t v_spinlock;
void *vortex_spinlock_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_LOCKS; i++) {
        vortex_spinlock_lock(&v_spinlock);
        shared_counter++;
        vortex_spinlock_unlock(&v_spinlock);
    }
    return NULL;
}

int main(void) {
    struct timespec start, end;
    double t_pthread, t_vmutex, t_vspin;
    
    printf("\n\033[1;36m========================================================================================\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] SUBSYSTEM: Context-Switch Latency Benchmark Suite\033[0m\n");
    printf("\033[1;33m [VORTEX-RT] ALGORITHM: User-Space Thread Bypass vs Kernel Traps\033[0m\n");
    printf("\033[1;36m========================================================================================\033[0m\n\n");
    
    printf("[SYS-INIT] Benchmark Module Initialized. Target: Minimizing kernel-mode transitions.\n");
    printf("[SYS-INIT] Spawning %d threads, %d locks each.\n\n", NUM_THREADS, NUM_LOCKS);


    // Scenario A
    printf("Benchmarking A: System Pthreads + pthread_mutex_t...\n");
    shared_counter = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_t pt[NUM_THREADS];
    for(int i=0; i<NUM_THREADS; i++) {
        pthread_create(&pt[i], NULL, pthread_worker, NULL);
    }
    for(int i=0; i<NUM_THREADS; i++) {
        pthread_join(pt[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_pthread = time_diff(&start, &end);

    // Scenario B
    printf("Benchmarking B: Vortex-RT + Yielding Mutex...\n");
    shared_counter = 0;
    vortex_init();
    vortex_mutex_init(&v_mutex);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0; i<NUM_THREADS; i++) {
        vortex_create(vortex_mutex_worker, NULL, PRIORITY_NORMAL);
    }
    vortex_wait_all();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_vmutex = time_diff(&start, &end);

    // Scenario C
    printf("Benchmarking C: Vortex-RT + Atomic Spinlock...\n\n");
    shared_counter = 0;
    vortex_init(); 
    vortex_spinlock_init(&v_spinlock);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0; i<NUM_THREADS; i++) {
        vortex_create(vortex_spinlock_worker, NULL, PRIORITY_HIGH);
    }
    vortex_wait_all();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_vspin = time_diff(&start, &end);

    // Chart Output
    printf("\033[1;37m[ EXECUTION LATENCY (Lower is better) ]\033[0m\n");
    printf("A. Kernel Pthreads : \033[1;31m%.5f sec\033[0m\n", t_pthread);
    printf("B. Vortex Mutexes  : \033[1;32m%.5f sec\033[0m\n", t_vmutex);
    printf("C. Vortex Spinlocks: \033[1;36m%.5f sec\033[0m\n\n", t_vspin);

    printf("\033[1;37m[ PERFORMANCE CHART ]\033[0m\n");
    double mx = t_pthread > t_vmutex ? t_pthread : t_vmutex;
    if (t_vspin > mx) mx = t_vspin;
    if (mx == 0) mx = 1;

    int bA = (int)((t_pthread / mx) * 50.0);
    int bB = (int)((t_vmutex / mx) * 50.0);
    int bC = (int)((t_vspin / mx) * 50.0);

    if (bA == 0 && t_pthread > 0) bA = 1;
    if (bB == 0 && t_vmutex > 0) bB = 1;
    if (bC == 0 && t_vspin > 0) bC = 1;

    printf("Kernel Mutex   | ");
    for(int i=0; i<bA; i++) printf("\033[31m█\033[0m"); printf("\n");
    printf("Vortex Mutex   | ");
    for(int i=0; i<bB; i++) printf("\033[32m█\033[0m"); printf("\n");
    printf("Vortex Spinlock| ");
    for(int i=0; i<bC; i++) printf("\033[36m█\033[0m"); printf("\n\n");

    return 0;
}
