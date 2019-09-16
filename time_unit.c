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

#include "test.h"
#include "time.h"

#include <stdlib.h>
#include <time.h>

static int test_sleep_0_ms(void)
{
    milli_sleep(0);
    return 0;
}

static int test_sleep_1_ms(void)
{
    struct timespec now;
    uint64_t old_ms, new_ms;

    EXPECT_INT_ZERO(clock_gettime(CLOCK_MONOTONIC, &now));
    old_ms = timespec_to_ms(&now);
    milli_sleep(1);
    EXPECT_INT_ZERO(clock_gettime(CLOCK_MONOTONIC, &now));
    new_ms = timespec_to_ms(&now);
    EXPECT_INT_LT(old_ms, new_ms);
    return 0;
}

int main(void)
{
    EXPECT_INT_ZERO(test_sleep_0_ms());
    EXPECT_INT_ZERO(test_sleep_1_ms());
    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
