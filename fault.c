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

#include "fault.h"
#include "json.h"
#include "log.h"
#include "util.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * kibosh_fault_unreadable 
 */
static struct kibosh_fault_unreadable *kibosh_fault_unreadable_parse(json_value *obj)
{
    struct kibosh_fault_unreadable *fault;
    json_value *code_obj;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("kibosh_fault_unreadable_parse: No \"code\" field found in fault object.");
        return NULL;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_unreadable_parse: OOM");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNREADABLE);
    fault->code = code_obj->u.integer;
    return fault;
}

static char *kibosh_fault_unreadable_unparse(struct kibosh_fault_unreadable *fault)
{
    return dynprintf("{\"type\":\"unreadable\", \"code\":%d}", fault->code);
}

static int kibosh_fault_unreadable_check(struct kibosh_fault_unreadable *fault, const char *path UNUSED,
                                         const char *op)
{
    if (strcmp(op, "read") == 0) {
        return fault->code;
    }
    return 0;
}

static void kibosh_fault_unreadable_free(struct kibosh_fault_unreadable *fault)
{
    free(fault);
}

/**
 * kibosh_fault_base 
 */
struct kibosh_fault_base *kibosh_fault_base_parse(json_value *obj)
{
    json_value *child;

    child = get_child(obj, "type");
    if (!child) {
        INFO("kibosh_fault_base: No \"type\" field found in root object.");
        return NULL;
    }
    if (child->type != json_string) {
        INFO("kibosh_fault_base: \"type\" field was not a string.");
        return NULL;
    }
    if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unreadable_parse(obj);
    }
    return NULL;
}

char *kibosh_fault_base_unparse(struct kibosh_fault_base *fault)
{
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return kibosh_fault_unreadable_unparse((struct kibosh_fault_unreadable*)fault);
    }
    return NULL;
}

int kibosh_fault_base_check(struct kibosh_fault_base *fault, const char *path, const char *op)
{
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return kibosh_fault_unreadable_check((struct kibosh_fault_unreadable*)fault, path, op);
    }
    return -ENOSYS;
}

void kibosh_fault_base_free(struct kibosh_fault_base *fault)
{
    if (!fault)
        return;
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        kibosh_fault_unreadable_free((struct kibosh_fault_unreadable*)fault);
    }
}

/**
 * kibosh_faults 
 */
static int fault_array_parse(json_value *arr, struct kibosh_faults **out)
{
    struct kibosh_faults *faults = NULL;
    int ret = -EIO, i, num_faults = 0;

    if (arr->type != json_array) {
        INFO("fault_array_parse: \"faults\" was not an array.");
        goto done;
    }
    num_faults = arr->u.array.length;

    faults = calloc(1, sizeof(*faults));
    if (!faults) {
        INFO("fault_array_parse: out of memory when trying to allocate faults structure.");
        goto done;
    }
    faults->list = calloc(num_faults + 1, sizeof(struct kibosh_fault_base *));
    if (!faults->list) {
        INFO("fault_array_parse: out of memory when trying to allocate a list of %d faults.\n",
             num_faults);
        goto done;
    }
    for (i = 0; i < num_faults; i++) {
        faults->list[i] = (struct kibosh_fault_base*)kibosh_fault_base_parse(arr->u.array.values[i]);
        if (!faults->list[i]) {
            goto done;
        }
    }
    ret = 0;
done:
    if (ret) {
        if (faults) {
            for (i = 0; i < num_faults; i++) {
                kibosh_fault_base_free(faults->list[i]);
            }
            if (faults->list) {
                free(faults->list);
            }
            free(faults);
        }
        *out = NULL;
        return ret;
    }
    *out = faults;
    return 0;
}

int faults_parse(const char *str, struct kibosh_faults **out)
{
    int ret = -EIO;
    char error[json_error_max] = { 0 };
    json_value *root = NULL, *faults;
    json_settings settings = { 0 };

    root = json_parse_ex(&settings, str, strlen(str), error);
    if (!root) {
        INFO("faults_parse: failed to parse input string of length %zd: %s\n", strlen(str), error);
        goto done;
    }
    faults = get_child(root, "faults");
    if (!faults) {
        INFO("faults_parse: failed to parse input string: there was no \"faults\" array "
             "in the root object.");
        goto done;
    }
    ret = fault_array_parse(faults, out);
    if (ret)
        goto done;
    ret = 0;
done:
    if (root) {
        json_value_free(root);
    }
    return ret;
}

char *faults_unparse(const struct kibosh_faults *faults)
{
    int num_faults = 0;
    struct kibosh_fault_base **iter;
    char **strs = NULL;
    char *json = NULL, **ptr;

    for (iter = faults->list; *iter; iter++) {
        num_faults++;
    }
    strs = calloc(sizeof(char*), (2 * num_faults) + 3);
    if (!strs)
        goto done;
    ptr = strs;
    *ptr = strdup("{\"faults\":[");
    if (!(*ptr))
        goto done;
    ptr++;
    for (iter = faults->list; *iter; iter++) {
        if (iter != faults->list) {
            *ptr = strdup(", ");
            if (!(*ptr))
                goto done;
            ptr++;
        }
        *ptr = kibosh_fault_base_unparse(*iter);
        if (!(*ptr))
            goto done;
        ptr++;
    }
    *ptr = strdup("]}");
    if (!*ptr)
        goto done;
    ptr++;
    *ptr = NULL;
    json = join_strs(strs);
    if (!json)
        goto done;

done:
    if (strs) {
        for (ptr = strs; *ptr; ptr++) {
            free(*ptr);
        }
        free(strs);
        strs = NULL;
    }
    return json;
}

int faults_check(struct kibosh_faults *faults, const char *path, const char *op)
{
    struct kibosh_fault_base **iter;

    for (iter = faults->list; *iter; iter++) {
        int ret = kibosh_fault_base_check(*iter, path, op);
        if (ret)
            return ret;
    }
    return 0;
}

void faults_free(struct kibosh_faults *faults)
{
    struct kibosh_fault_base **iter;

    for (iter = faults->list; *iter; iter++) {
        kibosh_fault_base_free(*iter);
    }
    free(faults->list);
    faults->list = NULL;
    free(faults);
}

// vim: ts=4:sw=4:tw=99:et
