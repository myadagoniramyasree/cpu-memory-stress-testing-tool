/**
 * @file cpu_stress.c
 * @brief CPU-intensive computation algorithms and worker workload loops.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO2: Process Management and CPU Stress.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "cpu_stress.h"
#include "ipc.h"

#define MATRIX_DIM 32

/* Kernel 1: Matrix Multiplication (Cache & ALU stress) */
void cpu_kernel_matrix_mult(void) {
    static volatile double A[MATRIX_DIM][MATRIX_DIM];
    static volatile double B[MATRIX_DIM][MATRIX_DIM];
    static volatile double C[MATRIX_DIM][MATRIX_DIM];

    for (int i = 0; i < MATRIX_DIM; i++) {
        for (int j = 0; j < MATRIX_DIM; j++) {
            A[i][j] = (double)(i + j) * 0.01;
            B[i][j] = (double)(i * j) * 0.02;
            C[i][j] = 0.0;
        }
    }

    for (int i = 0; i < MATRIX_DIM; i++) {
        for (int k = 0; k < MATRIX_DIM; k++) {
            for (int j = 0; j < MATRIX_DIM; j++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

/* Kernel 2: 64-bit Cryptographic-style Hash Mixing (Integer pipeline stress) */
void cpu_kernel_hash_mix(void) {
    volatile uint64_t hash = 0xCBF29CE484222325ULL;
    for (int i = 0; i < 2000; i++) {
        hash ^= (uint64_t)i + 0x9e3779b97f4a7c15ULL;
        hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
        hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
        hash ^= (hash >> 31);
    }
}

/* Kernel 3: Transcendental math & Square Roots (FPU / SIMD stress) */
void cpu_kernel_sqrt_trans(void) {
    volatile double sum = 1.0;
    for (int i = 1; i <= 1500; i++) {
        sum += sqrt((double)i) * sin((double)i * 0.005) + cos((double)i * 0.003);
    }
}

int cpu_stress_worker_loop(int worker_id, int duration_sec, cpu_algo_t algo,
                           shm_metrics_t *shm, worker_report_t *report) {
    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    uint64_t iterations = 0;
    pid_t my_pid = getpid();

    while (!g_stop_requested) {
        /* Run calculation batch */
        switch (algo) {
            case CPU_ALGO_MATRIX:
                cpu_kernel_matrix_mult();
                break;
            case CPU_ALGO_HASH:
                cpu_kernel_hash_mix();
                break;
            case CPU_ALGO_SQRT:
                cpu_kernel_sqrt_trans();
                break;
        }
        iterations++;

        /* Check elapsed time every 128 iterations to reduce syscall overhead */
        if ((iterations & 0x7F) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &ts_now);
            double elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                             (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;
            if (elapsed >= (double)duration_sec) {
                break;
            }

            /* Update shared memory if configured */
            if (shm) {
                ipc_shm_update_worker(shm, worker_id, my_pid, iterations, 0);
                if (shm->terminate_flag) {
                    break;
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    double total_elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                           (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;

    /* Collect resource usage via getrusage() */
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    if (report) {
        report->pid = my_pid;
        report->worker_id = worker_id;
        report->iterations = iterations;
        report->elapsed_sec = total_elapsed;
        report->user_time_sec = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec * 1e-6;
        report->sys_time_sec = (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec * 1e-6;
        report->min_faults = usage.ru_minflt;
        report->maj_faults = usage.ru_majflt;
        report->memory_bytes = 0;
        report->exit_code = 0;
        snprintf(report->status_msg, sizeof(report->status_msg),
                 "Worker %d completed %lu CPU iterations in %.2fs (User: %.2fs, Sys: %.2fs)",
                 worker_id, (unsigned long)iterations, total_elapsed,
                 report->user_time_sec, report->sys_time_sec);
    }

    return 0;
}
