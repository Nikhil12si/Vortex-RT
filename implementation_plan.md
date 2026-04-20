# Vortex-RT Architecture Expansion Plan

This document outlines the architectural changes required to expand your user-space thread library by roughly 300% in complexity, introducing multi-level priority scheduling, time-sleep functionality, and user-space synchronization (Mutexes).

## User Review Required

Please review the proposed approach below. Once you approve, I will automatically rewrite the massive codebase and provide the terminal presentation commands sequentially at the end.

## Proposed Changes

### Core System (`vortex.h` & `vortex_internal.h`)
- **[MODIFY] `vortex.h`**: 
  - Update `ThreadState`: `READY`, `RUNNING`, `SLEEPING`, `WAITING_LOCK`, `FINISHED`.
  - Add Priority Enum: `PRIORITY_HIGH` (0), `PRIORITY_NORMAL` (1), `PRIORITY_LOW` (2).
  - Update `TCB`: Include `priority`, `wakeup_time` (Unix timestamp/millis).
  - Modify `vortex_create` signature to accept a thread priority.
  - Add Mutex definition `vortex_mutex_t` and `vortex_sleep(int ms)` prototype.
- **[MODIFY] `vortex_internal.h`**: Add references for the new sleep queue and mutex unblocking mechanisms.

### Multi-Level Priority Scheduler (`scheduler.c`)
- **[MODIFY] `scheduler.c`**:
  - Replace the single ready queue with 3 distinct linked-list queues: `ready_queue_high`, `ready_queue_normal`, `ready_queue_low`.
  - Introduce a `sleep_queue` continuously checked on every `vortex_yield` tick.
  - Upgrade `scheduler_dequeue()` to strictly cascade from High -> Normal -> Low.
  - Overhaul `vortex_print_queue_status()` to output a sprawling, real-time dashboard tracking all active queues, mutex locks, and sleeping times.

### Synchronization Subsystem (`sync.c` & `vortex.c`)
- **[NEW] `sync.c`**:
  - Implement `vortex_mutex_init()`
  - Implement `vortex_mutex_lock(mutex)`: Try to acquire. If taken, insert calling thread directly into the `mutex->wait_queue`, set state to `WAITING_LOCK`, and call `vortex_yield()`. This perfectly emulates non-busy blocking!
  - Implement `vortex_mutex_unlock(mutex)`: Release lock, pop a thread from `mutex->wait_queue`, set to `READY`, and inject it back into the Scheduler's ready queues based on its priority level.

### Visualizers & Benchmarks (`demo.c` & `benchmark.c`)
- **[MODIFY] `demo.c`**:
  - Implement the **Producer-Consumer Problem**.
  - Producers will push data into a shared buffer, Consumers pull from it.
  - Protected fully by `vortex_mutex_t`. If a producer hits a full buffer, it will `vortex_sleep(100)` to chill out.
  - ANSI dashboard will display Producers running and Consumers waiting on locks dynamically.
- **[MODIFY] `benchmark.c`**:
  - Huge expansion: Scale up to spawning 1,000 threads.
  - Each thread will lock/unlock a specific mutex 10,000 times (10,000,000 total operations) vs Native Pthreads.
- **[MODIFY] `Makefile`**: Hook up `sync.c` to compilation targets.

## Open Questions

> [!NOTE]
> Standard "Producer-Consumer" usually utilizes Semaphores or Condition Variables to prevent busy-loop checking when buffers are full. Since you specifically requested just Mutexes, I will implement it such that Producers and Consumers use `vortex_mutex_lock`, check the buffer, and if it's full/empty, they release the locker, call `vortex_sleep()` to back off, and retry later. Is this perfectly acceptable for your presentation?

## Verification Plan

### Automated Tests
- Running `./demo` will visually prove: (1) High-priority threads run before low-priority, (2) Threads wait flawlessly in Mutex queues instead of ready queues.

### Manual Verification
- Outputting the sequence of final commands to show the Professor (including compiling, visualizer running, benchmark stress-testing).
