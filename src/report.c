/**
 * @file report.c
 * @brief Performance report generation (CSV datasets, text summaries, and history ledger).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO5: File-System Operations and Performance Reports.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include "report.h"
#include "logger.h"

int report_csv_init(const char *csv_path) {
    if (!csv_path) return -1;

    char dir_copy[512];
    strncpy(dir_copy, csv_path, sizeof(dir_copy) - 1);
    char *slash = strrchr(dir_copy, '/');
    if (slash) {
        *slash = '\0';
        logger_ensure_dir(dir_copy);
    }

    int fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        log_error("Report Error: open() failed for CSV '%s': %s", csv_path, strerror(errno));
        return -1;
    }

    const char *header = "Timestamp_Sec,Elapsed_Sec,Mode,Workers,User_CPU_Pct,Sys_CPU_Pct,"
                         "Total_CPU_Pct,Mem_Used_MB,Mem_Avail_MB,App_VmRSS_MB,App_VmSize_MB,"
                         "Min_Faults,Maj_Faults,Throughput_Iter_Sec,Total_Iterations\n";

    ssize_t written = write(fd, header, strlen(header));
    close(fd);

    return (written > 0) ? 0 : -1;
}

int report_csv_append(const char *csv_path, const system_metrics_t *metrics,
                      const stress_config_t *cfg, double elapsed_sec) {
    if (!csv_path || !metrics || !cfg) return -1;

    int fd = open(csv_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1;

    char row[512];
    int len = snprintf(row, sizeof(row),
                      "%.2f,%.2f,%s,%d,%.2f,%.2f,%.2f,%zu,%zu,%zu,%zu,%ld,%ld,%.2f,%lu\n",
                      metrics->timestamp_sec, elapsed_sec,
                      mode_to_string(cfg->mode), cfg->num_workers,
                      metrics->user_cpu_pct, metrics->sys_cpu_pct, metrics->total_cpu_pct,
                      metrics->used_mem_mb, metrics->avail_mem_mb,
                      metrics->app_vmrss_mb, metrics->app_vmsize_mb,
                      metrics->min_faults, metrics->maj_faults,
                      metrics->throughput_iter_sec, (unsigned long)metrics->total_iterations);

    if (len > 0) {
        ssize_t written = write(fd, row, (size_t)len);
        (void)written;
    }
    close(fd);
    return 0;
}

int report_update_history_ledger(const char *history_csv_path,
                                const stress_config_t *cfg,
                                uint64_t total_iterations,
                                double avg_cpu_pct,
                                size_t peak_rss_mb,
                                double duration_sec,
                                const char *status) {
    if (!history_csv_path || !cfg) return -1;

    struct stat st;
    bool exists = (stat(history_csv_path, &st) == 0);

    int fd = open(history_csv_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1;

    if (!exists) {
        const char *hdr = "Timestamp,Mode,Workers,Duration_Sec,Memory_MB,Avg_CPU_Pct,Peak_RSS_MB,Total_Iterations,Status\n";
        ssize_t w = write(fd, hdr, strlen(hdr));
        (void)w;
    }

    char line[256];
    int len = snprintf(line, sizeof(line), "%s,%s,%d,%.2f,%zu,%.2f,%zu,%lu,%s\n",
                      cfg->timestamp_str, mode_to_string(cfg->mode),
                      cfg->num_workers, duration_sec, cfg->memory_mb,
                      avg_cpu_pct, peak_rss_mb, (unsigned long)total_iterations,
                      status ? status : "SUCCESS");

    if (len > 0) {
        ssize_t written = write(fd, line, (size_t)len);
        (void)written;
    }
    close(fd);
    return 0;
}

int report_generate_summary(const stress_config_t *cfg,
                            const process_group_t *pg,
                            const thread_group_t *tg,
                            const system_metrics_t *final_metrics,
                            double total_time_sec) {
    if (!cfg) return -1;

    int fd = open(cfg->summary_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        log_error("Report Error: open() failed for summary '%s': %s", cfg->summary_file, strerror(errno));
        return -1;
    }

    char buf[4096];
    int offset = 0;

    offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
        "================================================================================\n"
        "                  CPU AND MEMORY STRESS TESTING TOOL REPORT                     \n"
        "                           OSSP HACKATHON EDITION                               \n"
        "================================================================================\n"
        "Execution Timestamp : %s\n"
        "Workload Mode       : %s\n"
        "Configured Workers  : %d (%s)\n"
        "Configured Duration : %d seconds (Actual Elapsed: %.2f seconds)\n"
        "Execution Model     : %s\n"
        "IPC Mechanism       : %s\n",
        cfg->timestamp_str, mode_to_string(cfg->mode),
        cfg->num_workers, cfg->mode == MODE_THREADS ? "Pthreads" : "Processes",
        cfg->duration_sec, total_time_sec,
        exec_mode_to_string(cfg->exec_mode),
        ipc_to_string(cfg->ipc_type));

    if (cfg->mode == MODE_MEMORY || cfg->mode == MODE_COMBINED || cfg->mode == MODE_COW_DEMO) {
        offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
            "Target Memory Limit : %zu MB\n"
            "Access Pattern      : %s\n"
            "Allocation Method   : %s\n",
            cfg->memory_mb, pattern_to_string(cfg->mem_pattern),
            cfg->alloc_type == ALLOC_MMAP ? "mmap(MAP_ANON)" : "malloc()");
    }

    offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
        "\n--------------------------------------------------------------------------------\n"
        "RESOURCE UTILIZATION SUMMARY\n"
        "--------------------------------------------------------------------------------\n");

    if (final_metrics) {
        offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
            "Total CPU Utilization      : %.2f%%\n"
            "  -> User CPU Time         : %.2f%%\n"
            "  -> System (Kernel) Time  : %.2f%%\n"
            "Total System Memory        : %zu MB\n"
            "Used Memory at Finish      : %zu MB\n"
            "Available Memory at Finish : %zu MB\n"
            "Application Resident (RSS) : %zu MB\n"
            "Application Virtual Memory : %zu MB\n"
            "Minor Page Faults          : %ld (Frame allocations without disk I/O)\n"
            "Major Page Faults          : %ld (I/O paging operations)\n"
            "Total Completed Iterations : %lu\n"
            "Average Throughput         : %.2f iterations/sec\n",
            final_metrics->total_cpu_pct, final_metrics->user_cpu_pct, final_metrics->sys_cpu_pct,
            final_metrics->total_mem_mb, final_metrics->used_mem_mb, final_metrics->avail_mem_mb,
            final_metrics->app_vmrss_mb, final_metrics->app_vmsize_mb,
            final_metrics->min_faults, final_metrics->maj_faults,
            (unsigned long)final_metrics->total_iterations,
            final_metrics->throughput_iter_sec);
    }

    offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
        "\n--------------------------------------------------------------------------------\n"
        "WORKER SUBSYSTEM BREAKDOWN\n"
        "--------------------------------------------------------------------------------\n");

    if (cfg->mode == MODE_THREADS && tg) {
        for (int i = 0; i < tg->count; i++) {
            offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
                "Thread #%02d : %lu iterations (Duration: %.2fs)\n",
                i, (unsigned long)tg->workers[i].iterations, tg->workers[i].elapsed_sec);
        }
    } else if (pg) {
        for (int i = 0; i < pg->count; i++) {
            offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
                "Worker #%02d [PID %d] : %lu iterations (User: %.2fs, Sys: %.2fs, MinFlt: %ld, ExitCode: %d)\n",
                i, pg->pids[i],
                (unsigned long)pg->reports[i].iterations,
                pg->reports[i].user_time_sec, pg->reports[i].sys_time_sec,
                pg->reports[i].min_faults,
                WEXITSTATUS(pg->exit_statuses[i]));
        }
    }

    offset += snprintf(buf + offset, sizeof(buf) - (size_t)offset,
        "\n--------------------------------------------------------------------------------\n"
        "COURSE OUTCOME (CO1 - CO6) VERIFICATION CHECKPOINTS\n"
        "--------------------------------------------------------------------------------\n"
        "[CO1] OS Interface & Syscalls   : CLI parsed with getopt_long; logged via open/write/close\n"
        "[CO2] Process Management        : fork(), execv(), waitpid(), WIFEXITED child reaping\n"
        "[CO3] Inter-Process Comm (IPC)  : Anonymous pipes, named pipes (FIFO), POSIX SHM, signals\n"
        "[CO4] Virtual Memory & Paging   : Demand paging observed via minor page faults & RSS\n"
        "[CO5] File System Operations    : Low-level file I/O, CSV telemetry, structured reports\n"
        "[CO6] Multithreading & Sync     : POSIX pthreads, mutexes, condition vars, semaphores\n"
        "================================================================================\n"
        "Artifacts Generated:\n"
        "  - Raw Execution Log : %s\n"
        "  - CSV Time-Series   : %s\n"
        "  - Text Summary      : %s\n"
        "================================================================================\n",
        cfg->log_file, cfg->csv_file, cfg->summary_file);

    ssize_t written = write(fd, buf, (size_t)offset);
    close(fd);

    return (written > 0) ? 0 : -1;
}
