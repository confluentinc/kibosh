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
 * Class for Kibosh faults that make files unreadable.
 */
struct kibosh_fault_unreadable {
    /**
     * The base class members.
     */
    struct kibosh_fault_base base;

    /**
     * The error code to return from read faults.
     */
    int code;
};

struct kibosh_faults {
    /**
     * A NULL-terminated list of pointers to fault objects.
     */
    struct kibosh_fault_base **list;
};

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
 * Free the memory associated with a fault objectg.
 *
 * @param fault     The fault object.
 */
void kibosh_fault_base_free(struct kibosh_fault_base *fault);

/**
 * JSON for an empty kibosh_faults object.
 */
#define FAULTS_EMPTY_JSON "{\"faults\":[]}"

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
