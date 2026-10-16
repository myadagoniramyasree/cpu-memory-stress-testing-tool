/**
 * @file monitor.h
 * @brief System resource utilization monitoring (/proc/stat, /proc/meminfo, /proc/self/status).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1, CO4, and System Metrics Collection.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/* Snapshot of system and process resources at a point in time */
typedef struct {
    double timestamp_sec;
    double user_cpu_pct;
    double sys_cpu_pct;
    double idle_cpu_pct;
    double total_cpu_pct;
    size_t total_mem_mb;
    size_t used_mem_mb;
    size_t free_mem_mb;
    size_t avail_mem_mb;
    size_t app_vmrss_mb;
    size_t app_vmsize_mb;
    long min_faults;
    long maj_faults;
    uint64_t total_iterations;
    double throughput_iter_sec;
} system_metrics_t;

/* Monitoring Context */
typedef struct {
    bool active;
    int interval_ms;
    uint64_t prev_user;
    uint64_t prev_nice;
    uint64_t prev_system;
    uint64_t prev_idle;
    uint64_t prev_iowait;
    long prev_minflt;
    long prev_majflt;
    double start_time_sec;
} monitor_context_t;

/* Monitor API */
int monitor_init(monitor_context_t *ctx, int interval_ms);
int monitor_sample(monitor_context_t *ctx, system_metrics_t *metrics, uint64_t current_iter);
void monitor_render_terminal_line(const system_metrics_t *metrics, double elapsed_sec, double total_sec);
void monitor_render_header(void);

/* Linux /proc Parsers */
int monitor_read_cpu_stats(uint64_t *user, uint64_t *nice, uint64_t *system, uint64_t *idle, uint64_t *iowait);
int monitor_read_mem_stats(size_t *total_mb, size_t *free_mb, size_t *avail_mb);
int monitor_read_process_stats(size_t *vm_size_mb, size_t *vm_rss_mb);

#endif /* MONITOR_H */
