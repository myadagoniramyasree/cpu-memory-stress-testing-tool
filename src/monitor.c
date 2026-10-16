/**
 * @file monitor.c
 * @brief System resource utilization monitoring (/proc/stat, /proc/meminfo, /proc/self/status).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1, CO4, and Linux Resource Monitoring APIs.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include "monitor.h"

int monitor_read_cpu_stats(uint64_t *user, uint64_t *nice, uint64_t *system, uint64_t *idle, uint64_t *iowait) {
    int fd = open("/proc/stat", O_RDONLY);
    if (fd < 0) return -1;

    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) return -1;
    buffer[bytes] = '\0';

    unsigned long long u = 0, n = 0, s = 0, id = 0, io = 0;
    if (sscanf(buffer, "cpu %llu %llu %llu %llu %llu", &u, &n, &s, &id, &io) >= 4) {
        if (user)   *user   = (uint64_t)u;
        if (nice)   *nice   = (uint64_t)n;
        if (system) *system = (uint64_t)s;
        if (idle)   *idle   = (uint64_t)id;
        if (iowait) *iowait = (uint64_t)io;
        return 0;
    }

    return -1;
}

int monitor_read_mem_stats(size_t *total_mb, size_t *free_mb, size_t *avail_mb) {
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) return -1;

    char buffer[2048];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) return -1;
    buffer[bytes] = '\0';

    unsigned long total_kb = 0, free_kb = 0, avail_kb = 0;
    char *ptr = strstr(buffer, "MemTotal:");
    if (ptr) sscanf(ptr, "MemTotal: %lu kB", &total_kb);

    ptr = strstr(buffer, "MemFree:");
    if (ptr) sscanf(ptr, "MemFree: %lu kB", &free_kb);

    ptr = strstr(buffer, "MemAvailable:");
    if (ptr) sscanf(ptr, "MemAvailable: %lu kB", &avail_kb);
    else avail_kb = free_kb;

    if (total_mb) *total_mb = (size_t)(total_kb / 1024);
    if (free_mb)  *free_mb  = (size_t)(free_kb / 1024);
    if (avail_mb) *avail_mb = (size_t)(avail_kb / 1024);

    return 0;
}

int monitor_read_process_stats(size_t *vm_size_mb, size_t *vm_rss_mb) {
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd < 0) return -1;

    char buffer[2048];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) return -1;
    buffer[bytes] = '\0';

    unsigned long size_kb = 0, rss_kb = 0;
    char *ptr = strstr(buffer, "VmSize:");
    if (ptr) sscanf(ptr, "VmSize: %lu kB", &size_kb);

    ptr = strstr(buffer, "VmRSS:");
    if (ptr) sscanf(ptr, "VmRSS: %lu kB", &rss_kb);

    if (vm_size_mb) *vm_size_mb = (size_t)(size_kb / 1024);
    if (vm_rss_mb)  *vm_rss_mb  = (size_t)(rss_kb / 1024);

    return 0;
}

int monitor_init(monitor_context_t *ctx, int interval_ms) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->interval_ms = interval_ms;
    ctx->active = true;

    monitor_read_cpu_stats(&ctx->prev_user, &ctx->prev_nice, &ctx->prev_system,
                           &ctx->prev_idle, &ctx->prev_iowait);

    struct rusage ru_self, ru_child;
    getrusage(RUSAGE_SELF, &ru_self);
    getrusage(RUSAGE_CHILDREN, &ru_child);
    ctx->prev_minflt = ru_self.ru_minflt + ru_child.ru_minflt;
    ctx->prev_majflt = ru_self.ru_majflt + ru_child.ru_majflt;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ctx->start_time_sec = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;

    return 0;
}

int monitor_sample(monitor_context_t *ctx, system_metrics_t *metrics, uint64_t current_iter) {
    if (!ctx || !metrics) return -1;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double now_sec = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    metrics->timestamp_sec = now_sec - ctx->start_time_sec;

    /* Read CPU metrics and calculate delta percentage */
    uint64_t cur_u = 0, cur_n = 0, cur_s = 0, cur_id = 0, cur_io = 0;
    if (monitor_read_cpu_stats(&cur_u, &cur_n, &cur_s, &cur_id, &cur_io) == 0) {
        uint64_t du = cur_u - ctx->prev_user;
        uint64_t dn = cur_n - ctx->prev_nice;
        uint64_t ds = cur_s - ctx->prev_system;
        uint64_t did = cur_id - ctx->prev_idle;
        uint64_t dio = cur_io - ctx->prev_iowait;

        uint64_t dtotal = du + dn + ds + did + dio;
        if (dtotal > 0) {
            metrics->user_cpu_pct  = ((double)(du + dn) * 100.0) / (double)dtotal;
            metrics->sys_cpu_pct   = ((double)ds * 100.0) / (double)dtotal;
            metrics->idle_cpu_pct  = ((double)did * 100.0) / (double)dtotal;
            metrics->total_cpu_pct = ((double)(dtotal - did) * 100.0) / (double)dtotal;
        }

        ctx->prev_user = cur_u;
        ctx->prev_nice = cur_n;
        ctx->prev_system = cur_s;
        ctx->prev_idle = cur_id;
        ctx->prev_iowait = cur_io;
    }

    /* Read system memory stats */
    monitor_read_mem_stats(&metrics->total_mem_mb, &metrics->free_mem_mb, &metrics->avail_mem_mb);
    if (metrics->total_mem_mb >= metrics->avail_mem_mb) {
        metrics->used_mem_mb = metrics->total_mem_mb - metrics->avail_mem_mb;
    }

    /* Read process memory stats */
    monitor_read_process_stats(&metrics->app_vmsize_mb, &metrics->app_vmrss_mb);

    /* Read page faults */
    struct rusage ru_self, ru_child;
    getrusage(RUSAGE_SELF, &ru_self);
    getrusage(RUSAGE_CHILDREN, &ru_child);
    long total_min = ru_self.ru_minflt + ru_child.ru_minflt;
    long total_maj = ru_self.ru_majflt + ru_child.ru_majflt;

    metrics->min_faults = total_min;
    metrics->maj_faults = total_maj;

    metrics->total_iterations = current_iter;
    if (metrics->timestamp_sec > 0.0) {
        metrics->throughput_iter_sec = (double)current_iter / metrics->timestamp_sec;
    }

    return 0;
}

void monitor_render_header(void) {
    printf("\n  %-8s | %-16s | %-18s | %-10s | %-10s | %-14s\n",
           "ELAPSED", "TOTAL CPU (U/S)", "SYS MEM (USED/AVAIL)", "APP RSS", "PAGE FAULTS", "THROUGHPUT");
    printf("  ---------+------------------+--------------------+------------+------------+---------------\n");
}

void monitor_render_terminal_line(const system_metrics_t *metrics, double elapsed_sec, double total_sec) {
    if (!metrics) return;

    printf("  %5.1fs/%-3.0fs | %5.1f%% (%4.1f/%4.1f) | %5zuM / %-5zuM   | %5zu MB   | %-10ld | %10.0f it/s\n",
           elapsed_sec, total_sec,
           metrics->total_cpu_pct, metrics->user_cpu_pct, metrics->sys_cpu_pct,
           metrics->used_mem_mb, metrics->avail_mem_mb,
           metrics->app_vmrss_mb,
           metrics->min_faults,
           metrics->throughput_iter_sec);
    fflush(stdout);
}
