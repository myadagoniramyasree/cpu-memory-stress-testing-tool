/**
 * @file cli.c
 * @brief Command-line interface, argument parsing, help, and dry-run display.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1: Operating System Interface and Command Execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "cli.h"
#include "config.h"

static struct option long_options[] = {
    {"mode",        required_argument, 0, 'm'},
    {"workers",     required_argument, 0, 'w'},
    {"time",        required_argument, 0, 't'},
    {"memory",      required_argument, 0, 'b'},
    {"pattern",     required_argument, 0, 'p'},
    {"operation",   required_argument, 0, 'o'},
    {"alloc",       required_argument, 0, 'A'},
    {"algo",        required_argument, 0, 'a'},
    {"ipc",         required_argument, 0, 'i'},
    {"exec",        required_argument, 0, 'e'},
    {"sample",      required_argument, 0, 's'},
    {"outdir",      required_argument, 0, 'O'},
    {"dry-run",     no_argument,       0, 'd'},
    {"verbose",     no_argument,       0, 'v'},
    {"no-sync",     no_argument,       0, 'N'},
    {"help",        no_argument,       0, 'h'},
    {"version",     no_argument,       0, 'V'},
    {0, 0, 0, 0}
};

int cli_parse(int argc, char *argv[], stress_config_t *cfg) {
    if (!cfg) return -1;

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "m:w:t:b:p:o:A:a:i:e:s:O:dvhVN", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'm':
                if (strcmp(optarg, "cpu") == 0) {
                    cfg->mode = MODE_CPU;
                } else if (strcmp(optarg, "memory") == 0 || strcmp(optarg, "mem") == 0) {
                    cfg->mode = MODE_MEMORY;
                } else if (strcmp(optarg, "combined") == 0) {
                    cfg->mode = MODE_COMBINED;
                } else if (strcmp(optarg, "threads") == 0) {
                    cfg->mode = MODE_THREADS;
                } else if (strcmp(optarg, "race-demo") == 0) {
                    cfg->mode = MODE_RACE_DEMO;
                } else if (strcmp(optarg, "cow-demo") == 0) {
                    cfg->mode = MODE_COW_DEMO;
                } else {
                    fprintf(stderr, "Error: Unknown mode '%s'. Choose from: cpu, memory, combined, threads, race-demo, cow-demo.\n", optarg);
                    return -1;
                }
                break;

            case 'w':
                cfg->num_workers = atoi(optarg);
                break;

            case 't':
                cfg->duration_sec = atoi(optarg);
                break;

            case 'b':
                cfg->memory_mb = (size_t)strtoul(optarg, NULL, 10);
                break;

            case 'p':
                if (strcmp(optarg, "sequential") == 0 || strcmp(optarg, "seq") == 0) {
                    cfg->mem_pattern = PATTERN_SEQUENTIAL;
                } else if (strcmp(optarg, "random") == 0 || strcmp(optarg, "rand") == 0) {
                    cfg->mem_pattern = PATTERN_RANDOM;
                } else {
                    fprintf(stderr, "Error: Unknown memory pattern '%s'. Choose: sequential, random.\n", optarg);
                    return -1;
                }
                break;

            case 'o':
                if (strcmp(optarg, "rw") == 0) {
                    cfg->mem_op = MEM_OP_READ_WRITE;
                } else if (strcmp(optarg, "write") == 0) {
                    cfg->mem_op = MEM_OP_WRITE_ONLY;
                } else if (strcmp(optarg, "read") == 0) {
                    cfg->mem_op = MEM_OP_READ_ONLY;
                } else {
                    fprintf(stderr, "Error: Unknown memory operation '%s'. Choose: rw, write, read.\n", optarg);
                    return -1;
                }
                break;

            case 'A':
                if (strcmp(optarg, "mmap") == 0) {
                    cfg->alloc_type = ALLOC_MMAP;
                } else if (strcmp(optarg, "malloc") == 0) {
                    cfg->alloc_type = ALLOC_MALLOC;
                } else {
                    fprintf(stderr, "Error: Unknown alloc type '%s'. Choose: malloc, mmap.\n", optarg);
                    return -1;
                }
                break;

            case 'a':
                if (strcmp(optarg, "matrix") == 0) {
                    cfg->cpu_algo = CPU_ALGO_MATRIX;
                } else if (strcmp(optarg, "hash") == 0) {
                    cfg->cpu_algo = CPU_ALGO_HASH;
                } else if (strcmp(optarg, "sqrt") == 0) {
                    cfg->cpu_algo = CPU_ALGO_SQRT;
                } else {
                    fprintf(stderr, "Error: Unknown CPU algorithm '%s'. Choose: matrix, hash, sqrt.\n", optarg);
                    return -1;
                }
                break;

            case 'i':
                if (strcmp(optarg, "pipe") == 0) {
                    cfg->ipc_type = IPC_PIPE;
                } else if (strcmp(optarg, "fifo") == 0) {
                    cfg->ipc_type = IPC_FIFO;
                } else if (strcmp(optarg, "shm") == 0) {
                    cfg->ipc_type = IPC_SHM;
                } else {
                    fprintf(stderr, "Error: Unknown IPC type '%s'. Choose: pipe, fifo, shm.\n", optarg);
                    return -1;
                }
                break;

            case 'e':
                if (strcmp(optarg, "fork") == 0) {
                    cfg->exec_mode = EXEC_MODE_FORK;
                } else if (strcmp(optarg, "exec") == 0) {
                    cfg->exec_mode = EXEC_MODE_EXEC;
                } else {
                    fprintf(stderr, "Error: Unknown exec mode '%s'. Choose: fork, exec.\n", optarg);
                    return -1;
                }
                break;

            case 's':
                cfg->sample_interval_ms = atoi(optarg);
                break;

            case 'O':
                strncpy(cfg->output_dir, optarg, sizeof(cfg->output_dir) - 1);
                /* Refresh file paths */
                snprintf(cfg->log_file, sizeof(cfg->log_file), "%s/stress_%s.log", cfg->output_dir, cfg->timestamp_str);
                snprintf(cfg->csv_file, sizeof(cfg->csv_file), "%s/stress_%s.csv", cfg->output_dir, cfg->timestamp_str);
                snprintf(cfg->summary_file, sizeof(cfg->summary_file), "%s/stress_%s_summary.txt", cfg->output_dir, cfg->timestamp_str);
                break;

            case 'd':
                cfg->dry_run = true;
                break;

            case 'v':
                cfg->verbose = true;
                break;

            case 'N':
                cfg->sync_threads = false; /* Unsynchronized race condition */
                break;

            case 'h':
                cli_print_help(argv[0]);
                exit(EXIT_SUCCESS);

            case 'V':
                cli_print_version();
                exit(EXIT_SUCCESS);

            default:
                cli_print_help(argv[0]);
                return -1;
        }
    }

    return 0;
}

