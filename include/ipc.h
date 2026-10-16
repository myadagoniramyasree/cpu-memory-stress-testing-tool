/**
 * @file ipc.h
 * @brief Inter-Process Communication (Pipes, FIFOs, Signals, Shared Memory).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO3: Inter-Process Communication.
 */

#ifndef IPC_H
#define IPC_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include "config.h"

#define SHM_NAME_PREFIX "/stress_tool_shm"
#define FIFO_PATH_PREFIX "/tmp/stress_tool_fifo"

/* Global stop signal flag set by signal handlers */
extern volatile sig_atomic_t g_stop_requested;

/* Worker telemetry metrics reported back to controller */
typedef struct {
    pid_t pid;
    int worker_id;
    uint64_t iterations;
    double elapsed_sec;
    double user_time_sec;
    double sys_time_sec;
    long min_faults;
    long maj_faults;
    size_t memory_bytes;
    int exit_code;
    char status_msg[128];
} worker_report_t;

/* Real-time shared memory buffer for live telemetry */
typedef struct {
    int active_workers;
    int total_workers;
    volatile bool terminate_flag;
    struct {
        pid_t pid;
        uint64_t iterations;
        uint64_t bytes_accessed;
        uint64_t heartbeat_timestamp;
        bool active;
    } workers[MAX_WORKERS];
} shm_metrics_t;

/* IPC Context */
typedef struct {
    ipc_type_t type;
    int num_workers;
    int anon_pipes[MAX_WORKERS][2]; /* [0]=read, [1]=write */
    char fifo_path[MAX_PATH_LEN];
    int fifo_fd;
    char shm_name[64];
    int shm_fd;
    shm_metrics_t *shm_ptr;
    size_t shm_size;
} ipc_context_t;

/* IPC Lifecycle Functions */
int ipc_init(ipc_context_t *ctx, ipc_type_t type, int num_workers);
void ipc_cleanup(ipc_context_t *ctx);

/* Anonymous Pipe API */
int ipc_pipe_send_report(int write_fd, const worker_report_t *report);
int ipc_pipe_read_report(int read_fd, worker_report_t *report);

/* FIFO API */
int ipc_fifo_send_report(const char *fifo_path, const worker_report_t *report);
int ipc_fifo_read_report(int fifo_fd, worker_report_t *report);

/* Shared Memory API */
int ipc_shm_update_worker(shm_metrics_t *shm, int worker_id, pid_t pid, uint64_t iter, uint64_t bytes);
void ipc_shm_set_terminate(shm_metrics_t *shm);

/* Signal Management (CO3) */
void ipc_setup_signal_handlers(void);
void ipc_send_signal_to_group(pid_t pgid, int sig);

#endif /* IPC_H */
