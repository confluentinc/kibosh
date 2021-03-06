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

#ifndef KIBOSH_FAULT_H
#define KIBOSH_FAULT_H

#include "json.h"

/**
 * The type of kibosh fault.
 */
enum kibosh_fault_type {
    KIBOSH_FAULT_TYPE_UNREADABLE,
    KIBOSH_FAULT_TYPE_READ_DELAY,
    KIBOSH_FAULT_TYPE_UNWRITABLE,
    KIBOSH_FAULT_TYPE_WRITE_DELAY,
    KIBOSH_FAULT_TYPE_READ_CORRUPT,
    KIBOSH_FAULT_TYPE_WRITE_CORRUPT,
};

/**
 * Buffer corruption strategies.
 * TODO: we should not require users to enter a numeric code for these, but have strings
 * for them instead.
 */
enum buffer_corruption_type {
    /**
     * Replace bytes at random positions with zero bytes.
     */
    CORRUPT_ZERO = 1000,

    /**
     * Replace bytes at random positions with random byte values.
     */
    CORRUPT_RAND = 1001,

    /**
     * Replace sequential bytes at the end of the file with zero bytes.
     */
    CORRUPT_ZERO_SEQ = 1100,

    /**
     * Replace sequential bytes at the end of the file with random bytes values.
     */
    CORRUPT_RAND_SEQ = 1101,

    /**
     * Silently drop bytes at the end of the file.
     */
    CORRUPT_DROP = 1200,
};

/**
 * Base class for Kibosh faults.
 */
struct kibosh_fault_base {
    /**
     * The type of fault.
     */
    enum kibosh_fault_type type;
};

/**
 * The name of the kibosh_fault_unreadable type.
 */
#define KIBOSH_FAULT_TYPE_UNREADABLE_NAME "unreadable"

/**
 * The class for Kibosh faults that make files unreadable.
 */
struct kibosh_fault_unreadable {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
     * The path suffix, can be used to specify a file extension.
     */
    char *suffix;

    /**
     * The error code to return from read faults.
     */
    int code;
};

/**
 * The name of the kibosh_fault_read_delay type.
 */
#define KIBOSH_FAULT_TYPE_READ_DELAY_NAME "read_delay"

/**
 * The class for Kibosh faults that lead to delays when reading.
 */
struct kibosh_fault_read_delay {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
     * The path suffix, can be used to specify a file extension.
     */
    char *suffix;

    /**
     * The number of milliseconds to delay the read.
     */
    uint32_t delay_ms;

    /**
     * The fraction of reads that are delayed. This should be a value between 0.0 and 1.0 inclusive.
     */
    double fraction;
};

/**
 * The name of the kibosh_fault_unwritable type.
 */
#define KIBOSH_FAULT_TYPE_UNWRITABLE_NAME "unwritable"

/**
 * The class for Kibosh faults that make files unwritable.
 */
struct kibosh_fault_unwritable {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
     * The path suffix, can be used to specify a file extension.
     */
    char *suffix;

    /**
     * The error code to return from write faults.
     */
    int code;
};

/**
 * The name of the kibosh_fault_write_delay type.
 */
#define KIBOSH_FAULT_TYPE_WRITE_DELAY_NAME "write_delay"

/**
 * The class for Kibosh faults that lead to delays when writing.
 */
struct kibosh_fault_write_delay {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
     * The path suffix, can be used to specify a file extension.
     */
    char *suffix;

    /**
     * The number of milliseconds to delay the read.
     */
    uint32_t delay_ms;

    /**
     * The fraction of writes that are delayed. This should be a value between 0.0 and 1.0 inclusive.
     */
    double fraction;
};

/**
 * The name of the kibosh_fault_read_corrupt type.
 */
#define KIBOSH_FAULT_TYPE_READ_CORRUPT_NAME "read_corrupt"

/**
 * The class for Kibosh faults that lead to corrupted data when reading.
 */
struct kibosh_fault_read_corrupt {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
     * The path suffix, can be used to specify a file extension.
     */
    char *suffix;

    /**
     * The mode of read corruption.
     */
    enum buffer_corruption_type mode;

    /**
     * Number of corruption fault injected before switching to unwritable fault.
     * Less than 0 means never switch.
     */
    int count;

    /**
     * The fraction of bytes to be corrupted. This should be a value between 0.0 and 1.0 inclusive.
     */
    double fraction;
};

/**
 * The name of the kibosh_fault_write_corrupt type.
 */
#define KIBOSH_FAULT_TYPE_WRITE_CORRUPT_NAME "write_corrupt"

/**
 * The class for Kibosh faults that lead to corrupted data when writing.
 */
