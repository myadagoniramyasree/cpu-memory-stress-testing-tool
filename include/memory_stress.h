/**
 * @file memory_stress.h
 * @brief Memory allocation, access patterns, and virtual memory experimentation.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO4: Memory Stress and Virtual Memory.
 */

#ifndef MEMORY_STRESS_H
#define MEMORY_STRESS_H

#include <stddef.h>
#include "config.h"
#include "ipc.h"

/* Execute a memory stress workload loop */
int memory_stress_worker_loop(int worker_id, size_t memory_mb, int duration_sec,
                             mem_pattern_t pattern, mem_op_t op,
                             mem_alloc_type_t alloc_type,
                             shm_metrics_t *shm, worker_report_t *report);

/* Run an interactive/educational Copy-On-Write and Demand Paging experiment */
int memory_run_cow_experiment(size_t memory_mb);

/* Touch all allocated pages to force physical frame allocation */
void memory_touch_pages(char *buffer, size_t size, size_t page_size, mem_op_t op);

/* Sequential access pass */
void memory_access_sequential(char *buffer, size_t size, size_t page_size, mem_op_t op);

/* Random access pass */
void memory_access_random(char *buffer, size_t size, size_t page_size, mem_op_t op, size_t touches);

#endif /* MEMORY_STRESS_H */
