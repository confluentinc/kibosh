/**
 * Copyright 2017 Confluent Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#ifndef KIBOSH_LOG_H
#define KIBOSH_LOG_H

#include <stdint.h> // for uint32_t
#include <stdio.h> // for printf and FILE*

/*
 * Logging functions for Kibosh.
 *
 * Logging is fairly simple.  It all goes to a FILE* somewhere.  By default
 * the file is stdout, but it can be set to somewhere in the filesystem as
 * well.  There is an option to enable or disable debug logging for each FUSE
 * operation.  Info logging is always enabled.
 */

// The global log file.
extern FILE *global_kibosh_log_file;
#define GET_KIBOSH_LOG_FILE \
    ((global_kibosh_log_file != NULL) ? global_kibosh_log_file : stdout)

// The global log settings.
#define KIBOSH_LOG_DEBUG_ENABLED 0x1
#define KIBOSH_LOG_INFO_ENABLED 0x2
#define KIBOSH_LOG_ALL_ENABLED \
    (KIBOSH_LOG_DEBUG_ENABLED | \
     KIBOSH_LOG_INFO_ENABLED)
extern uint32_t global_kibosh_log_settings;

/**
 * Print a debug message.
 */
#define DEBUG_HELPER(fmt, ...) \
    if (global_kibosh_log_settings & KIBOSH_LOG_DEBUG_ENABLED) { \
        fputs(log_prefix(), GET_KIBOSH_LOG_FILE); \
        fprintf(GET_KIBOSH_LOG_FILE, "DEBUG " fmt "%s", __VA_ARGS__); \
    }
#define DEBUG(...) DEBUG_HELPER(__VA_ARGS__, "")

/**
 * Print an info message.
 */
#define INFO_HELPER(fmt, ...) \
    if (global_kibosh_log_settings & KIBOSH_LOG_INFO_ENABLED) { \
        fputs(log_prefix(), GET_KIBOSH_LOG_FILE); \
        fprintf(GET_KIBOSH_LOG_FILE, "INFO " fmt "%s", __VA_ARGS__); \
    }
#define INFO(...) INFO_HELPER(__VA_ARGS__, "")

/**
 * Initialize the kibosh log file.
 *
 * This function must be called before any threads are started.
 */
void kibosh_log_init(FILE *log_file, uint32_t settings);

/**
 * Return a prefix that should be used for log lines.
 *
 * @return              The log line prefix.  This will be stored in a thread-local variable.
 */
const char* log_prefix(void);

/**
 * A thread-safe strerror alternative.
 *
 * @param errnum        The POSIX error number.  Negative numbers are treated as their positive 
 *                      equivalents.
 *
 * @return              A thread-local error string.  It will remain valid
 *                      until the next call to safe_strerror.
 */
const char *safe_strerror(int err);

/**
 * Converts a uint32_t to a string.  This function is safe to be called from a
 * signal handler.
 *
 * @param val           The integer.
 * @param buf           (out param) Where to put the string.  The string will be
 *                          null-terminated.
 * @param buf_len       The length of the string.
 *
 * @return              -ENAMETOOLONG if the buf_len was too short.
 *                      Otherwise, the length of the string that was written, not
 *                      including the terminating null.
 */
int signal_safe_uint32_to_string(uint32_t val, char *buf, size_t buf_len);

/**
 * Emit a shutdown message.  This function is safe to be called from a signal
 * handler.
 *
 * @param signal        The signal number we are shutting down with.
 *
 * @return              0 on success; a negative error code otherwise.
 */
int emit_shutdown_message(int signal);

#endif

// vim: ts=4:sw=4:et
