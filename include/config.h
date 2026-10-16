/**
 * @file config.h
 * @brief Configuration structures, constants, and validation prototypes.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1 (OS Interface), CO2 (Process Management), and Safety Controls.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define TOOL_NAME "stress_tool"
#define TOOL_VERSION "1.0.0"

/* Safety Limits */
#define DEFAULT_WORKERS 2
#define MIN_WORKERS 1
#define MAX_WORKERS 64

#define DEFAULT_DURATION_SEC 10
#define MIN_DURATION_SEC 1
#define MAX_DURATION_SEC 600

#define DEFAULT_MEMORY_MB 256
#define MIN_MEMORY_MB 1
#define ABSOLUTE_MAX_MEMORY_MB 8192

#define DEFAULT_SAMPLE_INTERVAL_MS 1000
#define MIN_SAMPLE_INTERVAL_MS 100
#define MAX_SAMPLE_INTERVAL_MS 10000

#define MAX_PATH_LEN 1024
#define DEFAULT_OUTPUT_DIR "results"

/* Workload Modes */
typedef enum {
    MODE_CPU = 0,        /* CO2: Process-based CPU stress */
    MODE_MEMORY,         /* CO4: Memory stress and virtual memory tests */
    MODE_COMBINED,       /* Both CPU and Memory workloads concurrently */
    MODE_THREADS,        /* CO6: Thread-based CPU/Memory workloads */
    MODE_RACE_DEMO,      /* CO6: Race condition vs Mutex synchronization demo */
    MODE_COW_DEMO        /* CO4: Copy-On-Write and Demand Paging experiment */
} stress_mode_t;

/* Memory Access Patterns */
typedef enum {
    PATTERN_SEQUENTIAL = 0,
    PATTERN_RANDOM
} mem_pattern_t;

/* Memory Operations */
typedef enum {
    MEM_OP_READ_WRITE = 0,
    MEM_OP_WRITE_ONLY,
    MEM_OP_READ_ONLY
} mem_op_t;

/* Memory Allocation Type */
typedef enum {
    ALLOC_MALLOC = 0,
    ALLOC_MMAP
} mem_alloc_type_t;

/* Worker Execution Mechanism (CO1/CO2) */
typedef enum {
    EXEC_MODE_FORK = 0,  /* Direct function execution in child fork */
    EXEC_MODE_EXEC       /* fork() followed by execv() of external worker binary */
} exec_mode_t;

/* IPC Mechanism Selection (CO3) */
typedef enum {
    IPC_PIPE = 0,        /* Anonymous pipes */
    IPC_FIFO,            /* Named pipes (FIFOs) */
    IPC_SHM              /* POSIX shared memory */
} ipc_type_t;

/* CPU Calculation Algorithm */
typedef enum {
    CPU_ALGO_MATRIX = 0, /* Matrix multiplication (Cache & ALU intensive) */
    CPU_ALGO_HASH,       /* Cryptographic-style 64-bit integer mixing */
    CPU_ALGO_SQRT        /* Floating point square roots and transcendentals */
} cpu_algo_t;

/* Complete Runtime Configuration */
typedef struct {
    stress_mode_t mode;
    int num_workers;
    int duration_sec;
    size_t memory_mb;
    mem_pattern_t mem_pattern;
    mem_op_t mem_op;
    mem_alloc_type_t alloc_type;
    exec_mode_t exec_mode;
    ipc_type_t ipc_type;
    cpu_algo_t cpu_algo;
    int sample_interval_ms;
    bool dry_run;
    bool verbose;
    bool sync_threads;   /* For thread race demo: true = mutex locked, false = raw race */
    char output_dir[MAX_PATH_LEN];
    char log_file[MAX_PATH_LEN];
    char csv_file[MAX_PATH_LEN];
    char summary_file[MAX_PATH_LEN];
    char timestamp_str[64];
} stress_config_t;

/* Function Prototypes */
void config_init_defaults(stress_config_t *cfg);
int config_validate(stress_config_t *cfg, char *err_buf, size_t err_len);
void config_print(const stress_config_t *cfg);
size_t config_get_system_available_memory_mb(void);
const char *mode_to_string(stress_mode_t mode);
const char *pattern_to_string(mem_pattern_t pattern);
const char *ipc_to_string(ipc_type_t ipc);
const char *exec_mode_to_string(exec_mode_t exec_mode);

#endif /* CONFIG_H */
