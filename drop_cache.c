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
#include "log.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct drop_cache_thread {
    pthread_t pthread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char *path;
    int should_run;
    int period;
};

int drop_cache(const char *path)
{
    int ret, fd;
    char buf[] = { '1' };

    fd = open(path, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        return -errno;
    }
    ret = safe_write(fd, buf, sizeof(buf));
    close(fd);
    return ret;
}

static void *drop_cache_thread_run(void *arg)
{
    struct drop_cache_thread *thread = (struct drop_cache_thread *)arg;
    struct timespec deadline;
    int ret;

    INFO("drop_cache_thread: starting with period %d.\n", thread->period);
    while (1) {
        pthread_mutex_lock(&thread->lock);
        if (!thread->should_run) {
            pthread_mutex_unlock(&thread->lock);
            break;
        }
        ret = clock_gettime(CLOCK_MONOTONIC, &deadline);
        if (ret) {
            abort();
        }
        deadline.tv_sec += thread->period;
        ret = pthread_cond_timedwait(&thread->cond, &thread->lock, &deadline);
        INFO("ret = %d\n", ret);
        pthread_mutex_unlock(&thread->lock);
        ret = drop_cache(thread->path);
        if (ret) {
            INFO("drop_cache_thread: failed to drop cache: %s (%d)\n",
                 safe_strerror(ret), ret);
        } else {
            DEBUG("drop_cache_thread: dropped cache.\n");
        }
    }
    INFO("drop_cache_thread: exiting.\n");
    return NULL;
}

struct drop_cache_thread *drop_cache_thread_start(const char *path, int period)
{
    struct drop_cache_thread *thread = NULL;
    pthread_condattr_t attr;
    int ret;

    thread = calloc(sizeof(struct drop_cache_thread), 1);
    if (!thread) {
        INFO("drop_cache_thread_start: OOM\n");
        goto error;
    }
    thread->should_run = 1;
    thread->period = period;
    thread->path = strdup(path);
    if (!thread->path) {
        INFO("drop_cache_thread_start: OOM\n");
        goto error;
    }
    ret = pthread_mutex_init(&thread->lock, NULL);
    if (ret) {
        INFO("drop_cache_thread_start: failed to create lock: %s (%d)\n",
             safe_strerror(ret), ret);
        goto error_free_path;
    }
    ret = pthread_condattr_init(&attr);
    if (ret) {
        INFO("drop_cache_thread_start: failed to create condattr: %s (%d)\n",
             safe_strerror(ret), ret);
        goto error_mutex_destroy;
    }
    ret = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (ret) {
        INFO("drop_cache_thread_start: failed to set cond clock: %s (%d)\n",
             safe_strerror(ret), ret);
        goto error_condattr_destroy;
    }
    ret = pthread_cond_init(&thread->cond, &attr);
    if (ret) {
        INFO("drop_cache_thread_start: failed to create cond: %s (%d)\n",
             safe_strerror(ret), ret);
        goto error_condattr_destroy;
    }
    ret = pthread_create(&thread->pthread, NULL, drop_cache_thread_run, thread);
    if (ret) {
        INFO("drop_cache_thread_start: failed to create thread: %s (%d)\n",
             safe_strerror(ret), ret);
        goto error_cond_destroy;
    }
    pthread_condattr_destroy(&attr);
    return thread;

error_cond_destroy:
    pthread_cond_destroy(&thread->cond);
error_condattr_destroy:
    pthread_condattr_destroy(&attr);
error_mutex_destroy:
    pthread_mutex_destroy(&thread->lock);
error_free_path:
    free(thread->path);
error:
    free(thread);
    return NULL;
}

void drop_cache_thread_join(struct drop_cache_thread *thread)
{
    pthread_mutex_lock(&thread->lock);
    thread->should_run = 0;
    pthread_cond_signal(&thread->cond);
    pthread_mutex_unlock(&thread->lock);
    pthread_join(thread->pthread, NULL);
    pthread_cond_destroy(&thread->cond);
    pthread_mutex_destroy(&thread->lock);
    free(thread->path);
    free(thread);
}

// vim: ts=4:sw=4:tw=99:et
