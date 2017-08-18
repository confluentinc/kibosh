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
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t safe_write(int fd, const void *b, size_t c)
{
    int res;

    while (c > 0) {
        res = write(fd, b, c);
        if (res < 0) {
            if (errno != EINTR)
                return -errno;
        } else {
            c -= res;
            b = (char *)b + res;
        }
    }
    return 0;
}

int write_string_to_file(const char *path, const char *str)
{
    int fd, ret;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return -errno;
    }
    ret = safe_write(fd, str, strlen(str));
    if (ret < 0) {
        close(fd);
        return ret;
    }
    if (close(fd) < 0) {
        return -errno;
    }
    return 0;
}

ssize_t safe_read(int fd, void *b, size_t c)
{
    int res;
    size_t cnt = 0;

    while (cnt < c) {
        res = read(fd, b, c - cnt);
        if (res <= 0) {
            if (res == 0)
                return cnt;
            if (errno != EINTR)
                return -errno;
        } else {
            cnt += res;
            b = (char *)b + res;
        }
    }
    return cnt;
}

int read_string_from_fd(int fd, char *buf, size_t buf_len)
{
    int ret = safe_read(fd, buf, buf_len - 1);
    if (ret < 0) {
        return ret;
    }
    buf[ret] = '\0';
    return 0;
}

int read_string_from_file(const char *path, char *buf, size_t buf_len)
{
    int fd, ret;

    fd = open(path, O_RDONLY, 0666);
    if (fd < 0) {
        return -errno;
    }
    ret = read_string_from_fd(fd, buf, buf_len);
    if (ret < 0) {
        close(fd);
        return ret;
    }
    if (close(fd) < 0) {
        return -errno;
    }
    return 0;
}

int duplicate_fd(int dest_fd, int src_fd)
{
    char buf[128];
    int ret;

    while (1) {
        int nread = safe_read(src_fd, buf, sizeof(buf));
        if (nread <= 0)
            return nread;
        ret = safe_write(dest_fd, buf, nread);
        if (ret < 0)
            return ret;
    }
}

// vim: ts=4:sw=4:tw=99:et
