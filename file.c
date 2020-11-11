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

#include "file.h"
#include "fs.h"
#include "log.h"
#include "time.h"
#include "util.h"
#include "fault.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

static struct kibosh_file *kibosh_file_alloc(enum kibosh_file_type type,
                                             const char *path)
{
    struct kibosh_file *file = malloc(sizeof(struct kibosh_file) +
                                    strlen(path) + 1);
    if (!file)
        return NULL;
    file->type = type;
    file->fd = -1;
    strcpy(file->path, path);
    return file;
}

static int kibosh_open_control_file_impl(const char *path, int flags,
        struct fuse_file_info *info)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    struct kibosh_file *file;

    file = kibosh_file_alloc(KIBOSH_FILE_TYPE_CONTROL, path);
    if (!file)
        return -ENOMEM;
    file->fd = kibosh_fs_accessor_fd_alloc(fs, !(flags & O_TRUNC));
    if (file->fd < 0) {
        free(file);
        return file->fd;
    }
    info->fh = (uintptr_t)(void*)file;
    return 0;
}

static int kibosh_open_normal_file_impl(const char *path, int flags,
            mode_t mode, struct fuse_file_info *info)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    int ret = 0;
    char bpath[PATH_MAX] = { 0 };
    struct kibosh_file *file = NULL;

    file = kibosh_file_alloc(KIBOSH_FILE_TYPE_NORMAL, path);
    if (!file) {
        ret = -ENOMEM;
        goto error;
    }

    // Assume that FUSE has already taken care of the umask.
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    file->fd = open(bpath, flags, mode);

    if (file->fd < 0) {
        ret = -errno;
        goto error;
    }

    // If new file is created, change the owner to actual user
    if ((flags & O_CREAT) == O_CREAT)  {
        if (fchown(file->fd, fuse_get_context()->uid, fuse_get_context()->gid) < 0) {
            ret = -errno;
            goto error;
        }
    }

    info->fh = (uintptr_t)(void*)file;
    return 0;

error:
    if (file) {
        if (file->fd >= 0) {
            close(file->fd);
        }
        free(file);
    }
    return ret;
}

static int kibosh_open_impl(const char *path, int addflags,
            mode_t mode, struct fuse_file_info *info)
{
    int ret, flags;
    enum kibosh_file_type type;

    flags = info->flags;
    if ((flags & O_ACCMODE) == 0)  {
        flags |= O_RDONLY;
    }
    if (strcmp(KIBOSH_CONTROL_PATH, path) == 0) {
        type = KIBOSH_FILE_TYPE_CONTROL;
        ret = kibosh_open_control_file_impl(path, flags, info);
    } else {
        type = KIBOSH_FILE_TYPE_NORMAL;
        ret = kibosh_open_normal_file_impl(path, flags, mode, info);
    }
    if (global_kibosh_log_settings & KIBOSH_LOG_DEBUG_ENABLED) {
        char addflags_str[128] = { 0 };
        char flags_str[128] = { 0 };
        open_flags_to_str(addflags, addflags_str, sizeof(addflags_str));
        open_flags_to_str(info->flags, flags_str, sizeof(addflags_str));
        DEBUG("kibosh_open_impl(path=%s, addflags=%s, info->flags=%s, "
              "mode=%04o, type=%s) = %d\n", path, addflags_str, flags_str,
              mode, kibosh_file_type_str(type), ret);
    }
    return AS_FUSE_ERR(ret);
}

int kibosh_create(const char *path, mode_t mode, struct fuse_file_info *info)
{
    DEBUG("kibosh_create(path=%s, mode=%04o)\n", path, mode);
    return kibosh_open_impl(path, O_CREAT, mode, info);
}

int kibosh_fallocate(const char *path UNUSED, int mode, off_t offset,
                     off_t len, struct fuse_file_info *info)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (fallocate(file->fd, mode, offset, len) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_fallocate(file->path=%s, mode=%04o, offset=%"PRId64
          "len=%"PRId64", file->fd=%d) = %d\n", file->path, mode,
          (int64_t)offset, (int64_t)len, file->fd, ret);
    return AS_FUSE_ERR(ret);
}

int kibosh_fgetattr(const char *path UNUSED, struct stat *stat, struct fuse_file_info *info)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (fstat(file->fd, stat) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_fgetattr(file->path=%s, fd=%d) = %d (%s)\n",
          file->path, file->fd, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_flush(const char *path UNUSED, struct fuse_file_info *info UNUSED)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;

    // There is no cache to flush here, so this operation is a no-op.
    DEBUG("kibosh_flush(file->path=%s) = 0\n", file->path);
    return AS_FUSE_ERR(0);
}

