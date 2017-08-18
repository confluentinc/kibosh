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

#ifndef KIBOSH_META_H
#define KIBOSH_META_H

#include <fuse.h>
#include <unistd.h> // for size_t
#include <sys/types.h> // for mode_t, dev_t

struct stat;
struct statvfs;
struct utimbuf;

int kibosh_chmod(const char *path, mode_t mode);
int kibosh_chown(const char *path, uid_t uid, gid_t gid);
int kibosh_fsyncdir(const char *path, int datasync, struct fuse_file_info *info);
int kibosh_getattr(const char *path, struct stat *stbuf);
int kibosh_getxattr(const char *path, const char *name, char *value, size_t size);
int kibosh_link(const char *oldpath, const char *newpath);
int kibosh_listxattr(const char *path, char *list, size_t size);
int kibosh_mkdir(const char *path, mode_t mode);
int kibosh_mknod(const char *path, mode_t mode, dev_t dev);
int kibosh_opendir(const char *path, struct fuse_file_info *info);
int kibosh_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *info);
int kibosh_readlink(const char *path, char *buf, size_t size);
int kibosh_releasedir(const char *path, struct fuse_file_info *info);
int kibosh_removexattr(const char *path, const char *name);
int kibosh_rename(const char *oldpath, const char *newpath);
int kibosh_rmdir(const char *path);
int kibosh_setxattr(const char *path, const char *name, const char *value,
                    size_t size, int flags);
int kibosh_statfs(const char *path, struct statvfs *vfs);
int kibosh_symlink(const char *oldpath, const char *newpath);
int kibosh_truncate(const char *path, off_t off);
int kibosh_unlink(const char *path);
int kibosh_utime(const char *path, struct utimbuf *buf);
int kibosh_utimens(const char *path, const struct timespec tv[2]);

#endif

// vim: ts=4:sw=4:tw=99:et
