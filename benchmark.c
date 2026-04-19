#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vortex.h"

#define NUM_YIELDS 2000000

double time_diff(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

// ---------------------------------------------
// STANDARD PTHREADS SCENARIO
// ---------------------------------------------
void *pthread_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_YIELDS; i++) {
        sched_yield(); // Forces context switch to kernel scheduler
    }
    return NULL;
}

// ---------------------------------------------
// VORTEX-RT USER-SPACE THREAD SCENARIO
// ---------------------------------------------
void vortex_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_YIELDS; i++) {
        vortex_yield(); // Bypasses kernel, stays strictly within process boundaries
    }
}

int main(void) {
    struct timespec start, end;
    
    printf("\033[1;36m====================================================\033[0m\n");
    printf("         VORTEX-RT CONTEXT SWITCH BENCHMARK         \n");
    printf("\033[1;36m====================================================\033[0m\n");
    printf("Executing \033[1;33m%d\033[0m rapid yields per thread...\n\n", NUM_YIELDS);

    // --- 1. Benchmarking Pthreads ---
    printf("Benchmarking System pthread (sched_yield)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pthread_t pt1, pt2;
    pthread_create(&pt1, NULL, pthread_worker, NULL);
    pthread_create(&pt2, NULL, pthread_worker, NULL);
    pthread_join(pt1, NULL);
    pthread_join(pt2, NULL);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double pt_time = time_diff(&start, &end);
    
    // --- 2. Benchmarking Vortex-RT ---
    printf("Benchmarking User-Level Vortex-RT (vortex_yield)...\n\n");
    vortex_init();
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    vortex_create(vortex_worker, NULL);
    vortex_create(vortex_worker, NULL);
    vortex_wait_all();
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double vt_time = time_diff(&start, &end);

    // --- Output Analytics & Bar Chart ---
    printf("\033[1;37m[ EXECUTION TIMES ]\033[0m\n");
    printf("Pthreads (Kernel Switch): \033[1;31m%.5f sec\033[0m\n", pt_time);
    printf("Vortex-RT (User Switch) : \033[1;32m%.5f sec\033[0m\n", vt_time);

    printf("\n\033[1;37m[ LATENCY REDUCTION CHART ]\033[0m\n");
    
    double max_time = pt_time > vt_time ? pt_time : vt_time;
    if (max_time == 0) max_time = 1; // Prevent div block
    
    int pt_bars = (int)((pt_time / max_time) * 50.0);
    int vt_bars = (int)((vt_time / max_time) * 50.0);
    
    // Guarantee minimal visibility
    if (pt_bars == 0 && pt_time > 0) pt_bars = 1;
    if (vt_bars == 0 && vt_time > 0) vt_bars = 1;

    printf("Pthreads  | ");
    for(int i = 0; i < pt_bars; i++) printf("\033[31m█\033[0m");
    printf("\n");

    printf("Vortex-RT | ");
    for(int i = 0; i < vt_bars; i++) printf("\033[32m█\033[0m");
    printf("\n\n");

    return 0;
}
