/**
 * @file thread_manager.h
 * @brief POSIX pthreads worker engine, mutex, condition variable, semaphore synchronization.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO6: Multithreading and Synchronization.
 */

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <pthread.h>
#include <semaphore.h>
#include "config.h"
#include "ipc.h"

/* Thread worker context */
typedef struct {
    int thread_id;
    const stress_config_t *cfg;
    uint64_t iterations;
    double elapsed_sec;
    long min_faults;
    long maj_faults;
    pthread_t thread_handle;
} thread_worker_t;

/* Thread group state */
typedef struct {
    int count;
    thread_worker_t workers[MAX_WORKERS];
    pthread_mutex_t shared_mutex;
    pthread_cond_t start_cond;
    sem_t concurrency_sem;
    bool start_flag;
    uint64_t shared_counter;
    uint64_t expected_counter;
} thread_group_t;

/* Run thread-based workload */
int thread_manager_run(const stress_config_t *cfg, thread_group_t *tg);

/* Run race condition vs mutex demonstration */
int thread_manager_run_race_demo(int num_threads, int increments_per_thread, bool use_mutex);

#endif /* THREAD_MANAGER_H */
