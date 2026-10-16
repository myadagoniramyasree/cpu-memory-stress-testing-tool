/**
 * @file config.c
 * @brief Configuration initialization, validation, and safety boundaries.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"

/* Query system available memory from /proc/meminfo */
size_t config_get_system_available_memory_mb(void) {
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
        return 1024; /* Fallback safe default: 1GB */
    }

    char buffer[2048];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read <= 0) {
        return 1024;
    }
    buffer[bytes_read] = '\0';

    char *line = strstr(buffer, "MemAvailable:");
    if (!line) {
        line = strstr(buffer, "MemFree:");
    }

    if (line) {
        unsigned long kb = 0;
        if (sscanf(line, "%*s %lu", &kb) == 1) {
            return (size_t)(kb / 1024);
        }
    }

    return 1024;
}

void config_init_defaults(stress_config_t *cfg) {
    if (!cfg) return;

    memset(cfg, 0, sizeof(*cfg));
    cfg->mode = MODE_CPU;
    
    /* Detect number of CPU cores for sensible worker default */
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0) nprocs = 2;
    cfg->num_workers = (int)(nprocs > 4 ? 4 : nprocs);

    cfg->duration_sec = DEFAULT_DURATION_SEC;
    cfg->memory_mb = DEFAULT_MEMORY_MB;
    cfg->mem_pattern = PATTERN_SEQUENTIAL;
    cfg->mem_op = MEM_OP_READ_WRITE;
    cfg->alloc_type = ALLOC_MALLOC;
    cfg->exec_mode = EXEC_MODE_FORK;
    cfg->ipc_type = IPC_PIPE;
    cfg->cpu_algo = CPU_ALGO_MATRIX;
    cfg->sample_interval_ms = DEFAULT_SAMPLE_INTERVAL_MS;
    cfg->dry_run = false;
    cfg->verbose = false;
    cfg->sync_threads = true;

    /* Timestamp for unique artifact filenames */
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    strftime(cfg->timestamp_str, sizeof(cfg->timestamp_str), "%Y%m%d_%H%M%S", &tm_buf);

    strncpy(cfg->output_dir, DEFAULT_OUTPUT_DIR, sizeof(cfg->output_dir) - 1);
    snprintf(cfg->log_file, sizeof(cfg->log_file), "%s/stress_%s.log", cfg->output_dir, cfg->timestamp_str);
    snprintf(cfg->csv_file, sizeof(cfg->csv_file), "%s/stress_%s.csv", cfg->output_dir, cfg->timestamp_str);
    snprintf(cfg->summary_file, sizeof(cfg->summary_file), "%s/stress_%s_summary.txt", cfg->output_dir, cfg->timestamp_str);
}

int config_validate(stress_config_t *cfg, char *err_buf, size_t err_len) {
    if (!cfg) {
        if (err_buf) snprintf(err_buf, err_len, "Configuration pointer is NULL");
        return -1;
    }

    /* Worker count check */
    if (cfg->num_workers < MIN_WORKERS || cfg->num_workers > MAX_WORKERS) {
        if (err_buf) {
            snprintf(err_buf, err_len, "Worker count (%d) must be between %d and %d",
                     cfg->num_workers, MIN_WORKERS, MAX_WORKERS);
        }
        return -1;
    }

    /* Duration check */
    if (cfg->duration_sec < MIN_DURATION_SEC || cfg->duration_sec > MAX_DURATION_SEC) {
        if (err_buf) {
            snprintf(err_buf, err_len, "Duration (%d sec) must be between %d and %d seconds",
                     cfg->duration_sec, MIN_DURATION_SEC, MAX_DURATION_SEC);
        }
        return -1;
    }

    /* Sampling interval check */
    if (cfg->sample_interval_ms < MIN_SAMPLE_INTERVAL_MS || cfg->sample_interval_ms > MAX_SAMPLE_INTERVAL_MS) {
        if (err_buf) {
            snprintf(err_buf, err_len, "Sampling interval (%d ms) must be between %d and %d ms",
                     cfg->sample_interval_ms, MIN_SAMPLE_INTERVAL_MS, MAX_SAMPLE_INTERVAL_MS);
        }
        return -1;
    }

    /* Memory limit safety check */
    if (cfg->mode == MODE_MEMORY || cfg->mode == MODE_COMBINED || cfg->mode == MODE_COW_DEMO) {
        if (cfg->memory_mb < MIN_MEMORY_MB) {
            if (err_buf) {
                snprintf(err_buf, err_len, "Memory size must be at least %d MB", MIN_MEMORY_MB);
            }
            return -1;
        }

        size_t avail_mb = config_get_system_available_memory_mb();
        size_t safe_ceiling = (avail_mb * 80) / 100; /* Max 80% of available memory */
        if (safe_ceiling > ABSOLUTE_MAX_MEMORY_MB) {
            safe_ceiling = ABSOLUTE_MAX_MEMORY_MB;
        }

        if (cfg->memory_mb > safe_ceiling) {
            if (err_buf) {
                snprintf(err_buf, err_len,
                         "Requested memory (%zu MB) exceeds safe operating ceiling (%zu MB, 80%% of available RAM)",
                         cfg->memory_mb, safe_ceiling);
            }
            return -1;
        }
    }

    return 0;
}

