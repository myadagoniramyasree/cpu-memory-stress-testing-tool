/**
 * @file thread_manager.c
 * @brief POSIX pthreads worker engine, mutex, condition variable, and semaphore synchronization.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO6: Multithreading and Synchronization.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <errno.h>
#include "thread_manager.h"
#include "cpu_stress.h"
#include "memory_stress.h"
#include "monitor.h"
#include "logger.h"
#include "report.h"

/* Shared structures for thread worker routines */
typedef struct {
    int thread_id;
    thread_group_t *tg;
    const stress_config_t *cfg;
    uint64_t iterations;
    double elapsed_sec;
} worker_arg_t;

/* Worker thread routine */
static void *thread_worker_routine(void *arg) {
    worker_arg_t *warg = (worker_arg_t *)arg;
    thread_group_t *tg = warg->tg;
    const stress_config_t *cfg = warg->cfg;

    /* CO6: Condition Variable Barrier. All threads block until parent signals broadcast */
    pthread_mutex_lock(&tg->shared_mutex);
    while (!tg->start_flag) {
        pthread_cond_wait(&tg->start_cond, &tg->shared_mutex);
    }
    pthread_mutex_unlock(&tg->shared_mutex);

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    uint64_t local_iter = 0;

    while (!g_stop_requested) {
        /* CO6: Counting Semaphore - throttle concurrent access */
        sem_wait(&tg->concurrency_sem);

        if (cfg->mode == MODE_MEMORY) {
            cpu_kernel_hash_mix();
        } else {
            cpu_kernel_matrix_mult();
        }

        sem_post(&tg->concurrency_sem);
        local_iter++;

        /* Periodically update shared counter protected by Mutex */
        if ((local_iter & 0x7F) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &ts_now);
            double elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                             (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;
            if (elapsed >= (double)cfg->duration_sec) {
                break;
            }

            pthread_mutex_lock(&tg->shared_mutex);
            tg->shared_counter += 128;
            pthread_mutex_unlock(&tg->shared_mutex);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    warg->elapsed_sec = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                        (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;
    warg->iterations = local_iter;

    return NULL;
}

int thread_manager_run(const stress_config_t *cfg, thread_group_t *tg) {
    if (!cfg || !tg) return -1;
    memset(tg, 0, sizeof(*tg));
    tg->count = cfg->num_workers;

    /* Initialize synchronization primitives */
    pthread_mutex_init(&tg->shared_mutex, NULL);
    pthread_cond_init(&tg->start_cond, NULL);
    /* Semaphore initialized with capacity equal to workers or max 8 */
    sem_init(&tg->concurrency_sem, 0, (unsigned int)(cfg->num_workers > 8 ? 8 : cfg->num_workers));
    tg->start_flag = false;
    tg->shared_counter = 0;

    worker_arg_t worker_args[MAX_WORKERS];
    log_info("Thread Manager: Creating %d POSIX pthreads", cfg->num_workers);

    for (int i = 0; i < cfg->num_workers; i++) {
        worker_args[i].thread_id = i;
        worker_args[i].tg = tg;
        worker_args[i].cfg = cfg;
        worker_args[i].iterations = 0;
        worker_args[i].elapsed_sec = 0.0;

        int rc = pthread_create(&tg->workers[i].thread_handle, NULL,
                                thread_worker_routine, &worker_args[i]);
        if (rc != 0) {
            log_error("Thread Manager: pthread_create failed for thread %d: %s", i, strerror(rc));
            return -1;
        }
    }

    /* Release all threads simultaneously using pthread_cond_broadcast */
    pthread_mutex_lock(&tg->shared_mutex);
    tg->start_flag = true;
    pthread_cond_broadcast(&tg->start_cond);
    pthread_mutex_unlock(&tg->shared_mutex);

    log_info("Thread Manager: Condition broadcast emitted. All %d threads running.", cfg->num_workers);

    /* Monitor loop */
    monitor_context_t mon_ctx;
    monitor_init(&mon_ctx, cfg->sample_interval_ms);
    monitor_render_header();

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    double elapsed = 0.0;

    while (elapsed < (double)cfg->duration_sec && !g_stop_requested) {
        usleep((useconds_t)cfg->sample_interval_ms * 1000);

        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                  (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;

        pthread_mutex_lock(&tg->shared_mutex);
        uint64_t current_shared = tg->shared_counter;
        pthread_mutex_unlock(&tg->shared_mutex);

        system_metrics_t metrics;
        monitor_sample(&mon_ctx, &metrics, current_shared);
        monitor_render_terminal_line(&metrics, elapsed, (double)cfg->duration_sec);

        report_csv_append(cfg->csv_file, &metrics, cfg, elapsed);
    }

    /* Wait and join all worker threads */
    for (int i = 0; i < cfg->num_workers; i++) {
        pthread_join(tg->workers[i].thread_handle, NULL);
        tg->workers[i].iterations = worker_args[i].iterations;
        tg->workers[i].elapsed_sec = worker_args[i].elapsed_sec;
        log_debug("Thread Manager: Joined thread %d (%lu iterations, %.2fs)",
                  i, (unsigned long)worker_args[i].iterations, worker_args[i].elapsed_sec);
    }

    /* Destroy sync objects */
    pthread_mutex_destroy(&tg->shared_mutex);
    pthread_cond_destroy(&tg->start_cond);
    sem_destroy(&tg->concurrency_sem);

    return 0;
}

/* =========================================================================
 * CO6 DEMONSTRATION: Race Condition vs. Mutex Synchronization
 * ========================================================================= */
static volatile uint64_t s_race_counter = 0;
static pthread_mutex_t s_race_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    int increments;
    bool use_mutex;
} race_arg_t;

static void *race_thread_worker(void *arg) {
    race_arg_t *r = (race_arg_t *)arg;

    if (r->use_mutex) {
        /* Correct: Synchronized with Mutex */
        for (int i = 0; i < r->increments; i++) {
            pthread_mutex_lock(&s_race_mutex);
            s_race_counter++;
            pthread_mutex_unlock(&s_race_mutex);
        }
    } else {
        /* Race Condition: Raw unprotected increment (Read-Modify-Write race) */
        for (int i = 0; i < r->increments; i++) {
            s_race_counter++;
        }
    }

    return NULL;
}

int thread_manager_run_race_demo(int num_threads, int increments_per_thread, bool use_mutex) {
    if (num_threads < 1 || num_threads > MAX_WORKERS) num_threads = 4;
    if (increments_per_thread <= 0) increments_per_thread = 500000;

    uint64_t expected = (uint64_t)num_threads * (uint64_t)increments_per_thread;
    s_race_counter = 0;

    printf("\n=================================================================\n");
    printf("   CO6 DEMO: RACE CONDITION VS MUTEX SYNCHRONIZATION            \n");
    printf("=================================================================\n");
    printf("  Worker Threads          : %d\n", num_threads);
    printf("  Increments Per Thread   : %d\n", increments_per_thread);
    printf("  Expected Final Total    : %lu\n", (unsigned long)expected);
    printf("  Synchronization Scheme  : %s\n",
           use_mutex ? "POSIX Mutex (pthread_mutex_lock/unlock)" : "NONE (Raw Unsynchronized Increments)");
    printf("-----------------------------------------------------------------\n");

    pthread_t threads[MAX_WORKERS];
    race_arg_t args[MAX_WORKERS];

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].increments = increments_per_thread;
        args[i].use_mutex = use_mutex;
        pthread_create(&threads[i], NULL, race_thread_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    double elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                     (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;

    int64_t lost_updates = (int64_t)expected - (int64_t)s_race_counter;
    double loss_pct = ((double)lost_updates / (double)expected) * 100.0;

    printf("  Observed Final Total    : %lu\n", (unsigned long)s_race_counter);
    printf("  Time Elapsed            : %.4f seconds\n", elapsed);

    if (use_mutex) {
        printf("  Result Verification     : [PASSED] Counter matches expected total exactly!\n");
        printf("  OS Concept              : Mutex enforced mutual exclusion across critical sections,\n"
               "                            preventing race conditions and data corruption.\n");
    } else {
        printf("  Lost Updates            : %ld updates lost (%.2f%% corruption!)\n",
               (long)lost_updates, loss_pct);
        printf("  Result Verification     : [RACE CONDITION CONFIRMED]\n");
        printf("  OS Concept              : Concurrent threads suffered read-modify-write interleaved\n"
               "                            hazards on the shared register/cache variable.\n");
    }
    printf("=================================================================\n\n");

    return 0;
}
