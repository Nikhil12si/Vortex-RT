#ifndef VORTEX_H
#define VORTEX_H

#include <ucontext.h>

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_WAITING_LOCK,
    THREAD_WAITING_COND,
    THREAD_WAITING_JOIN,  // Blocking on another thread to die
    THREAD_WAITING_SEM,   // Blocking on a Semaphore token
    THREAD_ZOMBIE,        // Finished execution, retaining return value for extraction
    THREAD_FINISHED
} ThreadState;

#define PRIORITY_HIGH   0
#define PRIORITY_NORMAL 1
#define PRIORITY_LOW    2
#define AGING_THRESHOLD_MS 1500 // Automatically promote priority if starved

typedef struct ThreadControlBlock {
    int id;
    int priority;
    long long wakeup_time;
    long long last_run_time; // Kernel tracking mechanism for Starvation Aging
    int has_been_promoted;   // Visualization tracker 
    
    char *stack;
    ucontext_t context;
    ThreadState state;
    
    void* (*func)(void*);    // Upgraded standard OS payload signature
    void *arg;
    void *retval;            // Extracted mathematical payload upon death

    struct ThreadControlBlock *joining_thread; // The thread tracking our lifecycle
    struct ThreadControlBlock *next;
} TCB;

/* =========================================
 * CORE LIFECYCLE MECHANICS
 * ========================================= */
void vortex_init(void);
int vortex_create(void* (*func)(void*), void *arg, int priority);
void vortex_yield(void);
void vortex_sleep(int ms);
void vortex_exit(void *retval);

// NEW: Dynamically wait for a specific thread identifier to perish and extract its value
int vortex_join(int thread_id, void **retval);
void vortex_wait_all(void);

// Hooks
void vortex_print_dashboard(void);
void vortex_print_dining_dashboard(void);

/* =========================================
 * SYNCHRONIZATION: MUTEXES
 * ========================================= */
typedef struct {
    int locked;
    int owner_id;
    TCB *wait_queue_head;
    TCB *wait_queue_tail;
} vortex_mutex_t;

void vortex_mutex_init(vortex_mutex_t *mutex);
void vortex_mutex_lock(vortex_mutex_t *mutex);
void vortex_mutex_unlock(vortex_mutex_t *mutex);

/* =========================================
 * SYNCHRONIZATION: CONDITION VARIABLES
 * ========================================= */
typedef struct {
    TCB *wait_queue_head;
    TCB *wait_queue_tail;
} vortex_cond_t;

void vortex_cond_init(vortex_cond_t *cond);
void vortex_cond_wait(vortex_cond_t *cond, vortex_mutex_t *mutex);
void vortex_cond_signal(vortex_cond_t *cond);

/* =========================================
 * SYNCHRONIZATION: COUNTING SEMAPHORES (NEW)
 * ========================================= */
typedef struct {
    int count;
    TCB *wait_queue_head;
    TCB *wait_queue_tail;
} vortex_sem_t;

void vortex_sem_init(vortex_sem_t *sem, int count);
void vortex_sem_wait(vortex_sem_t *sem);
void vortex_sem_post(vortex_sem_t *sem);

/* =========================================
 * SYNCHRONIZATION: ATOMIC SPINLOCKS
 * ========================================= */
typedef struct {
    volatile int flag;
} vortex_spinlock_t;

void vortex_spinlock_init(vortex_spinlock_t *lock);
void vortex_spinlock_lock(vortex_spinlock_t *lock);
void vortex_spinlock_unlock(vortex_spinlock_t *lock);

#endif // VORTEX_H
