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

#ifndef KIBOSH_IO_H
#define KIBOSH_IO_H

#include <unistd.h> // for ssize_t, size_t

/**
 * Write a series of bytes to a file.
 *
 * @param fd        The file descriptor.
 * @param b         The buffer.
 * @param c         The number of bytes to write.
 *
 * @return          A negative error number on error, or 0 on success.
 */
ssize_t safe_write(int fd, const void *b, size_t c);

/**
 * Write a null-terminated string to a file.
 *
 * @param path      The path.
 * @param str       The null-terminated string..
 *
 * @return          A negative error number on error, or 0 on success.
 */
int write_string_to_file(const char *path, const char *str);

/**
 * Read a series of bytes from a file.
 *
 * @param fd        The file descriptor.
 * @param buf       The buffer.
 * @param buf_len   The maximum number of bytes to read.
 *
 * @return          A negative error number on error, or the number of bytes
 *                  read on success.
 */
ssize_t safe_read(int fd, void *b, size_t c);

/**
 * Read a null-terminated string from the given file descriptor.
 *
 * @param fd        The file descriptor.
 * @param buf       The buffer.
 * @param buf_len   The length of the buffer.
 *
 * @return          a negative error number on error, or 0 on success.
 */
int read_string_from_fd(int fd, char *buf, size_t buf_len);

/**
 * Read a null-terminated string from the given file path.
 *
 * @param path      The path.
 * @param buf       The buffer.
 * @param buf_len   The length of the buffer.
 *
 * @return          a negative error number on error, or 0 on success.
 */
int read_string_from_file(const char *path, char *buf, size_t buf_len);

/**
 * Copy the content of one file descriptor in another, using an intermediate buffer.
 *
 * @param dest_fd   The destination fd.
 * @param src_fd    The source fd.
 *
 * @return          0 on success; a negative error code otherwise.
 */
int duplicate_fd(int dest_fd, int src_fd);

#endif

// vim: ts=4:sw=4:tw=99:et
