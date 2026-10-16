/**
 * @file main.c
 * @brief Master entry point and controller for CPU and Memory Stress Testing Tool.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Covers all six Course Outcomes: CO1 to CO6.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "config.h"
#include "cli.h"
#include "logger.h"
#include "ipc.h"
#include "process_manager.h"
#include "thread_manager.h"
#include "memory_stress.h"
#include "monitor.h"
#include "report.h"

int main(int argc, char *argv[]) {
    stress_config_t cfg;
    config_init_defaults(&cfg);

    /* 1. Parse Command-Line Arguments (CO1) */
    if (cli_parse(argc, argv, &cfg) != 0) {
        return EXIT_FAILURE;
    }

    /* 2. Validate Configuration against Safety Constraints */
    char err_buf[256];
    if (config_validate(&cfg, err_buf, sizeof(err_buf)) != 0) {
        fprintf(stderr, "\n[CONFIGURATION ERROR]: %s\n\n", err_buf);
        return EXIT_FAILURE;
    }

    /* 3. Dry-Run Mode */
    if (cfg.dry_run) {
        cli_print_dry_run(&cfg);
        return EXIT_SUCCESS;
    }

    /* 4. Dedicated Educational Demos */
    if (cfg.mode == MODE_COW_DEMO) {
        /* CO4 Demand Paging & Copy-On-Write Experiment */
        return (memory_run_cow_experiment(cfg.memory_mb) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (cfg.mode == MODE_RACE_DEMO) {
        /* CO6 Race Condition vs Mutex Synchronization Demo */
        return (thread_manager_run_race_demo(cfg.num_workers, 500000, cfg.sync_threads) == 0)
               ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* 5. Initialize Logger and Output Directory (CO5) */
    if (logger_init(cfg.log_file, cfg.verbose ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO) != 0) {
        fprintf(stderr, "Failed to initialize logger file '%s'\n", cfg.log_file);
        return EXIT_FAILURE;
    }

    if (report_csv_init(cfg.csv_file) != 0) {
        fprintf(stderr, "Failed to initialize CSV dataset '%s'\n", cfg.csv_file);
        logger_close();
        return EXIT_FAILURE;
    }

    config_print(&cfg);
    log_info("Execution starting: Mode=%s, Workers=%d, Duration=%ds",
             mode_to_string(cfg.mode), cfg.num_workers, cfg.duration_sec);

    /* 6. Register Signal Handlers (CO3) */
    ipc_setup_signal_handlers();

    struct timespec ts_total_start, ts_total_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_total_start);

    process_group_t pg;
    memset(&pg, 0, sizeof(pg));

    thread_group_t tg;
    memset(&tg, 0, sizeof(tg));

    ipc_context_t ipc;
    memset(&ipc, 0, sizeof(ipc));

    uint64_t total_iterations = 0;

    /* 7. Dispatch Workload Engine */
    if (cfg.mode == MODE_THREADS) {
        /* CO6: Thread-based workload */
        if (thread_manager_run(&cfg, &tg) != 0) {
            log_error("Thread manager execution encountered errors");
        }
        for (int i = 0; i < tg.count; i++) {
            total_iterations += tg.workers[i].iterations;
        }
    } else {
        /* CO2 & CO3: Process-based workload */
        if (ipc_init(&ipc, cfg.ipc_type, cfg.num_workers) != 0) {
            log_error("IPC initialization failed");
            logger_close();
            return EXIT_FAILURE;
        }

        if (process_manager_run(&cfg, &pg, &ipc) != 0) {
            log_error("Process manager execution encountered errors");
        }

        for (int i = 0; i < pg.count; i++) {
            total_iterations += pg.reports[i].iterations;
        }

        ipc_cleanup(&ipc);
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_total_end);
    double total_elapsed = (double)(ts_total_end.tv_sec - ts_total_start.tv_sec) +
                           (double)(ts_total_end.tv_nsec - ts_total_start.tv_nsec) * 1e-9;

    /* 8. Collect Final System Metrics */
    monitor_context_t final_mon;
    monitor_init(&final_mon, 100);
    system_metrics_t final_metrics;
    monitor_sample(&final_mon, &final_metrics, total_iterations);
    final_metrics.throughput_iter_sec = (total_elapsed > 0.0) ? (double)total_iterations / total_elapsed : 0.0;

    /* 9. Generate Summary Report and Update History Ledger (CO5) */
    char history_path[MAX_PATH_LEN];
    snprintf(history_path, sizeof(history_path), "%s/test_history.csv", cfg.output_dir);

    report_generate_summary(&cfg, (cfg.mode == MODE_THREADS) ? NULL : &pg,
                            (cfg.mode == MODE_THREADS) ? &tg : NULL,
                            &final_metrics, total_elapsed);

    report_update_history_ledger(history_path, &cfg, total_iterations,
                                final_metrics.total_cpu_pct,
                                final_metrics.app_vmrss_mb,
                                total_elapsed, "COMPLETED");

    log_info("Stress test completed successfully in %.2f seconds. Reports generated.", total_elapsed);
    logger_close();

    /* 10. Terminal Results Confirmation */
    printf("\n=======================================================\n");
    printf("              TEST EXECUTION COMPLETED                 \n");
    printf("=======================================================\n");
    printf("  Duration Elapsed : %.2f seconds\n", total_elapsed);
    printf("  Total Iterations : %lu\n", (unsigned long)total_iterations);
    printf("  Throughput       : %.2f iterations/sec\n",
           total_elapsed > 0 ? (double)total_iterations / total_elapsed : 0.0);
    printf("  Artifacts Saved:\n");
    printf("    -> Text Summary : %s\n", cfg.summary_file);
    printf("    -> CSV Telemetry: %s\n", cfg.csv_file);
    printf("    -> Raw Log File : %s\n", cfg.log_file);
    printf("    -> History File : %s\n", history_path);
    printf("=======================================================\n\n");

    return EXIT_SUCCESS;
}
