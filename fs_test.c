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

#include "fs.h"
#include "io.h"
#include "log.h"
#include "test.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void print_usage(void) 
{
        fprintf(stderr, 
"fs_unit: tests filesystem operations in a directory.\n"
"\n"
"Usage:\n"
"fs_unit [test_path]\n");
}

static int test_create_and_remove_subdir(const char *base)
{
    char subdir1[PATH_MAX];

    EXPECT_POSIX_SUCC(access(base, R_OK));
    EXPECT_POSIX_SUCC(access(base, W_OK));
    snprintf(subdir1, sizeof(subdir1), "%s/subdir1", base);
    EXPECT_POSIX_FAIL(access(subdir1, F_OK), ENOENT);
    EXPECT_POSIX_SUCC(mkdir(subdir1, 0777));
    EXPECT_POSIX_FAIL(mkdir(subdir1, 0777), EEXIST);
    EXPECT_POSIX_SUCC(rmdir(subdir1));
    EXPECT_POSIX_FAIL(rmdir(subdir1), ENOENT);

    return 0;
}

static int test_create_and_remove_nested(const char *base)
{
    char nest[PATH_MAX], nest2[PATH_MAX], nest3[PATH_MAX];

    snprintf(nest, sizeof(nest), "%s/nest", base);
    EXPECT_POSIX_FAIL(access(nest, F_OK), ENOENT);
    EXPECT_POSIX_SUCC(mkdir(nest, 0777));
    EXPECT_POSIX_FAIL(mkdir(nest, 0777), EEXIST);
    snprintf(nest2, sizeof(nest2), "%s/nest2", nest);
    snprintf(nest3, sizeof(nest3), "%s/nest3", nest);
    EXPECT_POSIX_SUCC(mkdir(nest2, 0777));
    EXPECT_POSIX_SUCC(mkdir(nest3, 0777));
    EXPECT_INT_ZERO(recursive_unlink(nest));
    EXPECT_POSIX_FAIL(access(nest2, F_OK), ENOENT);
    EXPECT_POSIX_FAIL(access(nest3, F_OK), ENOENT);
    EXPECT_POSIX_FAIL(access(nest, F_OK), ENOENT);

    return 0;
}

static int create_read_fault(const char *base, int code)
{
    char json[256];
    char control_path[PATH_MAX];

    snprintf(control_path, sizeof(control_path), "%s%s", base, KIBOSH_CONTROL_PATH);
    snprintf(json, sizeof(json),
        "{\"faults\":[{\"type\":\"unreadable\", \"code\":%d}]}", code);
    EXPECT_INT_ZERO(write_string_to_file(control_path, json));
    return 0;
}

static int clear_faults(const char *base)
{
    char json[256];
    char control_path[PATH_MAX];

    snprintf(control_path, sizeof(control_path), "%s%s", base, KIBOSH_CONTROL_PATH);
    snprintf(json, sizeof(json), "{\"faults\":[]}");
    EXPECT_INT_ZERO(write_string_to_file(control_path, json));
    return 0;
}

static int test_create_and_read_file(const char *base, int read_fault)
{
    unsigned int i;
    int fd;
    char test_string[16384], test_string2[16384];
    char nest4[PATH_MAX], test_path[PATH_MAX];

    snprintf(nest4, sizeof(nest4), "%s/nest4", base);
    EXPECT_POSIX_FAIL(access(nest4, F_OK), ENOENT);
    EXPECT_POSIX_SUCC(mkdir(nest4, 0777));
    EXPECT_POSIX_FAIL(mkdir(nest4, 0777), EEXIST);
    snprintf(test_path, sizeof(test_path), "%s/test_file", nest4);
    for (i = 0; i < sizeof(test_string) / sizeof(test_string[0]); i++) {
        test_string[i] = ('0' + (i % 9));
    }
    test_string[(sizeof(test_string) / sizeof(test_string[0])) - 1] = '\0';
    EXPECT_INT_ZERO(write_string_to_file(test_path, test_string));
    if (read_fault) {
        EXPECT_INT_ZERO(create_read_fault(base, read_fault));
        fd = open(test_path, O_RDONLY, 0666);
        if (fd < 0) {
            EXPECT_POSIX_SUCC(fd);
        }
        EXPECT_INT_EQ(-read_fault, read_string_from_fd(fd, test_string2, sizeof(test_string2)));
        EXPECT_POSIX_SUCC(close(fd));
        EXPECT_INT_ZERO(clear_faults(base));
    } else {
        EXPECT_INT_ZERO(read_string_from_file(test_path, test_string2, sizeof(test_string2)));
        EXPECT_STR_EQ(test_string, test_string2);
    }
    EXPECT_INT_ZERO(recursive_unlink(nest4));
    EXPECT_POSIX_FAIL(access(nest4, F_OK), ENOENT);
    return 0;
}

int main(int argc, char **argv)
{
    char *base;

    if (argc < 2) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    base = argv[1];

    EXPECT_INT_ZERO(test_create_and_remove_subdir(base));

    EXPECT_INT_ZERO(test_create_and_remove_nested(base));

    EXPECT_INT_ZERO(test_create_and_read_file(base, 0));

    EXPECT_INT_ZERO(test_create_and_read_file(base, EIO));

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
