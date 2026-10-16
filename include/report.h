/**
 * @file report.h
 * @brief Performance report generation (CSV, text summary, historical ledger, markdown).
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO5: File-System Operations and Performance Reports.
 */

#ifndef REPORT_H
#define REPORT_H

#include "config.h"
#include "monitor.h"
#include "process_manager.h"
#include "thread_manager.h"

/* Initialize CSV file and write header using low-level write() */
int report_csv_init(const char *csv_path);

/* Append a sample record to the CSV dataset using low-level write() */
int report_csv_append(const char *csv_path, const system_metrics_t *metrics,
                      const stress_config_t *cfg, double elapsed_sec);

/* Generate human-readable text summary file */
int report_generate_summary(const stress_config_t *cfg,
                            const process_group_t *pg,
                            const thread_group_t *tg,
                            const system_metrics_t *final_metrics,
                            double total_time_sec);

/* Update historical test ledger (CSV) */
int report_update_history_ledger(const char *history_csv_path,
                                const stress_config_t *cfg,
                                uint64_t total_iterations,
                                double avg_cpu_pct,
                                size_t peak_rss_mb,
                                double duration_sec,
                                const char *status);

#endif /* REPORT_H */
