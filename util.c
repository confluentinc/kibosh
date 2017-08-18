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

#include "json.h"
#include "log.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

int snappend(char *str, size_t str_len, const char *fmt, ...)
{
    va_list ap;
    size_t slen = strlen(str);
    if (slen >= str_len + 1) {
        return -ENAMETOOLONG;
    }
    va_start(ap, fmt);
    vsnprintf(str + slen, str_len - slen, fmt, ap);
    va_end(ap);
    return 0;
}

char *dynprintf(const char *fmt, ...)
{
    va_list ap;
    char *out = NULL;
    va_start(ap, fmt);
    if (vasprintf(&out, fmt, ap) < 0) {
        out = NULL;
    }
    va_end(ap);
    return out;
}

char *join_strs(char **strs)
{
    char **iter, *s, *ret;
    int len = 1;

    for (iter = strs; *iter; iter++) {
        len += strlen(*iter);
    }
    ret = malloc(len);
    if (!ret)
        return NULL;
    s = ret;
    for (iter = strs; *iter; iter++) {
        strcpy(s, *iter);
        s += strlen(*iter);
    }
    return ret;
}

json_value *get_child(json_value *obj, const char *name)
{
    unsigned int i;

    if (obj->type != json_object) {
        return NULL;
    }
    for (i = 0; i < obj->u.object.length; i++) {
        if (strcmp(obj->u.object.values[i].name, name) == 0) {
            return obj->u.object.values[i].value;
        }
    }
    return NULL;
}

int open_flags_to_str(int flags, char *str, size_t max_len)
{
    const char *prefix = "";

    // TODO: optimize this a bit
    if (flags & O_RDONLY) {
        if (snappend(str, max_len, "%sO_RDONLY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_WRONLY) {
        if (snappend(str, max_len, "%sO_WRONLY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_RDWR) {
        if (snappend(str, max_len, "%sO_RDWR", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_CREAT) {
        if (snappend(str, max_len, "%sO_CREAT", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_EXCL) {
        if (snappend(str, max_len, "%sO_EXCL", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOCTTY) {
        if (snappend(str, max_len, "%sO_NOCTTY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_TRUNC) {
        if (snappend(str, max_len, "%sO_TRUNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_APPEND) {
        if (snappend(str, max_len, "%sO_APPEND", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NONBLOCK) {
        if (snappend(str, max_len, "%sO_NONBLOCK", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DSYNC) {
        if (snappend(str, max_len, "%sO_DSYNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & FASYNC) {
        if (snappend(str, max_len, "%sO_FASYNC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DIRECT) {
        if (snappend(str, max_len, "%sO_DIRECT", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_LARGEFILE) {
        if (snappend(str, max_len, "%sO_LARGEFILE", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_DIRECTORY) {
        if (snappend(str, max_len, "%sO_DIRECTORY", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOFOLLOW) {
        if (snappend(str, max_len, "%sO_NOFOLLOW", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_NOATIME) {
        if (snappend(str, max_len, "%sO_NOATIME", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    if (flags & O_CLOEXEC) {
        if (snappend(str, max_len, "%sO_CLOEXEC", prefix)) {
            return -ENAMETOOLONG;
        }
        prefix = "|";
    }
    return 0;
}

static int recursive_unlink_helper(int dirfd, const char *name)
{
    int fd = -1, ret = 0;
    DIR *dfd = NULL;
    struct stat stat;
    struct dirent *de;

    fd = openat(dirfd, name, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        ret = errno;
        fprintf(stderr, "error opening %s: %s\n", name, safe_strerror(ret));
        goto done;
    }
    if (fstat(fd, &stat) < 0) {
        ret = errno;
        fprintf(stderr, "failed to stat %s: %s\n", name, safe_strerror(ret));
        goto done;
    }
    if (!(S_ISDIR(stat.st_mode))) {
        if (unlinkat(dirfd, name, 0)) {
            ret = errno;
            fprintf(stderr, "failed to unlink %s: %s\n", name, safe_strerror(ret));
            goto done;
        }
    } else {
        dfd = fdopendir(fd);
        if (!dfd) {
            ret = errno;
            fprintf(stderr, "fopendir(%s) failed: %s\n", name, safe_strerror(ret));
            goto done;
        }
        while ((de = readdir(dfd))) {
            if ((de->d_name[0] ==  '.') && (de->d_name[1] == '\0')) {
                continue;
            }
            if ((de->d_name[0] ==  '.') && (de->d_name[1] == '.') &&
                    (de->d_name[2] == '\0')) {
                continue;
            }
            ret = recursive_unlink_helper(fd, de->d_name);
            if (ret) {
                goto done;
            }
        }
        if (closedir(dfd) < 0) {
            ret = errno;
            fprintf(stderr, "closedir(%s) failed: %s\n", name, safe_strerror(ret));
            goto done;
        }
        dfd = NULL;
        if (unlinkat(dirfd, name, AT_REMOVEDIR)) {
            ret = errno;
            fprintf(stderr, "failed to unlink directory %s: %s\n",
                    name, safe_strerror(ret));
            goto done;
        }
    }
done:
    if (fd >= 0) {
        close(fd);
    }
    if (dfd) {
        closedir(dfd);
    }
    return -ret;
}

int recursive_unlink(const char *name)
{
    return recursive_unlink_helper(AT_FDCWD, name);
}

int memfd_create(const char *name)
{
    int ret = syscall(SYS_memfd_create, name, 0);
    if (ret < 0) {
        return -errno;
    }
    return ret;
}

// vim: ts=4:sw=4:tw=99:et
