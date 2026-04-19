#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include "vortex.h"

void animate_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 6; i++) {
        // Output dynamic live visualization
        vortex_print_queue_status();
        
        // Emulate CPU workload (only present here for visual demonstration)
        usleep(350000); // 350ms wait
        
        // Round robin yield
        vortex_yield();
    }
}

int main(void) {
    printf("\n\033[1;36mInitializing Vortex-RT Visual Dashboard...\033[0m\n\n");
    
    vortex_init();
    
    // Spawn a pool of test threads
    vortex_create(animate_worker, NULL);
    vortex_create(animate_worker, NULL);
    vortex_create(animate_worker, NULL);
    vortex_create(animate_worker, NULL);
    
    // Hand over control to the thread dispatcher
    vortex_wait_all();
    
    printf("\n\n\033[1;32mExecution Terminated. Returning to main block!\033[0m\n\n");
    return 0;
}
