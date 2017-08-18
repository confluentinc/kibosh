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

static int test_snappend(void)
{
    char buf[16];
    char canary[16];
    char all_zero[16];

    memset(buf, 0, sizeof(buf));
    memset(canary, 0, sizeof(canary));
    memset(all_zero, 0, sizeof(all_zero));
    snappend(buf, sizeof(buf), "abracadabrafoomanchucalifrag");
    EXPECT_INT_ZERO(strcmp(buf, "abracadabrafoom"));
    EXPECT_INT_ZERO(memcmp(canary, all_zero, sizeof(canary)));
    snappend(buf, sizeof(buf), "other stuff");
    EXPECT_INT_ZERO(strcmp(buf, "abracadabrafoom"));
    EXPECT_INT_ZERO(memcmp(canary, all_zero, sizeof(canary)));
    memset(buf, 0, sizeof(buf));
    snappend(buf, sizeof(buf), "%d", 123);
    EXPECT_INT_ZERO(strcmp(buf, "123"));
    snappend(buf, sizeof(buf), "456");
    EXPECT_INT_ZERO(strcmp(buf, "123456"));
    snappend(buf, sizeof(buf), "789");
    EXPECT_INT_ZERO(strcmp(buf, "123456789"));

    return 0;
}

int main(void)
{
    EXPECT_INT_ZERO(test_snappend());

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