int kibosh_fsync(const char *path UNUSED, int datasync, struct fuse_file_info *info)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (datasync) {
        if (fdatasync(file->fd) < 0) {
            ret = -errno;
        }
    } else {
        if (fsync(file->fd) < 0) {
            ret = -errno;
        }
    }
    DEBUG("kibosh_fsync(file->path=%s, file->fd=%d, datasync=%d) = %d (%s)\n",
          file->path, file->fd, datasync, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_ftruncate(const char *path UNUSED, off_t len, struct fuse_file_info *info)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (ftruncate(file->fd, len) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_ftruncate(path=%s, len=%"PRId64", file->fd=%d) = %d (%s)\n",
          path, (int64_t)len, file->fd, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_open(const char *path, struct fuse_file_info *info)
{
    DEBUG("kibosh_open(path=%s): begin...\n", path);
    return kibosh_open_impl(path, 0, 0, info);
}

static char *printf_result_code(char *out, size_t size, int ret)
{
    if (ret >= 0) {
        snprintf(out, size, "%d", ret);
    } else {
        snprintf(out, size, "%d (%s)", ret, safe_strerror(ret));
    }
    return out;
}

int kibosh_read(const char *path UNUSED, char *buf, size_t size, off_t offset,
                struct fuse_file_info *info)
{
    int ret = 0;
    size_t off = 0;
    uint32_t uid, delay_ms = 0;
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    struct kibosh_fault_base *fault;
    const char *fault_name = NULL;
    char scratch[32];

    uid = fuse_get_context()->uid;
    while (off < size) {
        ret = pread(file->fd, buf + off, size - off, offset + off);
        if (ret < 0) {
            ret = -errno;
            break;
        } else if (ret == 0) {
            ret = off;
            break;
        }
        off += ret;
    }
    if (ret <= 0) {
        DEBUG("kibosh_read(file->path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32") "
              "= %s\n", file->path, size, (int64_t)offset, uid,
              printf_result_code(scratch, sizeof(scratch), ret));
        return ret;
    }
    pthread_mutex_lock(&fs->lock);
    fault = find_first_fault(fs->faults, file->path, "read");
    if (fault) {
        fault_name = kibosh_fault_type_name(fault);
        ret = apply_read_fault(fault, buf, ret, &delay_ms);
    }
    pthread_mutex_unlock(&fs->lock);
    if (delay_ms > 0) {
        milli_sleep(delay_ms);
    }
    if (fault_name) {
        INFO("kibosh_read(file->path=%s, size=%zd, offset=%"PRId64", uid=%"PRId32", "
             "fault=%s, delay_ms=%"PRId32 ") = %s\n",
             file->path, size, (int64_t)offset, uid, fault_name, delay_ms,
              printf_result_code(scratch, sizeof(scratch), ret));
    } else {
        DEBUG("kibosh_read(file->path=%s, size=%zd, offset=%"PRId64", uid=%"PRId32") "
              "= %s\n", file->path, size, (int64_t)offset, uid,
              printf_result_code(scratch, sizeof(scratch), ret));
    }
    return ret;
}

int kibosh_release(const char *path UNUSED, struct fuse_file_info *info)
{
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    int ret = 0;

    switch (file->type) {
    case KIBOSH_FILE_TYPE_NORMAL:
        if (close(file->fd) < 0) {
            ret = -errno;
        }
        break;
    case KIBOSH_FILE_TYPE_CONTROL:
        ret = kibosh_fs_accessor_fd_release(fs, file->fd);
        break;
    }
    DEBUG("kibosh_release(file->path=%s, file->fd=%d, type=%s) = %d (%s)\n",
          file->path, file->fd, kibosh_file_type_str(file->type), -ret, safe_strerror(-ret));
    file->fd = -1;
    free(file);
    return AS_FUSE_ERR(ret);
}

int kibosh_write(const char *path UNUSED, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *info)
{
    int ret = 0;
    uint32_t delay_ms = 0, uid = fuse_get_context()->uid;
    struct kibosh_file *file = (struct kibosh_file*)(uintptr_t)info->fh;
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    size_t off = 0;
    char *dynamic_buf = NULL, scratch[32];
    struct kibosh_fault_base *fault;
    const char *fault_name = NULL;

    pthread_mutex_lock(&fs->lock);
    fault = find_first_fault(fs->faults, file->path, "write");
    if (fault) {
        fault_name = kibosh_fault_type_name(fault);
        ret = apply_write_fault(fault, &buf, &dynamic_buf, size, &delay_ms);
    }
    pthread_mutex_unlock(&fs->lock);
    if (ret < 0) {
        goto done;
    }
    if (delay_ms > 0) {
        milli_sleep(delay_ms);
    }
    while (off < size) {
        ret = pwrite(file->fd, buf + off, size - off, offset + off);
        if (ret < 0) {
            ret = -errno;
            break;
        }
        off += ret;
    }
    ret = off;

done:
    free(dynamic_buf);
    if (fault_name) {
        INFO("kibosh_write(file->path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32
              ", fault=%s) = %s\n", file->path, size, (int64_t)offset, uid, fault_name,
              printf_result_code(scratch, sizeof(scratch), ret));
    } else {
        DEBUG("kibosh_read(file->path=%s, size=%zd, offset=%"PRId64", uid=%"PRId32") "
              "= %s\n", file->path, size, (int64_t)offset, uid,
              printf_result_code(scratch, sizeof(scratch), ret));
    }
    return ret;
}

const char *kibosh_file_type_str(enum kibosh_file_type type)
{
    switch (type) {
    case KIBOSH_FILE_TYPE_NORMAL:
        return "normal";
    case KIBOSH_FILE_TYPE_CONTROL:
        return "control";
    default:
        return "unknown";
    }
}

// vim: ts=4:sw=4:tw=99:et
