/**
 * Copyright 2019 Confluent Inc.
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

#include "time.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

void milli_sleep(uint32_t delay_ms)
{
    struct timespec deadline;
    uint32_t delay_seconds;
    // This should never fail... on Linux, at least.
    if (clock_gettime(CLOCK_MONOTONIC, &deadline)) {
        abort();
    }
    delay_seconds = delay_ms;
    delay_seconds /= 1000;
    deadline.tv_sec += delay_seconds;
    deadline.tv_nsec += (delay_ms - (delay_seconds * 1000)) * 1000000;
    // We use clock_nanosleep here in an attempt to avoid issues that might occur if the wall-clock
    // time is changed.  The function may return early if it is interrupted by a signal.
    while (1) {
        int rval = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
        if (rval == 0) {
            break;
        } else if (rval != EINTR) {
            // Error codes other than EINTR are bad news.
            abort();
        }
    }
}

uint64_t timespec_to_ms(const struct timespec *ts)
{
    uint64_t rval, seconds = ts->tv_sec, nanoseconds = ts->tv_nsec;
    rval = seconds * 1000;
    rval += nanoseconds / 1000000;
    return rval;
}

// vim: ts=4:sw=4:tw=99:et
