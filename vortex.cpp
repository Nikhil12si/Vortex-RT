#define _POSIX_C_SOURCE 199309L
#include "vortex.h"
#include "vortex_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define STACK_SIZE (1024 * 64)

#include <stdint.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#include <windows.h>
#include <malloc.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

static char* allocate_secure_stack(size_t stack_size) {
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    long page_size = sysInfo.dwPageSize;
    size_t alloc_size = stack_size + page_size;
    char *stack = (char *)VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!stack) { perror("VirtualAlloc stack failed"); exit(1); }
    DWORD oldProtect;
    if (!VirtualProtect(stack, page_size, PAGE_NOACCESS, &oldProtect)) {
        perror("VirtualProtect guard page failed"); exit(1);
    }
    return stack + page_size;
#else
    long page_size = sysconf(_SC_PAGESIZE);
    size_t alloc_size = stack_size + page_size;
    char *stack = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) { perror("mmap stack failed"); exit(1); }
    if (mprotect(stack, page_size, PROT_NONE) == -1) {
        perror("mprotect guard page failed"); exit(1);
    }
    return stack + page_size;
#endif
}

static TCB* allocate_tcb_from_pool() {
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
    TCB* new_thread = (TCB*)_aligned_malloc(sizeof(TCB), 64);
    if (!new_thread) { perror("_aligned_malloc TCB"); exit(1); }
#else
    TCB* new_thread;
    if (posix_memalign((void**)&new_thread, 64, sizeof(TCB)) != 0) {
        perror("posix_memalign TCB"); exit(1);
    }
#endif
    return new_thread;
}

static int next_thread_id = 0; // The Main Thread defaults functionally to ID: 0

void vortex_init(void) {
    // Elevate the Root Master OS Process (main) into a natively managed ThreadControlBlock!
    TCB *main_thread = allocate_tcb_from_pool();
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
    TCB *new_thread = allocate_tcb_from_pool();
    
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
    
    new_thread->stack = allocate_secure_stack(STACK_SIZE);
    
    uint64_t *stack_top = (uint64_t *)(new_thread->stack + STACK_SIZE);
    *(--stack_top) = (uint64_t)thread_wrapper;
    
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
    *(--stack_top) = 0; // rbp
    *(--stack_top) = 0; // rbx
    *(--stack_top) = 0; // rdi
    *(--stack_top) = 0; // rsi
    *(--stack_top) = 0; // r12
    *(--stack_top) = 0; // r13
    *(--stack_top) = 0; // r14
    *(--stack_top) = 0; // r15
#else
    *(--stack_top) = 0; // rbp
    *(--stack_top) = 0; // rbx
    *(--stack_top) = 0; // r12
    *(--stack_top) = 0; // r13
    *(--stack_top) = 0; // r14
    *(--stack_top) = 0; // r15
#endif
    
    new_thread->sp = stack_top;
    
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
    
    vortex_swap(&prev_thread->sp, &next_thread->sp);
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
        // extern removed
        while (scheduler_ready_empty() && !scheduler_sleep_empty()) {
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
    void *dummy_sp; 
    vortex_swap(&dummy_sp, &next_thread->sp);
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
    // extern removed
    while(1) {
        scheduler_process_sleep_queue();
        scheduler_process_aging();
        
        if (scheduler_ready_empty() && scheduler_sleep_empty()) {
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
