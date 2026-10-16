/**
 * @file logger.c
 * @brief Low-level file system logging using POSIX open(), write(), lseek(), and close().
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO5: File-System Operations and Performance Reports.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "logger.h"

static int s_log_fd = -1;
static log_level_t s_min_level = LOG_LEVEL_INFO;

int logger_ensure_dir(const char *dirpath) {
    if (!dirpath || strlen(dirpath) == 0) return 0;

    struct stat st;
    if (stat(dirpath, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        fprintf(stderr, "Error: Path '%s' exists but is not a directory\n", dirpath);
        return -1;
    }

    if (mkdir(dirpath, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) return 0;

    if (errno == EACCES) {
        fprintf(stderr, "File System Error: Permission denied creating directory '%s'\n", dirpath);
        return -1;
    } else if (errno == ENOSPC) {
        fprintf(stderr, "File System Error: No space left on device for directory '%s'\n", dirpath);
        return -1;
    } else if (errno == EROFS) {
        fprintf(stderr, "File System Error: Read-only file system for directory '%s'\n", dirpath);
        return -1;
    }

    fprintf(stderr, "File System Error: Failed creating directory '%s': %s\n", dirpath, strerror(errno));
    return -1;
}

int logger_init(const char *filepath, log_level_t min_level) {
    s_min_level = min_level;

    /* Extract directory component and ensure it exists */
    char dir_copy[512];
    strncpy(dir_copy, filepath, sizeof(dir_copy) - 1);
    char *slash = strrchr(dir_copy, '/');
    if (slash) {
        *slash = '\0';
        if (logger_ensure_dir(dir_copy) != 0) {
            return -1;
        }
    }

    /* Open log file using low-level syscall open() with append and creation flags */
    s_log_fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (s_log_fd < 0) {
        fprintf(stderr, "File System Error: open() failed on '%s': %s\n", filepath, strerror(errno));
        return -1;
    }

    /* Write log initialization header using write() syscall */
    char header[256];
    int len = snprintf(header, sizeof(header),
                      "\n--- STRESS TOOL LOG INITIALIZED [FD=%d, PID=%d] ---\n",
                      s_log_fd, getpid());
    if (len > 0) {
        ssize_t written = write(s_log_fd, header, (size_t)len);
        (void)written;
    }

    return 0;
}

void logger_close(void) {
    if (s_log_fd >= 0) {
        char footer[128];
        int len = snprintf(footer, sizeof(footer),
                          "--- STRESS TOOL LOG CLOSED [FD=%d] ---\n", s_log_fd);
        if (len > 0) {
            ssize_t written = write(s_log_fd, footer, (size_t)len);
            (void)written;
        }
        close(s_log_fd);
        s_log_fd = -1;
    }
}

static const char *level_to_str(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "LOG  ";
    }
}

void log_message(log_level_t level, const char *fmt, ...) {
    if (level < s_min_level) return;

    char timestamp[32];
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_buf);

    char msg_body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_body, sizeof(msg_body), fmt, args);
    va_end(args);

    char log_line[1200];
    int len = snprintf(log_line, sizeof(log_line), "[%s] [%s] [PID:%d] %s\n",
                       timestamp, level_to_str(level), getpid(), msg_body);

    if (len <= 0) return;

    /* Write directly to file descriptor using POSIX write() */
    if (s_log_fd >= 0) {
        ssize_t written = write(s_log_fd, log_line, (size_t)len);
        (void)written;
    }

    /* Also mirror warnings and errors to console */
    if (level >= LOG_LEVEL_WARN) {
        fprintf(stderr, "%s", log_line);
    }
}

void log_debug(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_message(LOG_LEVEL_DEBUG, "%s", buf);
}

void log_info(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_message(LOG_LEVEL_INFO, "%s", buf);
}

void log_warn(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_message(LOG_LEVEL_WARN, "%s", buf);
}

void log_error(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_message(LOG_LEVEL_ERROR, "%s", buf);
}
