/**
 * @file process_manager.h
 * @brief Process creation, external worker exec, and child lifecycle management.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO2: Process Management (fork, exec, wait, waitpid, exit).
 */

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include "config.h"
#include "ipc.h"

/* Process manager state and child tracking */
typedef struct {
    int count;
    pid_t pids[MAX_WORKERS];
    int exit_statuses[MAX_WORKERS];
    bool completed[MAX_WORKERS];
    worker_report_t reports[MAX_WORKERS];
} process_group_t;

/* Run process-based stress tests */
int process_manager_run(const stress_config_t *cfg, process_group_t *pg, ipc_context_t *ipc);

/* Wait for all child processes and reap them */
int process_manager_reap_children(process_group_t *pg);

/* Kill all worker processes gracefully via SIGTERM */
void process_manager_terminate_all(process_group_t *pg);

#endif /* PROCESS_MANAGER_H */
