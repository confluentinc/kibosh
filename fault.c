/**
 * Copyright 2017-2019 Confluent Inc.
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
#include "time.h"
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
    struct kibosh_fault_unreadable *fault = NULL;
    json_value *code_obj = NULL;
    json_value *prefix_obj = NULL;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("kibosh_fault_unreadable_parse: No valid \"code\" field found in fault object.\n");
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("kibosh_fault_unreadable_parse: No valid \"prefix\" field found in fault object.\n");
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_unreadable_parse: OOM\n");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNREADABLE);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("kibosh_fault_unreadable_parse: OOM\n");
        goto error;
    }
    fault->code = code_obj->u.integer;
    return fault;

error:
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
    return NULL;
}

static char *kibosh_fault_unreadable_unparse(struct kibosh_fault_unreadable *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"code\":%d}",
                    KIBOSH_FAULT_TYPE_UNREADABLE,
                    fault->prefix,
                    fault->code);
}

static int kibosh_fault_unreadable_check(struct kibosh_fault_unreadable *fault, const char *path,
                                         const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    return fault->code;
}

/**
 * kibosh_fault_read_delay
 */
static struct kibosh_fault_read_delay *kibosh_fault_read_delay_parse(json_value *obj)
{
    struct kibosh_fault_read_delay *fault = NULL;
    json_value *delay_ms_obj = NULL, *prefix_obj = NULL, *fraction_obj = NULL;

    delay_ms_obj = get_child(obj, "delay_ms");
    if ((!delay_ms_obj) || (delay_ms_obj->type != json_integer)) {
        INFO("kibosh_fault_read_delay_parse: No valid \"delay_ms\" field found in fault object.\n");
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("kibosh_fault_read_delay_parse: No valid \"prefix\" field found in fault object.\n");
        goto error;
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("kibosh_fault_read_delay_parse: No valid \"fraction\" field found in fault object.\n");
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_read_delay_parse: OOM\n");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_READ_DELAY);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("kibosh_fault_read_delay_parse: OOM\n");
        goto error;
    }
    fault->delay_ms = delay_ms_obj->u.integer;
    fault->fraction = fraction_obj->u.dbl;
    return fault;

error:
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
    return NULL;
}

static char *kibosh_fault_read_delay_unparse(struct kibosh_fault_read_delay *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"delay_ms\":%"PRId32"d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_READ_DELAY,
                     fault->prefix,
                     fault->delay_ms,
                     fault->fraction);
}

static int kibosh_fault_read_delay_check(struct kibosh_fault_read_delay *fault, const char *path,
                                            const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    // apply fraction
    srand((int) round(time(0)*RAND_FRAC));
    if (RAND_FRAC <= fault->fraction)
        milli_sleep(fault->delay_ms);
    return 0;
}

/**
 * kibosh_fault_unwritable
 */
static struct kibosh_fault_unwritable *kibosh_fault_unwritable_parse(json_value *obj)
{
    struct kibosh_fault_unwritable *fault = NULL;
    json_value *code_obj = NULL;
    json_value *prefix_obj = NULL;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("kibosh_fault_unwritable_parse: No valid \"code\" field found in fault object.\n");
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("kibosh_fault_unwritable_parse: No valid \"prefix\" field found in fault object.\n");
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_unwritable_parse: OOM\n");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNWRITABLE);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("kibosh_fault_unwritable_parse: OOM\n");
        goto error;
    }
    fault->code = code_obj->u.integer;
    return fault;

error:
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
    return NULL;
}

static char *kibosh_fault_unwritable_unparse(struct kibosh_fault_unwritable *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"code\":%d}",
                    KIBOSH_FAULT_TYPE_UNWRITABLE,
                    fault->prefix,
                    fault->code);
}

