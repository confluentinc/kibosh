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
 * The maximum length of a Kibosh fault type string.
 */
#define KIBOSH_FAULT_TYPE_STR_LEN 32

/**
 * Constant flags for byte corruption modes.
 * The mode numbers are chosen to be distinguished form ERRNO numbers (1 to 122).
 * Random position modes are grouped in 1000s range and sequential byte modes are grouped in 1100s range
 * so that more corruption modes can be added later for each group.
 */
typedef enum {
    CORRUPT_ZERO = 1000,        // replace bytes at random positions with \0
    CORRUPT_RAND = 1001,        // replace bytes at random positions with random char value
    CORRUPT_ZERO_SEQ = 1100,    // replace sequential bytes at the end of the file with \0
    CORRUPT_RAND_SEQ = 1101,    // replace sequential bytes at the end of the file with random char value
    CORRUPT_DROP = 1200,        // silently drop bytes at the end of the file
} corrupt_mode;

/**
 * Base class for Kibosh faults.
 */
struct kibosh_fault_base {
    /**
     * The type of fault.
     */
    char type[KIBOSH_FAULT_TYPE_STR_LEN];
};

/**
 * The type of kibosh_fault_unreadable.
 */
#define KIBOSH_FAULT_TYPE_UNREADABLE "unreadable"

/**
 * The type of kibosh_fault_read_delay.
 */
#define KIBOSH_FAULT_TYPE_READ_DELAY "read_delay"

/**
 * The type of kibosh_fault_unwritable.
 */
#define KIBOSH_FAULT_TYPE_UNWRITABLE "unwritable"

/**
 * The type of kibosh_fault_write_delay.
 */
#define KIBOSH_FAULT_TYPE_WRITE_DELAY "write_delay"

/**
 * The type of kibosh_fault_read_corrupt.
 */
#define KIBOSH_FAULT_TYPE_READ_CORRUPT "read_corrupt"

/**
 * The type of kibosh_fault_write_corrupt.
 */
#define KIBOSH_FAULT_TYPE_WRITE_CORRUPT "write_corrupt"

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
     * The fraction of reads that are delayed.
     */
    double fraction;
};

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
     * The fraction of writes that are delayed.
     */
    double fraction;
};

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
     * Mode of byte corruption.
     * 1000 -> zeros (default)
     * 1001 -> random values
     * 1100 -> sequential zeros
     * 1101 -> sequential random values
     * 1200 -> silently drop a fraction of bytes at the end of buffer
     */
    int mode;

    /**
     * Number of corruption fault injected before switching to unwritable fault.
     * Less than 0 means never switch.
     */
    int count;

    /**
     * The fraction of bytes to be corrupted.
     */
    double fraction;
};

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
     * Mode of byte corruption.
     * 1000 -> zeros (default)
     * 1001 -> random values
     * 1100 -> sequential zeros
     * 1101 -> sequential random values
     * 1200 -> silently drop a fraction of bytes at the end of buffer
     */
    int mode;

    /**
     * Number of corruption fault injected before switching to CORRUPT_DROP with fraction = 1.0.
     * Less than 0 means never switch.
     */
    int count;

    /**
     * The fraction of bytes to be corrupted.
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
 * @param fault     The fault structure.
 * @param path      The path.
 * @param op        The operation.
 *
 * @return          0 if no fault should be injected; an error code greater than 0 if we should
 *                  inject the corresponding fault. (Note: Linux FUSE requires negative return
 *                  codes.)
 */
int kibosh_fault_base_check(struct kibosh_fault_base *fault, const char *path, const char *op);

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
 * Check whether a given kibosh FS operation should trigger one of the configured faults.
 *
 * @param faults    The faults structure.
 * @param path      The path.
 * @param op        The operation.
 *
 * @return          0 if no fault should be injected; an error code greater than 0 if we should
 *                  inject the corresponding fault. (Note: Linux FUSE requires negative return
 *                  codes.)
 */
int faults_check(struct kibosh_faults *faults, const char *path, const char *op);

/**
 * Free a dynamically allocated kibosh_faults structure.
 *
 * @param faults    The structure to free.
 */
void faults_free(struct kibosh_faults *faults);

#endif

// vim: ts=4:sw=4:tw=99:et
