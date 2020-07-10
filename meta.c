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
#include "meta.h"
#include "util.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <fuse.h>
#include <limits.h> // TODO: remove PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h> 
#include <sys/xattr.h>
#include <unistd.h>

struct kibosh_dir {
    /**
     * The backing directory pointer.
     */
    DIR *dp;

    /**
     * The path of this directory when it was opened, as a NULL-terminated string.
     * Note that if the inode is renamed, or a parent directory is renamed, this
     * will not be updated.  This path is used to decide when to inject faults.
     */
    char path[0];
};

struct kibosh_dir *kibosh_dir_alloc(const char *path, DIR *dp)
{
    struct kibosh_dir *dir = malloc(sizeof(struct kibosh_dir) + strlen(path) + 1);
    if (!dir)
        return NULL;
    dir->dp = dp;
    strcpy(dir->path, path);
    return dir;
}

static char *alloc_zterm_xattr(const char *str, size_t len)
{
    char *nstr;

    if (!str) {
        return strdup("(NULL)");
    } else if (len <= 0) {
        return strdup("(empty)");
    }
    nstr = malloc(len + 1);
    if (!nstr) {
        return NULL;
    }
    memcpy(nstr, str, len);
    nstr[len] = '\0';
    return nstr;
}

int kibosh_chmod(const char *path, mode_t mode) 
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chmod(bpath, mode) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_chmod(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_chown(const char *path, uid_t uid, gid_t gid)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chown(bpath, uid, gid) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_chown(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_fsyncdir(const char *path UNUSED, int datasync, struct fuse_file_info *info)
{
    int ret = 0;
    struct kibosh_dir *dir = (struct kibosh_dir *)(uintptr_t)info->fh;

    if (datasync) {
        if (fdatasync(dirfd(dir->dp) < 0)) {
            ret = -errno;
        }
    } else {
        if (fsync(dirfd(dir->dp)) < 0) {
            ret = -errno;
        }
    }
    DEBUG("kibosh_fsyncdir(dir->path=%s, datasync=%d) = %d (%s)\n",
          dir->path, datasync, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_getattr(const char *path, struct stat *stbuf)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    enum kibosh_file_type type;
    int ret = 0;

    if (strcmp(KIBOSH_CONTROL_PATH, path) == 0) {
        type = KIBOSH_FILE_TYPE_CONTROL;
        ret = kibosh_fs_control_stat(fs, stbuf);
    } else {
        char bpath[PATH_MAX];
        type = KIBOSH_FILE_TYPE_NORMAL;
        snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
        if (stat(bpath, stbuf) < 0) {
            ret = -errno;
        }
    }
    DEBUG("kibosh_getattr(path=%s, type=%s) = %d (%s)\n",
          path, kibosh_file_type_str(type), -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_getxattr(const char *path, const char *name, char *value, size_t size)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX], *nvalue = NULL;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (getxattr(bpath, name, value, size) < 0) {
        ret = -errno;
    }
    if (global_kibosh_log_settings & KIBOSH_LOG_DEBUG_ENABLED) {
        if (ret == 0) {
            nvalue = alloc_zterm_xattr(value, size);
            if (!nvalue) {
                ret = -ENOMEM;
                goto done;
            }
            DEBUG("kibosh_getxattr(path=%s, bpath=%s, name=%s, value=%s) = 0\n",
                  path, bpath, name, nvalue);
        } else {
            DEBUG("kibosh_getxattr(path=%s, bpath=%s, name=%s) = %d (%s)\n",
                  path, bpath, name, -ret, safe_strerror(-ret));
        }
    }
done:
    free(nvalue);
    return AS_FUSE_ERR(ret);
}

int kibosh_link(const char *oldpath, const char *newpath) 
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (link(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_link(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, -ret,
          safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_listxattr(const char *path, char *list, size_t size)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (listxattr(bpath, list, size) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_listxattr(path=%s, bpath=%s, list=%s, size=%zd) = %d (%s)\n",
          path, bpath, list, size, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_mkdir(const char *path, mode_t mode)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mkdir(bpath, mode) < 0) {
        ret = -errno;
    }
    if (chown(bpath, fuse_get_context()->uid, fuse_get_context()->gid) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_mkdir(path=%s, bpath=%s, mode=%04o) = %d\n",
          path, bpath, mode, ret);
    return AS_FUSE_ERR(ret);
}

int kibosh_mknod(const char *path, mode_t mode, dev_t dev)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mknod(bpath, mode, dev) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_mknod(path=%s, bpath=%s, mode=%04o, dev=%"PRId64") = %d\n",
          path, bpath, mode, (int64_t)dev, ret);
    return AS_FUSE_ERR(ret);
}

int kibosh_opendir(const char *path, struct fuse_file_info *info)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    DIR *dp;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    dp = opendir(bpath);
    if (!dp) {
        ret = -errno;
    } else {
        struct kibosh_dir *dir = kibosh_dir_alloc(path, dp);
        info->fh = (uintptr_t)dir;
    }
    DEBUG("kibosh_opendir(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_readdir(const char *path UNUSED, void *buf,
                fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
{
    struct kibosh_dir *dir = (struct kibosh_dir *)(uintptr_t)info->fh;
    DIR *dp = dir->dp;
    struct dirent *de;
    int ret = 0;

    DEBUG("kibosh_readdir(dir->path=%s, offset=%"PRId64") begin\n",
          dir->path, (int64_t)offset);
    if (offset != 0) {
        seekdir(dp, (long)offset);
    }
    while (1) {
        // Note: calling readdir() is thread-safe on Linux, but not on all POSIX platforms.
        errno = 0;
        de = readdir(dp); 
        if (!de) {
            if (errno) {
                ret = -errno;
                break;
            }
            ret = 0;
            break;
        }
        if ((de->d_name[0] == '.') && ((de->d_name[1] == '\0') ||
             ((de->d_name[1] == '.') && (de->d_name[2] == '\0')))) {
            continue;
        }
        // We're using the directory filler API here.  This avoids the need to
        // fetch all the directory entries at once.
        // TODO: can we avoid calling telldir for every entry in the directory?
        offset = telldir(dp);
        if (filler(buf, de->d_name, NULL, offset)) {
            ret = 1;
            break;
        }
    }
    if (global_kibosh_log_settings & KIBOSH_LOG_DEBUG_ENABLED) {
        const char *exit_reason;
        if (ret < 0) {
            exit_reason = safe_strerror(-ret);
        } else if (ret == 1) {
            exit_reason = "filler satisified";
        } else {
            exit_reason = "no more entries";
        }
        DEBUG("kibosh_readdir(dir->path=%s, offset=%"PRId64"): %s\n",
              dir->path, (int64_t)offset, exit_reason);
    }
    if (ret < 0)
        return -ret;
    else
        return 0;
}

int kibosh_readlink(const char *path, char *buf, size_t size)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    ssize_t res;
    char bpath[PATH_MAX];
    int ret = 0;

    // POSIX semantics are a bit different than FUSE semantics... POSIX doesn't
    // require NULL-termination, but FUSE does.  POSIX also returns the length
    // of the text we fetched, but FUSE does not.
    if (size <= 0) {
        ret = 0;
        goto done;
    }
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    res = readlink(bpath, buf, size);
    if (res < 0) {
        ret = -errno;
        goto done;
    }
    buf[res] = '\0';
    ret = 0;

done:
    DEBUG("kibosh_readlink(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_releasedir(const char *path UNUSED, struct fuse_file_info *info)
{
    struct kibosh_dir *dir = (struct kibosh_dir *)(uintptr_t)info->fh;
    int ret = 0;

    if (closedir(dir->dp) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_releasedir(dir->path=%s) = %d (%s)\n",
          dir->path, -ret, safe_strerror(-ret));
    free(dir);
    return AS_FUSE_ERR(ret);
}

int kibosh_removexattr(const char *path, const char *name)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (removexattr(bpath, name) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_removexattr(path=%s, bpath=%s, name=%s) = %d (%s)\n",
          path, bpath, name, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_rename(const char *oldpath, const char *newpath) 
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (rename(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_rename(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, -ret,
          safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_rmdir(const char *path)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (rmdir(bpath) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_rmdir(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    errno = 0;
    return AS_FUSE_ERR(ret);
}

int kibosh_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX], *nvalue = NULL;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (setxattr(bpath, name, value, size, flags) < 0) {
        ret = -errno;
    }
    if (global_kibosh_log_settings & KIBOSH_LOG_DEBUG_ENABLED) {
        nvalue = alloc_zterm_xattr(value, size);
        if (!nvalue) {
            ret = -ENOMEM;
            goto done;
        }
        DEBUG("kibosh_setxattr(path=%s, bpath=%s, value=%s) = %d (%s)\n",
              path, bpath, nvalue, -ret, safe_strerror(-ret));
    }
done:
    free(nvalue);
    return AS_FUSE_ERR(ret);
}

int kibosh_statfs(const char *path, struct statvfs *vfs)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (statvfs(bpath, vfs) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_statfs(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_symlink(const char *oldpath, const char *newpath)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (symlink(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_symlink(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, -ret,
          safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_truncate(const char *path, off_t off)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (truncate(bpath, off) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_truncate(path=%s, bpath=%s, off=%"PRId64") = %d (%s)\n",
          path, bpath, (int64_t)off, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_unlink(const char *path)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (unlink(bpath) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_unlink(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

int kibosh_utime(const char *path, struct utimbuf *buf)
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utime(bpath, buf) < 0) {
        ret = -errno;
    }
    if (!buf) {
        // If buf == NULL, we attempted to set the times to the current time.
        DEBUG("kibosh_utime(path=%s, bpath=%s, actime=NULL, "
              "modtime=NULL) = %d (%s)\n", path, bpath, -ret, safe_strerror(-ret));
    } else {
        DEBUG("kibosh_utime(path=%s, bpath=%s, actime=%"PRId64", "
              "modtime=%"PRId64") = %d (%s)\n", path, bpath,
              (int64_t)buf->actime, (int64_t)buf->modtime, -ret, safe_strerror(-ret));
    }
    return AS_FUSE_ERR(ret);
}

int kibosh_utimens(const char *path, const struct timespec tv[2])
{
    struct kibosh_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utimensat(AT_FDCWD, bpath, tv, 0) < 0) {
        ret = -errno;
    }
    DEBUG("kibosh_utimens(path=%s, atime.tv_sec=%"PRId64", "
        "atime.tv_nsec=%"PRId64", mtime.tv_sec=%"PRId64", "
        "mtime.tv_nsec=%"PRId64") = %d (%s)\n",
        path, tv[0].tv_sec, tv[0].tv_nsec, tv[1].tv_sec, tv[1].tv_nsec,
        -ret, safe_strerror(-ret));
    return AS_FUSE_ERR(ret);
}

// vim: ts=4:sw=4:tw=99:et
