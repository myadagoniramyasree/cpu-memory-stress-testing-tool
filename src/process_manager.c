/**
 * @file process_manager.c
 * @brief Process creation, external worker exec, and child lifecycle management.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO2: Process Management (fork, exec, wait, waitpid, exit).
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include "process_manager.h"
#include "cpu_stress.h"
#include "memory_stress.h"
#include "monitor.h"
#include "logger.h"
#include "report.h"

int process_manager_run(const stress_config_t *cfg, process_group_t *pg, ipc_context_t *ipc) {
    if (!cfg || !pg || !ipc) return -1;
    memset(pg, 0, sizeof(*pg));
    pg->count = cfg->num_workers;

    monitor_context_t mon_ctx;
    monitor_init(&mon_ctx, cfg->sample_interval_ms);

    log_info("Process Manager: Spawning %d worker processes (Execution model: %s)",
             cfg->num_workers, exec_mode_to_string(cfg->exec_mode));

    for (int i = 0; i < cfg->num_workers; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            log_error("Process Manager Error: fork() failed for worker %d: %s", i, strerror(errno));
            process_manager_terminate_all(pg);
            return -1;
        }

        if (pid == 0) {
            /* === CHILD PROCESS === */
            /* Close unneeded pipe ends */
            if (ipc->type == IPC_PIPE) {
                close(ipc->anon_pipes[i][0]); /* Close read end in child */
                for (int j = 0; j < cfg->num_workers; j++) {
                    if (j != i) {
                        close(ipc->anon_pipes[j][0]);
                        close(ipc->anon_pipes[j][1]);
                    }
                }
            }

            /* CO1 / CO2: Demonstrate execv() launching external worker binary if requested */
            if (cfg->exec_mode == EXEC_MODE_EXEC) {
                char id_str[16], dur_str[16], mem_str[16], pipe_str[16];
                snprintf(id_str, sizeof(id_str), "%d", i);
                snprintf(dur_str, sizeof(dur_str), "%d", cfg->duration_sec);
                snprintf(mem_str, sizeof(mem_str), "%zu", cfg->memory_mb / (size_t)cfg->num_workers);
                snprintf(pipe_str, sizeof(pipe_str), "%d", ipc->type == IPC_PIPE ? ipc->anon_pipes[i][1] : -1);

                const char *mode_arg = (cfg->mode == MODE_MEMORY) ? "memory" : "cpu";
                char *worker_argv[] = {
                    "./stress_worker",
                    "--worker-id", id_str,
                    "--duration", dur_str,
                    "--mode", (char *)mode_arg,
                    "--memory", mem_str,
                    "--pipe-fd", pipe_str,
                    NULL
                };

                execv("./stress_worker", worker_argv);
                /* If execv returns, it failed */
                fprintf(stderr, "Child PID %d failed to exec './stress_worker': %s\n", getpid(), strerror(errno));
                _exit(127);
            }

            /* Direct fork() worker execution */
            worker_report_t rep;
            memset(&rep, 0, sizeof(rep));

            if (cfg->mode == MODE_MEMORY) {
                size_t mem_per_worker = cfg->memory_mb / (size_t)cfg->num_workers;
                if (mem_per_worker < 1) mem_per_worker = 1;
                memory_stress_worker_loop(i, mem_per_worker, cfg->duration_sec,
                                          cfg->mem_pattern, cfg->mem_op,
                                          cfg->alloc_type, ipc->shm_ptr, &rep);
            } else {
                cpu_stress_worker_loop(i, cfg->duration_sec, cfg->cpu_algo,
                                       ipc->shm_ptr, &rep);
            }

            /* Send telemetry back via IPC */
            if (ipc->type == IPC_PIPE) {
                ipc_pipe_send_report(ipc->anon_pipes[i][1], &rep);
                close(ipc->anon_pipes[i][1]);
            } else if (ipc->type == IPC_FIFO) {
                ipc_fifo_send_report(ipc->fifo_path, &rep);
            }

            _exit(0);
        } else {
            /* === PARENT PROCESS === */
            pg->pids[i] = pid;
            log_debug("Process Manager: Spawned child worker %d [PID: %d]", i, pid);
            if (ipc->type == IPC_PIPE) {
                close(ipc->anon_pipes[i][1]); /* Close child write end in parent */
            }
        }
    }

    /* Controller Monitoring Loop */
    monitor_render_header();

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    double elapsed = 0.0;
    uint64_t total_iterations = 0;

    while (elapsed < (double)cfg->duration_sec && !g_stop_requested) {
        usleep((useconds_t)cfg->sample_interval_ms * 1000);

        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                  (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;

        /* Aggregate iterations from SHM if available */
        if (ipc->type == IPC_SHM && ipc->shm_ptr) {
            total_iterations = 0;
            for (int i = 0; i < cfg->num_workers; i++) {
                total_iterations += ipc->shm_ptr->workers[i].iterations;
            }
        }

        system_metrics_t metrics;
        monitor_sample(&mon_ctx, &metrics, total_iterations);
        monitor_render_terminal_line(&metrics, elapsed, (double)cfg->duration_sec);

        /* Write sample to CSV report */
        report_csv_append(cfg->csv_file, &metrics, cfg, elapsed);

        /* Check if any child finished or died */
        int status;
        pid_t died;
        while ((died = waitpid(-1, &status, WNOHANG)) > 0) {
            for (int k = 0; k < pg->count; k++) {
                if (pg->pids[k] == died) {
                    pg->completed[k] = true;
                    pg->exit_statuses[k] = status;
                    break;
                }
            }
            log_debug("Process Manager: Worker PID %d exited during sample interval", died);
        }
    }

    if (g_stop_requested) {
        log_info("Process Manager: Stop signal received, notifying child workers...");
        for (int i = 0; i < pg->count; i++) {
            if (pg->pids[i] > 0 && !pg->completed[i]) {
                kill(pg->pids[i], SIGTERM);
            }
        }
        usleep(30000); /* Grace period for children to flush reports */
    }

    /* Signal termination if duration expired */
    if (ipc->type == IPC_SHM && ipc->shm_ptr) {
        ipc_shm_set_terminate(ipc->shm_ptr);
    }

    /* Read reports from anonymous pipes or FIFO if configured */
    if (ipc->type == IPC_PIPE) {
        for (int i = 0; i < cfg->num_workers; i++) {
            if (ipc_pipe_read_report(ipc->anon_pipes[i][0], &pg->reports[i]) == 0) {
                log_info("IPC: Received report from child PID %d: %s",
                         pg->reports[i].pid, pg->reports[i].status_msg);
            }
            close(ipc->anon_pipes[i][0]);
            ipc->anon_pipes[i][0] = -1;
        }
    } else if (ipc->type == IPC_FIFO && ipc->fifo_fd >= 0) {
        for (int i = 0; i < cfg->num_workers; i++) {
            if (ipc_fifo_read_report(ipc->fifo_fd, &pg->reports[i]) == 0) {
                log_info("IPC: Received report from child via FIFO: %s",
                         pg->reports[i].status_msg);
            }
        }
    }

    /* Reap all child processes */
    process_manager_reap_children(pg);

    return 0;
}

