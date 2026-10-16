/**
 * @file worker_main.c
 * @brief Standalone worker program launched via execv() by the main controller.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO1 & CO2: External worker execution via fork() + execv().
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "cpu_stress.h"
#include "memory_stress.h"
#include "ipc.h"

int main(int argc, char *argv[]) {
    int worker_id = 0;
    int duration_sec = 10;
    char mode[32] = "cpu";
    size_t memory_mb = 128;
    int pipe_fd = -1;

    static struct option long_options[] = {
        {"worker-id", required_argument, 0, 'w'},
        {"duration",  required_argument, 0, 'd'},
        {"mode",      required_argument, 0, 'm'},
        {"memory",    required_argument, 0, 'b'},
        {"pipe-fd",   required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "w:d:m:b:p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'w': worker_id = atoi(optarg); break;
            case 'd': duration_sec = atoi(optarg); break;
            case 'm': strncpy(mode, optarg, sizeof(mode) - 1); break;
            case 'b': memory_mb = (size_t)strtoul(optarg, NULL, 10); break;
            case 'p': pipe_fd = atoi(optarg); break;
            default: break;
        }
    }

    /* Register signal handlers for clean interruption */
    ipc_setup_signal_handlers();

    worker_report_t report;
    memset(&report, 0, sizeof(report));

    if (strcmp(mode, "memory") == 0) {
        memory_stress_worker_loop(worker_id, memory_mb, duration_sec,
                                  PATTERN_SEQUENTIAL, MEM_OP_READ_WRITE,
                                  ALLOC_MALLOC, NULL, &report);
    } else {
        cpu_stress_worker_loop(worker_id, duration_sec, CPU_ALGO_MATRIX, NULL, &report);
    }

    /* Send report back over pipe if open */
    if (pipe_fd >= 0) {
        ipc_pipe_send_report(pipe_fd, &report);
        close(pipe_fd);
    }

    return 0;
}