const char *mode_to_string(stress_mode_t mode) {
    switch (mode) {
        case MODE_CPU:        return "CPU-Stress (Processes)";
        case MODE_MEMORY:     return "Memory-Stress (Virtual Memory)";
        case MODE_COMBINED:   return "Combined (CPU + Memory)";
        case MODE_THREADS:    return "Pthreads-Stress (Threads)";
        case MODE_RACE_DEMO:  return "Race-Condition-Demo (CO6)";
        case MODE_COW_DEMO:   return "Copy-On-Write-Demo (CO4)";
        default:              return "Unknown";
    }
}

const char *pattern_to_string(mem_pattern_t pattern) {
    switch (pattern) {
        case PATTERN_SEQUENTIAL: return "Sequential";
        case PATTERN_RANDOM:     return "Random";
        default:                 return "Unknown";
    }
}

const char *ipc_to_string(ipc_type_t ipc) {
    switch (ipc) {
        case IPC_PIPE: return "Anonymous Pipe";
        case IPC_FIFO: return "Named Pipe (FIFO)";
        case IPC_SHM:  return "Shared Memory (POSIX SHM)";
        default:       return "Unknown";
    }
}

const char *exec_mode_to_string(exec_mode_t exec_mode) {
    switch (exec_mode) {
        case EXEC_MODE_FORK: return "fork() [In-Process]";
        case EXEC_MODE_EXEC: return "fork() + execv() [External Worker]";
        default:             return "Unknown";
    }
}

void config_print(const stress_config_t *cfg) {
    if (!cfg) return;

    printf("\n=======================================================\n");
    printf("   CPU & MEMORY STRESS TESTING TOOL - CONFIGURATION    \n");
    printf("=======================================================\n");
    printf("  Workload Mode       : %s\n", mode_to_string(cfg->mode));
    printf("  Worker Count        : %d\n", cfg->num_workers);
    printf("  Execution Model     : %s\n", exec_mode_to_string(cfg->exec_mode));
    printf("  IPC Mechanism       : %s\n", ipc_to_string(cfg->ipc_type));
    printf("  Workload Duration   : %d seconds\n", cfg->duration_sec);
    if (cfg->mode == MODE_MEMORY || cfg->mode == MODE_COMBINED || cfg->mode == MODE_COW_DEMO) {
        printf("  Memory Target       : %zu MB\n", cfg->memory_mb);
        printf("  Access Pattern      : %s\n", pattern_to_string(cfg->mem_pattern));
        printf("  Allocation Method   : %s\n", cfg->alloc_type == ALLOC_MMAP ? "mmap(MAP_ANON)" : "malloc()");
    }
    printf("  Sampling Interval   : %d ms\n", cfg->sample_interval_ms);
    printf("  Output Directory    : %s\n", cfg->output_dir);
    printf("  Log File            : %s\n", cfg->log_file);
    printf("  CSV Dataset         : %s\n", cfg->csv_file);
    printf("  Summary Report      : %s\n", cfg->summary_file);
    printf("  Dry Run Mode        : %s\n", cfg->dry_run ? "YES (Simulation Only)" : "NO (Live Execution)");
    printf("=======================================================\n\n");
}