struct kibosh_fault_write_corrupt {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The path prefix, starts with '/'.
     */
    char *prefix;

    /**
    * The path suffix, can be used to specify a file extension.
    */
    char *suffix;

    /**
     * The mode of write corruption.
     */
    enum buffer_corruption_type mode;

    /**
     * Number of corruption fault injected before switching to CORRUPT_DROP with fraction = 1.0.
     * Less than 0 means never switch.
     */
    int count;

    /**
     * The fraction of bytes to be corrupted. This should be a value between 0.0 and 1.0 inclusive.
     */
    double fraction;
};

struct kibosh_faults {
    /**
     * A NULL-terminated list of pointers to fault objects.
     */
    struct kibosh_fault_base **list;
};

/**
 * Allocate an empty Kibosh faults structure.
 *
 * @param out       (out parameter) the new Kibosh faults structure
 *
 * @return          0 on success, or the negative error code on error.
 */
int faults_calloc(struct kibosh_faults **out);

/**
 * Return the name of the fault type.
 *
 * @return          A constant string which is the name of the fault type.
 */
const char *kibosh_fault_type_name(struct kibosh_fault_base *fault);

/**
 * Parse a JSON string as a fault object.
 *
 * @param str       The string to parse.
 *
 * @return          the dynamically allocated kibosh_faults structure, or NULL on error.
 */
struct kibosh_fault_base *kibosh_fault_base_parse(json_value *obj);

/**
 * Convert a fault object into a JSON string.
 *
 * @param fault     The fault object.
 *
 * @return          A dynamically allocated JSON string on success; NULL on OOM.
 */
char *kibosh_fault_base_unparse(struct kibosh_fault_base *fault);

/**
 * Check whether a given kibosh FS operation should trigger this fault.
 *
 * @param fault     The fault.
 * @param path      The path.
 * @param op        The operation.
 *
 * @return          1 if the fault should not be injected; 0 otherwise.
 */
int kibosh_fault_matches(struct kibosh_fault_base *fault, const char *path, const char *op);

/**
 * Free the memory associated with a fault object.
 *
 * @param fault     The fault object.
 */
void kibosh_fault_base_free(struct kibosh_fault_base *fault);

/**
 * Parse a JSON string as a faults object.
 *
 * @param str       The string to parse.
 * @param out       (out param) the dynamically allocated kibosh_faults structure.
 *
 * @return          0 on success; an error code greater than 0 on failure.
 */
int faults_parse(const char *str, struct kibosh_faults **out);

/**
 * Convert a faults object into a JSON string.
 *
 * @param faults    The faults object.
 *
 * @return          A dynamically allocated JSON string on success; NULL on OOM.
 */
char *faults_unparse(const struct kibosh_faults *faults);

/**
 * Find the first fault that applies to the given path and operation.
 *
 * @param faults    The faults structure.
 * @param path      The path.
 * @param op        The operation.
 *
 * @return          NULL if no applicable fault could be found; the fault otherwise.
 */
struct kibosh_fault_base *find_first_fault(struct kibosh_faults *faults,
                                           const char *path, const char *op);

/**
 * Apply a fault during a read operation.
 *
 * @param fault     The fault to apply.
 * @param buf       The read buffer.
 * @param nread     The size of the read buffer.
 * @param delay_ms  (out param) the number of milliseconds to delay.
 *
 * @return          The result to return from the read operation.
 */
int apply_read_fault(struct kibosh_fault_base *fault, char *buf, int nread,
                     uint32_t *delay_ms);

/**
 * Apply a fault during a write operation.
 *
 * @param fault         The fault to apply.
 * @param buf           (inout) The write buffer.  May be changed if needed.
 * @param dynamic_buf   (output) If this function allocates a new buffer, it will be
 *                      stored here, so that the caller can free it later.
 * @param nread         The size of the write buffer.
 * @param delay_ms      (out param) the number of milliseconds to delay.
 *
 * @return              The result to return from the write operation.
 */
int apply_write_fault(struct kibosh_fault_base *fault, const char **buf, char **dynamic_buf,
                      int size, uint32_t *delay_ms);

/**
 * Free a dynamically allocated kibosh_faults structure.
 *
 * @param faults    The structure to free.
 */
void faults_free(struct kibosh_faults *faults);

/**
 * Corrupt a buffer.
 *
 * @param buf       The buffer.
 * @param size      The size.
 * @param mode      The corruption mode to use.
 * @param fraction  The fraction of bytes to corrupt.  Only needed for certain modes.
 *
 * @return          The new length to use for the buffer.
 */
int corrupt_buffer(char *buf, int size, enum buffer_corruption_type mode,
                    double fraction);

#endif

// vim: ts=4:sw=4:tw=99:et
