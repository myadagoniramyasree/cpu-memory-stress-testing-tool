/**
 * @file cpu_stress.h
 * @brief CPU-intensive calculation routines and worker workload loops.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO2: Process Management and CPU Stress.
 */

#ifndef CPU_STRESS_H
#define CPU_STRESS_H

#include "config.h"
#include "ipc.h"

/* Execute a CPU stress loop for specified duration and algorithm */
int cpu_stress_worker_loop(int worker_id, int duration_sec, cpu_algo_t algo,
                           shm_metrics_t *shm, worker_report_t *report);

/* Specific calculation kernels */
void cpu_kernel_matrix_mult(void);
void cpu_kernel_hash_mix(void);
void cpu_kernel_sqrt_trans(void);

#endif /* CPU_STRESS_H */
