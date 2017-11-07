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

// Get the POSIX versions of strerror_r
#define _POSIX_C_SOURCE 200112L
#undef _GNU_SOURCE

#include "io.h"
#include "log.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_PREFIX_BUFFER_SIZE 64
__thread char log_prefix_buffer[LOG_PREFIX_BUFFER_SIZE];

#define STRERROR_BUFFER_SIZE 128
__thread char strerror_buffer[STRERROR_BUFFER_SIZE];

FILE *global_kibosh_log_file = NULL;

int global_kibosh_log_file_fd = -1;

uint32_t global_kibosh_log_settings = KIBOSH_LOG_ALL_ENABLED;

void kibosh_log_init(FILE *log_file, uint32_t settings)
{
    global_kibosh_log_file = log_file;
    global_kibosh_log_file_fd = fileno(log_file);
    global_kibosh_log_settings = settings;
}

const char* log_prefix(void)
{
    int ret;
    struct timespec ts;
    time_t time;
    struct tm tm;

    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        snprintf(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "(clock_gettime failed) ");
        return log_prefix_buffer;
    }
    time = ts.tv_sec;
    if (!localtime_r(&time, &tm)) {
        snprintf(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "(localtime_r failed) ");
        return log_prefix_buffer;
    }
    ret = strftime(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "[%Y-%m-%d %H:%M:%S,", &tm);
    if (ret <= 0) {
        snprintf(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "(strftime failed) ");
        return log_prefix_buffer;
    }
    snprintf(log_prefix_buffer + ret, LOG_PREFIX_BUFFER_SIZE - ret, "%09ld] ", ts.tv_nsec);
    return log_prefix_buffer;
}

const char* safe_strerror(int errnum)
{
    if (errnum < 0) {
        errnum = -errnum;
    }
    if (strerror_r(errnum, strerror_buffer, STRERROR_BUFFER_SIZE)) {
        snprintf(strerror_buffer, STRERROR_BUFFER_SIZE, "Unknown error %d", errnum);
    }
    return strerror_buffer;
}

int signal_safe_uint32_to_string(uint32_t val, char *buf, size_t buf_len)
{
    size_t i, j;
    int temp;

    for (i = 0; i < buf_len - 1; i++) {
        buf[i] = '0' + (val % 10);
        val = val / 10;
        if (val == 0) {
            break;
        }
    }
    if (i == buf_len - 1)
        return -ENAMETOOLONG;
    i++;
    for (j = 0; j < i / 2; j++) {
        temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }
    buf[i] = '\0';
    return i;
}

#define SHUTDOWN_MSG "kibosh was terminated by signal "
#define SHUTDOWN_MSG_LEN (sizeof(SHUTDOWN_MSG) - 1)

int emit_shutdown_message(int signal)
{
    // Because this need to be callable from a signal handler, we can't use most of the nice
    // functions we would like to use, like malloc, printf or fwrite.  They are not signal-safe.
    // Signal handlers may be invoked while the malloc mutex or other locks are held.
    int ret;
    char buf[1024];
    memcpy(buf, SHUTDOWN_MSG, SHUTDOWN_MSG_LEN);
    ret = signal_safe_uint32_to_string(signal, buf + SHUTDOWN_MSG_LEN,
                                       sizeof(buf) - SHUTDOWN_MSG_LEN);
    if (ret < 0)
        return ret;
    buf[SHUTDOWN_MSG_LEN + ret] = '\n';
    // Here, we write directly to the underlying file descriptor of the log file.
    // This may result in our message cutting into a buffered message which is currently getting
    // written out.  But the process is dying anyway-- hopefully seeing this message will help
    // someone debug what happened.
    return safe_write(global_kibosh_log_file_fd, buf, SHUTDOWN_MSG_LEN + ret + 1);
}

// vim: ts=4:sw=4:tw=99:et
