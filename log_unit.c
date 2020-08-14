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
#include "test.h"
#include "util.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int test_log_prefix_lexicographically_increases(void)
{
    int ret;
    char *prefix1 = NULL;
    const char *cur_prefix = NULL;

    prefix1 = strdup(log_prefix());
    EXPECT_NONNULL(prefix1);
    do {
        cur_prefix = log_prefix();
        EXPECT_NONNULL(cur_prefix);
        ret = strcmp(cur_prefix, prefix1);
        if (ret <= 0) {
            fprintf(stderr, "test_log_prefix_lexicographically_increases: log prefix "
                "'%s' came before initial log prefix of '%s'\n", cur_prefix, prefix1);
        }
    } while (ret <= 0);

    free(prefix1);
    return 0;
}

static int test_signal_safe_uint32_to_string(void)
{
    char buf[128];

    memset(buf, 0, sizeof(buf));
    EXPECT_INT_EQ(3, signal_safe_uint32_to_string(123, buf, sizeof(buf)));
    EXPECT_STR_EQ("123", buf);
    EXPECT_INT_EQ(1, signal_safe_uint32_to_string(0, buf, sizeof(buf)));
    EXPECT_STR_EQ("0", buf);
    EXPECT_INT_EQ(-ENAMETOOLONG, signal_safe_uint32_to_string(14, buf, 2));
    EXPECT_INT_EQ(1, signal_safe_uint32_to_string(3, buf, 2));
    EXPECT_STR_EQ("3", buf);

    return 0;
}

static int test_emit_shutdown_message(void)
{
    FILE *fp;
    char path[PATH_MAX], buf[4096];
    char const *tmp = getenv("TMPDIR");

    if (!tmp)
        tmp = "/dev/shm";
    snprintf(path, sizeof(path), "%s/log_path.%lld.%ld", tmp, (long long)getpid(), lrand48());
    fp = fopen(path, "w");
    EXPECT_NONNULL(fp);
    kibosh_log_init(fp, 0);
    emit_shutdown_message(11);
    memset(buf, 0, sizeof(buf));
    EXPECT_INT_EQ(0, read_string_from_file(path, buf, sizeof(buf)));
    EXPECT_STR_EQ("kibosh was terminated by signal 11\n", buf);
    fclose(fp);
    return 0;
}

int main(void)
{
    EXPECT_INT_ZERO(test_log_prefix_lexicographically_increases());
    EXPECT_INT_ZERO(test_signal_safe_uint32_to_string());
    EXPECT_INT_ZERO(test_emit_shutdown_message());

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