void cli_print_version(void) {
    printf("%s version %s (OSSP Hackathon Edition)\n", TOOL_NAME, TOOL_VERSION);
    printf("Built for Linux/POSIX. Implements CO1-CO6.\n");
}

void cli_print_help(const char *prog_name) {
    printf("========================================================================\n");
    printf("       %s - CPU and Memory Stress Testing Tool (OSSP Hackathon)\n", TOOL_NAME);
    printf("========================================================================\n");
    printf("USAGE:\n");
    printf("  %s [OPTIONS]\n\n", prog_name);

    printf("MODES (-m, --mode):\n");
    printf("  cpu          Process-based CPU stress testing [CO2]\n");
    printf("  memory       Virtual memory stress and page-touching [CO4]\n");
    printf("  combined     Concurrent CPU and Memory stress testing\n");
    printf("  threads      POSIX pthreads CPU/Memory stress testing [CO6]\n");
    printf("  race-demo    Demonstration of Race Conditions vs Mutex locking [CO6]\n");
    printf("  cow-demo     Demand Paging and Copy-On-Write (COW) experiment [CO4]\n\n");

    printf("WORKLOAD OPTIONS:\n");
    printf("  -w, --workers <N>      Number of worker processes or threads (default: %d, max: %d)\n", DEFAULT_WORKERS, MAX_WORKERS);
    printf("  -t, --time <seconds>   Duration of stress test in seconds (default: %d)\n", DEFAULT_DURATION_SEC);
    printf("  -b, --memory <MB>      Memory allocation limit in MB (default: %d MB)\n", DEFAULT_MEMORY_MB);
    printf("  -p, --pattern <seq|rand> Memory access pattern (sequential, random)\n");
    printf("  -o, --operation <op>   Memory operation: rw (read-write), write, read\n");
    printf("  -A, --alloc <type>     Allocation API: malloc or mmap\n");
    printf("  -a, --algo <algo>      CPU algorithm: matrix, hash, sqrt\n\n");

    printf("SYSTEMS PROGRAMMING & OS OPTIONS:\n");
    printf("  -e, --exec <fork|exec> Worker model: direct fork() or fork()+execv() [CO1/CO2]\n");
    printf("  -i, --ipc <type>       IPC channel: pipe (anonymous), fifo, shm [CO3]\n");
    printf("  -s, --sample <ms>      Monitoring sample interval in ms (default: %d)\n", DEFAULT_SAMPLE_INTERVAL_MS);
    printf("  --no-sync              Disable mutex in race-demo to observe data corruption [CO6]\n");
    printf("  -d, --dry-run          Simulate configuration without stressing hardware\n");
    printf("  -v, --verbose          Enable detailed log and runtime output\n");
    printf("  -O, --outdir <dir>     Output directory for logs and CSV (default: '%s')\n", DEFAULT_OUTPUT_DIR);
    printf("  -h, --help             Display this help menu and exit\n");
    printf("  -V, --version          Display version information\n\n");

    printf("EXAMPLES:\n");
    printf("  # 1. CPU stress with 4 process workers for 10 seconds via anonymous pipes:\n");
    printf("       %s -m cpu -w 4 -t 10 -i pipe\n\n", prog_name);
    printf("  # 2. Memory stress with 512 MB sequential write using mmap for 15 seconds:\n");
    printf("       %s -m memory -b 512 -p sequential -A mmap -t 15\n\n", prog_name);
    printf("  # 3. Combined CPU & Memory stress using external worker executables (execv):\n");
    printf("       %s -m combined -w 4 -b 256 -e exec -t 10\n\n", prog_name);
    printf("  # 4. Multithreaded workload with POSIX pthreads and mutex coordination:\n");
    printf("       %s -m threads -w 4 -t 10\n\n", prog_name);
    printf("  # 5. Observe race condition bug (unsynchronized counters):\n");
    printf("       %s -m race-demo -w 8 --no-sync\n\n", prog_name);
    printf("  # 6. Verify mutex-synchronized race-free counter correctness:\n");
    printf("       %s -m race-demo -w 8\n\n", prog_name);
    printf("  # 7. Observe Copy-On-Write (COW) and Demand Paging page faults:\n");
    printf("       %s -m cow-demo -b 128\n\n", prog_name);
    printf("  # 8. Dry-run safety inspection:\n");
    printf("       %s -m combined -w 8 -b 1024 --dry-run\n\n", prog_name);
    printf("========================================================================\n");
}

