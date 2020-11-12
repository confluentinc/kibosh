/**
 * Copyright 2020 Confluent Inc.
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

#include "drop_cache.h"
#include "io.h"
#include "test.h"
#include "time.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_drop_cache(const char *path)
{
    char buf[16];
    EXPECT_INT_ZERO(drop_cache(path));
    memset(&buf, 0, sizeof(buf));
    EXPECT_INT_ZERO(read_string_from_file(path, buf, sizeof(buf)));
    EXPECT_INT_EQ('1', buf[0]);
    unlink(path);
    return 0;
}

static int test_create_thread_and_destroy(const char *path)
{
    struct drop_cache_thread *thread;

    thread = drop_cache_thread_start(path, 100000);
    EXPECT_NONNULL(thread);
    drop_cache_thread_join(thread);
    unlink(path);
    return 0;
}

static int test_create_thread_and_wait_for_file(const char *path)
{
    struct drop_cache_thread *thread;

    thread = drop_cache_thread_start(path, 1);
    EXPECT_NONNULL(thread);
    while (access(path, R_OK)) {
        milli_sleep(1);
    }
    drop_cache_thread_join(thread);
    unlink(path);
    return 0;
}

int main(void)
{
    char path[PATH_MAX];
    char const *tmp = getenv("TMPDIR");

    if (!tmp)
        tmp = "/dev/shm";
    snprintf(path, sizeof(path), "%s/drop_cache_path.%lld.%ld", tmp,
             (long long)getpid(), lrand48());

    EXPECT_INT_ZERO(test_drop_cache(path));
    EXPECT_INT_ZERO(test_create_thread_and_destroy(path));
    EXPECT_INT_ZERO(test_create_thread_and_wait_for_file(path));

    unlink(path);
    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
