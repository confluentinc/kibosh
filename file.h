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

#ifndef KIBOSH_FILE_H
#define KIBOSH_FILE_H

#include <fuse.h>
#include <sys/types.h> // for mode_t, dev_t
#include <unistd.h> // for size_t

int kibosh_create(const char *path, mode_t mode, struct fuse_file_info *info);
int kibosh_fallocate(const char *path, int mode, off_t offset,
                     off_t len, struct fuse_file_info *info);
int kibosh_fgetattr(const char *path, struct stat *stat, struct fuse_file_info *info);
int kibosh_flush(const char *path, struct fuse_file_info *info);
int kibosh_fsync(const char *path, int datasync, struct fuse_file_info *info);
int kibosh_ftruncate(const char *path, off_t len, struct fuse_file_info *info);
int kibosh_open(const char *path, struct fuse_file_info *info);
int kibosh_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *info);
int kibosh_release(const char *path, struct fuse_file_info *info);
int kibosh_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info);

enum kibosh_file_type {
    /**
     * A normal file.
     */
    KIBOSH_FILE_TYPE_NORMAL = 0,

    /**
     * The Kibosh control file.
     */
    KIBOSH_FILE_TYPE_CONTROL = 1,
};

struct kibosh_file {
    /**
     * The type of file which this is.
     */
    enum kibosh_file_type type;

    /**
     * The backing file descriptor associated with this file.
     */
    int fd;

    /**
     * The path of this file when it was opened, as a NULL-terminated string.
     *
     * Note that if the inode is renamed, or a parent directory is renamed, this
     * will not be updated.  This path is used to decide when to inject faults.
     */
    char path[0];
};

/**
 * Get a string identifying the file type.
 *
 * @param type      The file type.
 *
 * @return          A statically allocated string.
 */
const char *kibosh_file_type_str(enum kibosh_file_type type);

#endif

// vim: ts=4:sw=4:tw=99:et