void cli_print_dry_run(const stress_config_t *cfg) {
    size_t avail_ram = config_get_system_available_memory_mb();
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    long page_size = sysconf(_SC_PAGESIZE);

    printf("\n=======================================================\n");
    printf("         DRY RUN SIMULATION & SAFETY ANALYSIS          \n");
    printf("=======================================================\n");
    printf("  [System Topology]\n");
    printf("    Logical CPU Cores Online : %ld\n", nprocs);
    printf("    System Virtual Page Size : %ld bytes (%ld KB)\n", page_size, page_size / 1024);
    printf("    Available Physical RAM   : %zu MB\n\n", avail_ram);

    printf("  [Planned Workload]\n");
    printf("    Selected Mode            : %s\n", mode_to_string(cfg->mode));
    printf("    Workers Spawned          : %d (%s)\n", cfg->num_workers,
           cfg->mode == MODE_THREADS ? "POSIX pthreads" : "Processes via fork()");
    printf("    Execution Binary Model   : %s\n", exec_mode_to_string(cfg->exec_mode));
    printf("    IPC Mechanism            : %s\n", ipc_to_string(cfg->ipc_type));
    printf("    Duration Planned         : %d seconds\n", cfg->duration_sec);

    if (cfg->mode == MODE_MEMORY || cfg->mode == MODE_COMBINED || cfg->mode == MODE_COW_DEMO) {
        size_t num_pages = (cfg->memory_mb * 1024 * 1024) / (size_t)page_size;
        printf("    Memory Target Allocation : %zu MB (%zu pages)\n", cfg->memory_mb, num_pages);
        printf("    Memory Percent of RAM    : %.1f%%\n", (double)cfg->memory_mb * 100.0 / (double)avail_ram);
        printf("    Access Pattern           : %s\n", pattern_to_string(cfg->mem_pattern));
        printf("    Syscall Strategy         : %s\n", cfg->alloc_type == ALLOC_MMAP ? "sys_mmap(MAP_ANON|MAP_PRIVATE)" : "sys_brk/malloc()");
    }

    printf("\n  [Operating System & Kernel Verification]\n");
    printf("    Process Reaping Method   : waitpid(pid, &status, 0)\n");
    printf("    System Metric Interfaces : /proc/stat, /proc/meminfo, /proc/self/status\n");
    printf("    Low-Level File I/O       : open(O_WRONLY|O_CREAT), write(), lseek(), close()\n");
    printf("    Safety Boundaries Check  : PASSED. No destructive actions will be executed.\n");
    printf("=======================================================\n");
    printf("Dry-run complete. Run without -d or --dry-run to execute.\n\n");
}