int process_manager_reap_children(process_group_t *pg) {
    if (!pg) return -1;

    for (int i = 0; i < pg->count; i++) {
        if (pg->pids[i] > 0 && !pg->completed[i]) {
            int status = 0;
            pid_t reaped = waitpid(pg->pids[i], &status, 0);

            if (reaped > 0) {
                pg->completed[i] = true;
                pg->exit_statuses[i] = status;

                if (WIFEXITED(status)) {
                    log_info("Process Manager: Reaped child PID %d (Normal Exit, code: %d)",
                             reaped, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    log_warn("Process Manager: Reaped child PID %d (Killed by signal: %d)",
                             reaped, WTERMSIG(status));
                }
            } else {
                log_error("Process Manager: waitpid() failed for PID %d: %s",
                          pg->pids[i], strerror(errno));
            }
        }
    }

    return 0;
}

void process_manager_terminate_all(process_group_t *pg) {
    if (!pg) return;

    for (int i = 0; i < pg->count; i++) {
        if (pg->pids[i] > 0 && !pg->completed[i]) {
            kill(pg->pids[i], SIGTERM);
            log_debug("Process Manager: Sent SIGTERM to child PID %d", pg->pids[i]);
        }
    }

    /* Short grace period, then reap */
    usleep(50000);
    process_manager_reap_children(pg);
}
