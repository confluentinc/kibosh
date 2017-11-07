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

#include "test.h"
#include "util.h"

#include <errno.h>
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

int main(void)
{
    EXPECT_INT_ZERO(test_log_prefix_lexicographically_increases());

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
