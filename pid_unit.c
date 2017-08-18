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

#include "io.h"
#include "log.h"
#include "pid.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_write_pidfile(void)
{
    char path[4096], buf[4096], expected[4096];
    char const *tmp = getenv("TMPDIR");
    if (!tmp)
        tmp = "/dev/shm";
    snprintf(path, sizeof(path), "%s/pid_file.%lld.%d", tmp, (long long)getpid(), rand());
    EXPECT_INT_ZERO(write_pidfile(path));
    memset(buf, sizeof(buf), 0);
    EXPECT_INT_ZERO(read_string_from_file(path, buf, sizeof(buf)));
    snprintf(expected, sizeof(expected), "%lld\n", (long long)getpid());
    EXPECT_STR_EQ(expected, buf);
    EXPECT_INT_ZERO(remove_pidfile(path));
    return 0;
}

int main(void)
{
    EXPECT_INT_ZERO(test_write_pidfile());

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
