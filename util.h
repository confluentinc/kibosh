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

#ifndef KIBOSH_UTIL_H
#define KIBOSH_UTIL_H

#include "json.h" // for json_value
#include <unistd.h> // for size_t
#include <math.h> // for round()

/**
 * Like snprintf, but appends to a string that already exists.
 *
 * @param str       The string to append to
 * @param str_len   Maximum length allowed for str
 * @param fmt       Printf-style format string
 * @param ...       Printf-style arguments
 *
 * @return          0 on success; -ENAMETOOLONG if there was not enough
 *                      buffer space.
 */
int snappend(char *str, size_t str_len, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * A version of printf that dynamically allocates the string.
 *
 * @param fmt       The format string.
 * @param ...       Printf-style arguments
 *
 * @return          The dynamically allocated string on success; NULL on OOM
 */
char *dynprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * Create a dynamically allocated string which is the concatenation of a list
 * of strings.
 *
 * @param strs      A NULL-terminated list of strings to concatenate.
 *
 * @return          NULL on OOM, or a dynamically allocated concatenated
 *                  string otherwise.
 */
char *join_strs(char **strs);

/**
 * Given a JSON object, get the child with the given name.
 *
 * @param obj       The JSON object.
 *
 * @return          NULL if the json value was not an object;
 *                  NULL if the child was not found;
 *                  The child node otherwise.
 */
json_value *get_child(json_value *obj, const char *name);

/**
 * Convert a POSIX-open flags integer to a string.
 *
 * @param flags     The flags.
 * @param str       (out paramer) The buffer to write to.
 * @param max_len   Maximum length of the buffer to write to.
 *
 * @return          0 on success; -ENAMETOOLONG if there was not enough
 *                      buffer space.
 */
int open_flags_to_str(int flags, char *str, size_t max_len);

/**
 * Recursively unlink a path.
 *
 * @param path      the path to unlink
 *
 * @return          0 on success; negative error code otherwise
 */
int recursive_unlink(const char *name);

/**
 * Allocate a file descriptor stored entirely in memory, which is not accessible
 * from the filesystem namespace.
 *
 * @param name          The name to use for debugging purposes.
 * @param mode          The mode to use on the new fd.
 *
 * @return              A negative error code on failure; the file descriptor otherwise.
 */
int memfd_create(const char *name, int mode);

#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

#ifdef __GNUC__
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH
#endif

/**
 * Convert a potentially negative error code into a FUSE-style positive error code.
 */
#define AS_FUSE_ERR(x) ((x > 0) ? (-x) : x)

#endif

// vim: ts=4:sw=4:et
