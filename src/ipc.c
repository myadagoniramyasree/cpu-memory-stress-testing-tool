/**
 * @file ipc.c
 * @brief Inter-Process Communication (Pipes, FIFOs, Signals, Shared Memory).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO3: Inter-Process Communication.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "ipc.h"
#include "logger.h"

volatile sig_atomic_t g_stop_requested = 0;

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGALRM) {
        g_stop_requested = 1;
    }
}

void ipc_setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* Let slow syscalls interrupt cleanly */

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
}

void ipc_send_signal_to_group(pid_t pgid, int sig) {
    if (pgid > 0) {
        kill(-pgid, sig);
    }
}

int ipc_init(ipc_context_t *ctx, ipc_type_t type, int num_workers) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->type = type;
    ctx->num_workers = num_workers;
    ctx->fifo_fd = -1;
    ctx->shm_fd = -1;

    /* 1. Anonymous Pipes (CO3) */
    if (type == IPC_PIPE) {
        for (int i = 0; i < num_workers; i++) {
            if (pipe(ctx->anon_pipes[i]) < 0) {
                log_error("IPC Error: pipe() failed for worker %d: %s", i, strerror(errno));
                return -1;
            }
            log_debug("IPC: Created anonymous pipe for worker %d [read=%d, write=%d]",
                      i, ctx->anon_pipes[i][0], ctx->anon_pipes[i][1]);
        }
    }

    /* 2. Named Pipes / FIFOs (CO3) */
    if (type == IPC_FIFO) {
        snprintf(ctx->fifo_path, sizeof(ctx->fifo_path), "%s_%d", FIFO_PATH_PREFIX, getpid());
        unlink(ctx->fifo_path); /* Remove if existing */
        if (mkfifo(ctx->fifo_path, 0666) < 0) {
            log_error("IPC Error: mkfifo() failed for '%s': %s", ctx->fifo_path, strerror(errno));
            return -1;
        }
        ctx->fifo_fd = open(ctx->fifo_path, O_RDWR | O_NONBLOCK);
        if (ctx->fifo_fd < 0) {
            log_error("IPC Error: open() failed on FIFO '%s': %s", ctx->fifo_path, strerror(errno));
            return -1;
        }
        log_info("IPC: Created Named Pipe (FIFO) at '%s' [FD=%d]", ctx->fifo_path, ctx->fifo_fd);
    }

    /* 3. POSIX Shared Memory (CO3) */
    if (type == IPC_SHM) {
        snprintf(ctx->shm_name, sizeof(ctx->shm_name), "%s_%d", SHM_NAME_PREFIX, getpid());
        shm_unlink(ctx->shm_name); /* Remove stale segment if any */

        ctx->shm_fd = shm_open(ctx->shm_name, O_CREAT | O_RDWR, 0666);
        if (ctx->shm_fd < 0) {
            log_error("IPC Error: shm_open() failed for '%s': %s", ctx->shm_name, strerror(errno));
            return -1;
        }

        ctx->shm_size = sizeof(shm_metrics_t);
        if (ftruncate(ctx->shm_fd, (off_t)ctx->shm_size) < 0) {
            log_error("IPC Error: ftruncate() failed for SHM: %s", strerror(errno));
            close(ctx->shm_fd);
            ctx->shm_fd = -1;
            return -1;
        }

        ctx->shm_ptr = (shm_metrics_t *)mmap(NULL, ctx->shm_size,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED, ctx->shm_fd, 0);
        if (ctx->shm_ptr == MAP_FAILED) {
            log_error("IPC Error: mmap() failed for SHM: %s", strerror(errno));
            ctx->shm_ptr = NULL;
            close(ctx->shm_fd);
            ctx->shm_fd = -1;
            return -1;
        }

        memset(ctx->shm_ptr, 0, ctx->shm_size);
        ctx->shm_ptr->total_workers = num_workers;
        log_info("IPC: Allocated POSIX Shared Memory segment '%s' (%zu bytes)",
                 ctx->shm_name, ctx->shm_size);
    }

    return 0;
}

void ipc_cleanup(ipc_context_t *ctx) {
    if (!ctx) return;

    /* Close anonymous pipes */
    if (ctx->type == IPC_PIPE) {
        for (int i = 0; i < ctx->num_workers; i++) {
            if (ctx->anon_pipes[i][0] >= 0) close(ctx->anon_pipes[i][0]);
            if (ctx->anon_pipes[i][1] >= 0) close(ctx->anon_pipes[i][1]);
        }
    }

    /* Unlink and close FIFO */
    if (ctx->type == IPC_FIFO) {
        if (ctx->fifo_fd >= 0) {
            close(ctx->fifo_fd);
            ctx->fifo_fd = -1;
        }
        if (ctx->fifo_path[0] != '\0') {
            unlink(ctx->fifo_path);
            log_debug("IPC: Unlinked FIFO '%s'", ctx->fifo_path);
        }
    }

    /* Unmap and unlink Shared Memory */
    if (ctx->type == IPC_SHM) {
        if (ctx->shm_ptr && ctx->shm_ptr != MAP_FAILED) {
            munmap(ctx->shm_ptr, ctx->shm_size);
            ctx->shm_ptr = NULL;
        }
        if (ctx->shm_fd >= 0) {
            close(ctx->shm_fd);
            ctx->shm_fd = -1;
        }
        if (ctx->shm_name[0] != '\0') {
            shm_unlink(ctx->shm_name);
            log_debug("IPC: Unlinked SHM '%s'", ctx->shm_name);
        }
    }
}

int ipc_pipe_send_report(int write_fd, const worker_report_t *report) {
    if (write_fd < 0 || !report) return -1;
    ssize_t bytes = write(write_fd, report, sizeof(*report));
    return (bytes == sizeof(*report)) ? 0 : -1;
}

int ipc_pipe_read_report(int read_fd, worker_report_t *report) {
    if (read_fd < 0 || !report) return -1;
    ssize_t bytes = read(read_fd, report, sizeof(*report));
    return (bytes == sizeof(*report)) ? 0 : -1;
}

int ipc_fifo_send_report(const char *fifo_path, const worker_report_t *report) {
    if (!fifo_path || !report) return -1;
    int fd = open(fifo_path, O_WRONLY);
    if (fd < 0) return -1;
    ssize_t written = write(fd, report, sizeof(*report));
    close(fd);
    return (written == sizeof(*report)) ? 0 : -1;
}

int ipc_fifo_read_report(int fifo_fd, worker_report_t *report) {
    if (fifo_fd < 0 || !report) return -1;
    ssize_t bytes = read(fifo_fd, report, sizeof(*report));
    return (bytes == sizeof(*report)) ? 0 : -1;
}

int ipc_shm_update_worker(shm_metrics_t *shm, int worker_id, pid_t pid, uint64_t iter, uint64_t bytes) {
    if (!shm || worker_id < 0 || worker_id >= MAX_WORKERS) return -1;
    shm->workers[worker_id].pid = pid;
    shm->workers[worker_id].iterations = iter;
    shm->workers[worker_id].bytes_accessed = bytes;
    shm->workers[worker_id].heartbeat_timestamp = (uint64_t)time(NULL);
    shm->workers[worker_id].active = true;
    return 0;
}

void ipc_shm_set_terminate(shm_metrics_t *shm) {
    if (shm) {
        shm->terminate_flag = true;
    }
}
