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

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int write_pidfile(const char *path)
{
    char str[128];
    int ret;

    pid_t pid = getpid();
    snprintf(str, sizeof(str), "%lld\n", (long long) pid);
    ret = write_string_to_file(path, str);
    if (ret < 0) {
        INFO("write_pidfile(%s): failed to write pidfile: %s (%d)\n",
             path, safe_strerror(-ret), -ret);
        return ret;
    }
    return 0;
}

int remove_pidfile(const char *path)
{
    if (unlink(path) < 0) {
        int ret = -errno;
        INFO("remove_pidfile(%s): failed to delete pidfile: %s (%d)\n",
             path, safe_strerror(-ret), -ret);
        return ret;
    }
    return 0;
}

// vim: ts=4:sw=4:tw=99:et