static int kibosh_fault_unwritable_check(struct kibosh_fault_unwritable *fault, const char *path,
                                         const char *op)
{
    if (strcmp(op, "write") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    return fault->code;
}

/**
 * kibosh_fault_read_corrupt
 */
static struct kibosh_fault_read_corrupt *kibosh_fault_read_corrupt_parse(json_value *obj)
{
    struct kibosh_fault_read_corrupt *fault = NULL;
    json_value *mode_obj = NULL, *prefix_obj = NULL, *fraction_obj = NULL, *file_type_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"mode\" field found in fault object.\n");
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"prefix\" field found in fault object.\n");
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_read_corrupt_parse: OOM\n");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_READ_CORRUPT);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("kibosh_fault_read_corrupt_parse: OOM\n");
        goto error;
    }
    file_type_obj = get_child(obj, "file_type");
    if ((!file_type_obj) || (file_type_obj->type != json_string)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"file_type\" field found in fault object, will apply read_corrupt to all files.\n");
        fault->file_type = NULL;
    } else {
        fault->file_type = strdup(file_type_obj->u.string.ptr);
        if (!fault->file_type) {
            INFO("kibosh_fault_read_corrupt_parse: OOM\n");
            goto error;
        }
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"fraction\" field found in fault object, will apply read_corrupt to all bytes\n");
        fault->fraction = 0.5;
    } else {
        fault->fraction = fraction_obj->u.dbl;
    }
    fault->mode = mode_obj->u.integer;
    return fault;

error:
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
    return NULL;
}

static char *kibosh_fault_read_corrupt_unparse(struct kibosh_fault_read_corrupt *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"mode\":%d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_READ_DELAY,
                    fault->prefix,
                    fault->mode,
                    fault->fraction);
}

static int kibosh_fault_read_corrupt_check(struct kibosh_fault_read_corrupt *fault, const char *path,
                                            const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (fault->file_type != NULL && strstr(path, fault->file_type) == NULL) {
        return 0;
    }
    return fault->mode;
}

/**
 * kibosh_fault_write_corrupt
 */
static struct kibosh_fault_write_corrupt *kibosh_fault_write_corrupt_parse(json_value *obj)
{
    struct kibosh_fault_write_corrupt *fault = NULL;
    json_value *mode_obj = NULL, *prefix_obj = NULL, *fraction_obj = NULL, *file_type_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("kibosh_fault_write_corrupt_parse: No valid \"mode\" field found in fault object.\n");
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("kibosh_fault_write_corrupt_parse: No valid \"prefix\" field found in fault object.\n");
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("kibosh_fault_write_corrupt_parse: OOM\n");
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_WRITE_CORRUPT);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("kibosh_fault_read_corrupt_parse: OOM\n");
        goto error;
    }
    file_type_obj = get_child(obj, "file_type");
    if ((!file_type_obj) || (file_type_obj->type != json_string)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"file_type\" field found in fault object, will apply read_corrupt to all files.\n");
        fault->file_type = NULL;
    } else {
        fault->file_type = strdup(file_type_obj->u.string.ptr);
        if (!fault->file_type) {
            INFO("kibosh_fault_read_corrupt_parse: OOM\n");
            goto error;
        }
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("kibosh_fault_read_corrupt_parse: No valid \"fraction\" field found in fault object, will apply read_corrupt to all bytes\n");
        fault->fraction = 0.5;
    } else {
        fault->fraction = fraction_obj->u.dbl;
    }
    fault->mode = mode_obj->u.integer;
    return fault;

error:
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
    return NULL;
}

static char *kibosh_fault_write_corrupt_unparse(struct kibosh_fault_write_corrupt *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"mode\":%d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_READ_DELAY,
                    fault->prefix,
                    fault->mode,
                    fault->fraction);
}

static int kibosh_fault_write_corrupt_check(struct kibosh_fault_write_corrupt *fault, const char *path,
                                            const char *op)
{
    if (strcmp(op, "write") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (fault->file_type != NULL && strstr(path, fault->file_type) == NULL) {
        return 0;
    }
    return fault->mode;
}

static void kibosh_fault_unreadable_free(struct kibosh_fault_unreadable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
}

static void kibosh_fault_read_delay_free(struct kibosh_fault_read_delay *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
}

static void kibosh_fault_unwritable_free(struct kibosh_fault_unwritable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
}

static void kibosh_fault_read_corrupt_free(struct kibosh_fault_read_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
}

static void kibosh_fault_write_corrupt_free(struct kibosh_fault_write_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault);
    }
}

/**
 * kibosh_fault_base 
 */
struct kibosh_fault_base *kibosh_fault_base_parse(json_value *obj)
{
    json_value *child;

