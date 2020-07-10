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

#ifndef KIBOSH_TIME_H
#define KIBOSH_TIME_H

#include <stdint.h> // for uint32_t
#include <time.h>

struct timespec;

/**
 * Sleep for a fixed number of milliseconds.
 *
 * @param delay_ms      The number of milliseconds to sleep.
 */
extern void milli_sleep(uint32_t delay_ms);

/**
 * Convert a timespec into a time in milliseconds.
 * This loses precision, because a timespec can represent individual nanoseconds.
 *
 * @param ts            The timespec
 * @return              The time in milliseconds
 */
extern uint64_t timespec_to_ms(const struct timespec *ts);

#endif

// vim: ts=4:sw=4:tw=99:et
