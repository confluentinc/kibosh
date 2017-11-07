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

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_PREFIX_BUFFER_SIZE 64
__thread char log_prefix_buffer[LOG_PREFIX_BUFFER_SIZE];

#define STRERROR_BUFFER_SIZE 128
__thread char strerror_buffer[STRERROR_BUFFER_SIZE];

FILE *global_kibosh_log_file = NULL;

uint32_t global_kibosh_log_settings = KIBOSH_LOG_ALL_ENABLED;

void kibosh_log_init(FILE *log_file, uint32_t settings)
{
    global_kibosh_log_file = log_file;
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
    ret = strftime(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "%Y-%m-%d %H:%M:%S,", &tm);
    if (ret <= 0) {
        snprintf(log_prefix_buffer, LOG_PREFIX_BUFFER_SIZE, "(strftime failed) ");
        return log_prefix_buffer;
    }
    snprintf(log_prefix_buffer + ret, LOG_PREFIX_BUFFER_SIZE - ret, "%09ld ", ts.tv_nsec);
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

// vim: ts=4:sw=4:tw=99:et