    child = get_child(obj, "type");
    if (!child) {
        INFO("kibosh_fault_base: No \"type\" field found in root object.\n");
        return NULL;
    }
    if (child->type != json_string) {
        INFO("kibosh_fault_base: \"type\" field was not a string.\n");
        return NULL;
    }
    if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unreadable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_delay_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNWRITABLE) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unwritable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_CORRUPT) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_corrupt_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_write_corrupt_parse(obj);
    }
    INFO("kibosh_fault_base: Unknown fault type \"%s\".\n", child->u.string.ptr);
    return NULL;
}

char *kibosh_fault_base_unparse(struct kibosh_fault_base *fault)
{
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return kibosh_fault_unreadable_unparse((struct kibosh_fault_unreadable*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        return kibosh_fault_read_delay_unparse((struct kibosh_fault_read_delay*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNWRITABLE) == 0) {
        return kibosh_fault_unwritable_unparse((struct kibosh_fault_unwritable*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_CORRUPT) == 0) {
        return kibosh_fault_read_corrupt_unparse((struct kibosh_fault_read_corrupt*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) == 0) {
        return kibosh_fault_write_corrupt_unparse((struct kibosh_fault_write_corrupt*)fault);
    }
    return NULL;
}

int kibosh_fault_base_check(struct kibosh_fault_base *fault, const char *path, const char *op)
{
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return kibosh_fault_unreadable_check((struct kibosh_fault_unreadable*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        return kibosh_fault_read_delay_check((struct kibosh_fault_read_delay*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNWRITABLE) ==0) {
        return kibosh_fault_unwritable_check((struct kibosh_fault_unwritable*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_CORRUPT) ==0) {
        return kibosh_fault_read_corrupt_check((struct kibosh_fault_read_corrupt*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) ==0) {
        return kibosh_fault_write_corrupt_check((struct kibosh_fault_write_corrupt*)fault, path, op);
    }
    return -ENOSYS;
}

void kibosh_fault_base_free(struct kibosh_fault_base *fault)
{
    if (!fault)
        return;
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        kibosh_fault_unreadable_free((struct kibosh_fault_unreadable*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        kibosh_fault_read_delay_free((struct kibosh_fault_read_delay*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNWRITABLE) == 0) {
        kibosh_fault_unwritable_free((struct kibosh_fault_unwritable*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_CORRUPT) == 0) {
        kibosh_fault_read_corrupt_free((struct kibosh_fault_read_corrupt*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) == 0) {
        kibosh_fault_write_corrupt_free((struct kibosh_fault_write_corrupt*)fault);
    }
}

int faults_calloc(struct kibosh_faults **out)
{
    struct kibosh_faults *faults;

    faults = calloc(1, sizeof(struct kibosh_faults));
    if (!faults)
        return -ENOMEM;
    faults->list = calloc(1, sizeof(struct kibosh_fault_base *));
    if (!faults->list) {
        free(faults);
        return -ENOMEM;
    }
    *out = faults;
    return 0;
}

/**
 * kibosh_faults 
 */
static int fault_array_parse(json_value *arr, struct kibosh_faults **out)
{
    struct kibosh_faults *faults = NULL;
    int ret = -EIO, i, num_faults = 0;

    if (arr->type != json_array) {
        INFO("fault_array_parse: \"faults\" was not an array.\n");
        goto done;
    }
    num_faults = arr->u.array.length;

    faults = calloc(1, sizeof(*faults));
    if (!faults) {
        INFO("fault_array_parse: out of memory when trying to allocate faults structure.\n");
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
        *out = calloc(1, sizeof(*faults));
        if (!*out) {
            ret = -ENOMEM;
            goto done;
        }
        (*out)->list = calloc(1, sizeof(struct kibosh_fault_base*));
        if (!(*out)->list) {
            ret = -ENOMEM;
            free(*out);
            goto done;
        }
    } else {
        ret = fault_array_parse(faults, out);
        if (ret)
            goto done;
    }
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

    int ret = 0;

    for (iter = faults->list; *iter; iter++) {
        int err_code = kibosh_fault_base_check(*iter, path, op);
        if (err_code)
            // get the smallest err_codes
            // i.e. when unreadable and read_corrupt faults both present, we want to execute unreadable
            if (!ret || err_code < ret)
                ret = err_code;
    }
    return ret;
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
