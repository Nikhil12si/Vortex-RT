# Vortex-RT: High-Performance User-Level Threading Library

Vortex-RT is a robust, custom user-level threading library written in C/C++ from scratch. It bypasses OS kernel scheduling to minimize context-switching latency. The framework provides a multi-level priority scheduler, starvation prevention (aging), advanced synchronization primitives, and robust thread lifecycle management.

This project demonstrates core operating system concepts in action through real-time terminal dashboards, solving classic concurrency problems, and directly comparing performance against standard POSIX pthreads.

## Features

- **Custom Context Switching:** Implemented directly in assembly for blazing-fast transitions.
- **Multi-Level Priority Scheduler:** Supports High, Normal, and Low priority queues.
- **Starvation Prevention (Aging):** Automatically promotes starved threads from lower priorities.
- **Advanced Synchronization Primitives:**
  - Mutexes
  - Condition Variables
  - Counting Semaphores
  - Atomic Spinlocks
- **Thread Lifecycle Management:** Full support for `create`, `yield`, `sleep`, `join`, and thread lifecycle state tracking.
- **Validation Demos:** Includes a Producer-Consumer simulation and a Dining Philosophers deadlock-avoidance simulator.
- **Benchmarking Suite:** Provides direct performance metrics to compare Vortex-RT against standard POSIX threads.

## Architecture

At its core, Vortex-RT utilizes a custom `ThreadControlBlock` (TCB) with cache-line alignment to prevent false sharing. Context switching is managed via a bespoke stack and hardware stack pointer manipulation.

### Thread States Managed:
- `THREAD_READY`
- `THREAD_RUNNING`
- `THREAD_SLEEPING`
- `THREAD_WAITING_LOCK`
- `THREAD_WAITING_COND`
- `THREAD_WAITING_JOIN`
- `THREAD_WAITING_SEM`
- `THREAD_ZOMBIE`

## Building the Project

This project uses a simple `Makefile` to compile the library and all demonstration executables. You will need `g++` installed.

To build all modules, simply run:
```bash
make
```

Or, you can build specific components:
```bash
make demo      # Builds the Producer-Consumer simulation
make dining    # Builds the Dining Philosophers simulator
make benchmark # Builds the performance benchmark suite
```

To clean the compiled binaries:
```bash
make clean
```

## Running the Demos

Once compiled, you can run the generated binaries directly from the terminal. 

### 1. Producer-Consumer Demo
Demonstrates real-time state changes, scheduling, and condition variable synchronization.
```bash
./demo
```

### 2. Dining Philosophers Simulator
Demonstrates deadlock-avoidance using semaphores and mutexes.
```bash
./dining
```

### 3. Performance Benchmark
Runs a comprehensive benchmark comparing Vortex-RT's context-switching and threading efficiency against POSIX pthreads.
```bash
./benchmark
```

## API Reference

Here are the core functions exposed by `vortex.h`:

### Thread Lifecycle
- `void vortex_init(void)`: Initialize the threading system.
- `int vortex_create(void* (*func)(void*), void *arg, int priority)`: Create a new thread.
- `void vortex_yield(void)`: Voluntarily yield the CPU.
- `void vortex_sleep(int ms)`: Sleep for a specific duration.
- `void vortex_exit(void *retval)`: Terminate thread and store return value.
- `int vortex_join(int thread_id, void **retval)`: Wait for a thread to exit and collect its payload.
- `void vortex_wait_all(void)`: Wait until all threads complete.

### Synchronization
- **Mutexes:** `vortex_mutex_init`, `vortex_mutex_lock`, `vortex_mutex_unlock`
- **Condition Variables:** `vortex_cond_init`, `vortex_cond_wait`, `vortex_cond_signal`
- **Semaphores:** `vortex_sem_init`, `vortex_sem_wait`, `vortex_sem_post`
- **Spinlocks:** `vortex_spinlock_init`, `vortex_spinlock_lock`, `vortex_spinlock_unlock`

## Author

Developed for an Operating Systems Project.
GitHub: [Nikhil12si](https://github.com/Nikhil12si)
