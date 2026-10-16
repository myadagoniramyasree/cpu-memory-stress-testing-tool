/**
 * @file logger.h
 * @brief Low-level file system logging using open(), write(), lseek(), and close().
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO5: File-System Operations and Performance Reports.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

/* Initialize low-level logger with destination path */
int logger_init(const char *filepath, log_level_t min_level);

/* Close logger file descriptor */
void logger_close(void);

/* Low-level write logging functions */
void log_message(log_level_t level, const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

/* Ensure directory exists using mkdir() */
int logger_ensure_dir(const char *dirpath);

#endif /* LOGGER_H */
