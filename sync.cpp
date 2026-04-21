#include "vortex.h"
#include "vortex_internal.h"
#include <stddef.h>

#if defined(__x86_64__) || defined(__i386__)
  #include <immintrin.h>
  #define CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__)
  #define CPU_PAUSE() __asm__ volatile("yield" ::: "memory")
#else
  #define CPU_PAUSE() do {} while(0)
#endif

void vortex_mutex_init(vortex_mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner_id = -1;
    mutex->wait_queue_head = NULL;
    mutex->wait_queue_tail = NULL;
}

void vortex_mutex_lock(vortex_mutex_t *mutex) {
    if (mutex->locked) {
        current_thread->state = THREAD_WAITING_LOCK;
        wait_queue_enqueue(&mutex->wait_queue_head, &mutex->wait_queue_tail, current_thread);
        vortex_yield();
    } else {
        mutex->locked = 1;
        mutex->owner_id = current_thread->id;
    }
}

void vortex_mutex_unlock(vortex_mutex_t *mutex) {
    if (mutex->owner_id == current_thread->id) {
        TCB *waiting_thread = wait_queue_dequeue(&mutex->wait_queue_head, &mutex->wait_queue_tail);
        if (waiting_thread) {
            mutex->owner_id = waiting_thread->id;
            waiting_thread->state = THREAD_READY;
            scheduler_enqueue_ready(waiting_thread);
        } else {
            mutex->locked = 0;
            mutex->owner_id = -1;
        }
    }
}

// ... Conditional Variables ...
void vortex_cond_init(vortex_cond_t *cond) {
    cond->wait_queue_head = NULL;
    cond->wait_queue_tail = NULL;
}

void vortex_cond_wait(vortex_cond_t *cond, vortex_mutex_t *mutex) {
    current_thread->state = THREAD_WAITING_COND;
    wait_queue_enqueue(&cond->wait_queue_head, &cond->wait_queue_tail, current_thread);
    vortex_mutex_unlock(mutex);
    vortex_yield();
    vortex_mutex_lock(mutex);
}

void vortex_cond_signal(vortex_cond_t *cond) {
    TCB *waiting_thread = wait_queue_dequeue(&cond->wait_queue_head, &cond->wait_queue_tail);
    if (waiting_thread) {
        waiting_thread->state = THREAD_READY;
        scheduler_enqueue_ready(waiting_thread);
    }
}

/* =========================================
 * CORE: COUNTING SEMAPHORES (THE NEW BOSS EXCLUSIVE)
 * ========================================= */
void vortex_sem_init(vortex_sem_t *sem, int count) {
    sem->count = count;
    sem->wait_queue_head = NULL;
    sem->wait_queue_tail = NULL;
}

void vortex_sem_wait(vortex_sem_t *sem) {
    if (sem->count > 0) {
        sem->count--;
    } else {
        // Starvation logic! Thread is suspended into queue.
        current_thread->state = THREAD_WAITING_SEM;
        wait_queue_enqueue(&sem->wait_queue_head, &sem->wait_queue_tail, current_thread);
        vortex_yield(); // Blocks here
    }
}

void vortex_sem_post(vortex_sem_t *sem) {
    TCB *waiting_thread = wait_queue_dequeue(&sem->wait_queue_head, &sem->wait_queue_tail);
    if (waiting_thread) {
        // Bypasses the integer increment, transferring standard execution purely to the waiter
        waiting_thread->state = THREAD_READY;
        scheduler_enqueue_ready(waiting_thread);
    } else {
        sem->count++;
    }
}

void vortex_spinlock_init(vortex_spinlock_t *lock) {
    lock->flag.clear(std::memory_order_relaxed);
}
void vortex_spinlock_lock(vortex_spinlock_t *lock) {
    while (lock->flag.test_and_set(std::memory_order_acquire)) {
        CPU_PAUSE();
    }
}
void vortex_spinlock_unlock(vortex_spinlock_t *lock) {
    lock->flag.clear(std::memory_order_release);
}
