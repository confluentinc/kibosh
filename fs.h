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

#ifndef KIBOSH_FS_H
#define KIBOSH_FS_H

#include <pthread.h> // for pthread_mutex_t

#define KIBOSH_CONTROL          "kibosh_control"
#define KIBOSH_CONTROL_PATH     ("/" KIBOSH_CONTROL)

struct stat;

struct kibosh_fs {
    /**
     * The root of the pass-through filesystem.  Immutable.
     */
    char *root;

    /**
     * The lock that protects control_fd, faults, and control_buf.
     */
    pthread_mutex_t lock;

    /**
     * The current control file.
     */
    int control_fd;

    /**
     * The current set of faults.  Protected by the lock.
     */
    struct kibosh_faults *faults;

    /**
     * The buffer used to read the control fd.
     */
    char *control_buf;
};

/**
 * Allocate a new kibosh_fs object.
 *
 * @param out       (out param) the new FS object.
 * @param root      The path to the root.  Will be shallow-copied.
 *
 * @return          0 on success; a negative error code otherwise.
 */
int kibosh_fs_alloc(struct kibosh_fs **out, char *root);

/**
 * Fill in the stat structure for the control file.
 *
 * @param fs        The kibosh_fs
 * @param stbuf     (out param) the stat structure.
 *
 * @return          0 on success; a negative error code otherwise.
 */
int kibosh_fs_control_stat(struct kibosh_fs *fs, struct stat *stbuf);

/**
 * Create a new accessor file descriptor.
 *
 * @param fs        The kibosh_fs.
 * @param populate  1 if we should copy the existing control file; 0 otherwise.
 *
 * @return          The fd on success; a negative error code otherwise.
 */
int kibosh_fs_accessor_fd_alloc(struct kibosh_fs *fs, int populate);

/**
 * Release an accessor file descriptor.
 *
 * This will update the configured faults if necessary.
 *
 * @param fs        The kibosh_fs
 * @param fd        The accessor file descriptor to release.
 *
 * @return          0 on success; a negative error code otherwise.
 */
int kibosh_fs_accessor_fd_release(struct kibosh_fs *fs, int fd);

/**
 * Check if we should inject a read fault.
 *
 * @param fs        The kibosh_fs.
 * @param path      The path being read.
 *
 * @return          0 if we should not inject a fault; the error code otherwise.
 */
int kibosh_fs_check_read_fault(struct kibosh_fs *fs, const char *path);

#endif

// vim: ts=4:sw=4:tw=99:et
