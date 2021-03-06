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
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/////
///// kibosh_fault_unreadable
/////
static void kibosh_fault_unreadable_free(struct kibosh_fault_unreadable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_unreadable *kibosh_fault_unreadable_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_unreadable *fault = NULL;
    json_value *code_obj = NULL;

    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_UNREADABLE;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("%s: No valid \"code\" field found in fault object.\n", __func__);
        goto error;
    }
    fault->code = code_obj->u.integer;
    return fault;

error:
    kibosh_fault_unreadable_free(fault);
    return NULL;
}

static char *kibosh_fault_unreadable_unparse(struct kibosh_fault_unreadable *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"code\":%d}",
                    KIBOSH_FAULT_TYPE_UNREADABLE_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->code);
}

static int kibosh_fault_unreadable_matches(struct kibosh_fault_unreadable *fault,
                                           const char *path, const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return 1;
}

static int kibosh_fault_unreadable_apply(struct kibosh_fault_unreadable *fault,
                                         uint32_t *delay_ms)
{
    *delay_ms = 0;
    return fault->code < 0 ? fault->code : -fault->code;
}

/////
///// kibosh_fault_read_delay
/////
static void kibosh_fault_read_delay_free(struct kibosh_fault_read_delay *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_read_delay *kibosh_fault_read_delay_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_read_delay *fault = NULL;
    json_value *delay_ms_obj = NULL;
    json_value *fraction_obj = NULL;

    delay_ms_obj = get_child(obj, "delay_ms");
    if ((!delay_ms_obj) || (delay_ms_obj->type != json_integer)) {
        INFO("%s: No valid \"delay_ms\" field found in fault object.\n", __func__);
        goto error;
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("%s: No valid \"fraction\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_READ_DELAY;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    fault->delay_ms = delay_ms_obj->u.integer;
    fault->fraction = fraction_obj->u.dbl;
    return fault;

error:
    kibosh_fault_read_delay_free(fault);
    return NULL;
}

static char *kibosh_fault_read_delay_unparse(struct kibosh_fault_read_delay *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"delay_ms\":%"PRId32"d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_READ_DELAY_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->delay_ms,
                    fault->fraction);
}

static int kibosh_fault_read_delay_matches(struct kibosh_fault_read_delay *fault,
                                           const char *path, const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return (drand48() <= fault->fraction);
}

static void kibosh_fault_read_delay_apply(struct kibosh_fault_read_delay *fault,
                                         uint32_t *delay_ms)
{
    *delay_ms = fault->delay_ms;
}

/////
///// kibosh_fault_unwritable
/////
static void kibosh_fault_unwritable_free(struct kibosh_fault_unwritable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_unwritable *kibosh_fault_unwritable_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_unwritable *fault = NULL;
    json_value *code_obj = NULL;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("%s: No valid \"code\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_UNWRITABLE;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    fault->code = code_obj->u.integer;
    return fault;

error:
    kibosh_fault_unwritable_free(fault);
    return NULL;
}

static char *kibosh_fault_unwritable_unparse(struct kibosh_fault_unwritable *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"code\":%d}",
                    KIBOSH_FAULT_TYPE_UNWRITABLE_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->code);
}

static int kibosh_fault_unwritable_matches(struct kibosh_fault_unwritable *fault,
                                           const char *path, const char *op)
{
    if (strcmp(op, "write") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return 1;
}

static int kibosh_fault_unwritable_apply(struct kibosh_fault_unwritable *fault,
                                         char **dyanmic_buf, uint32_t *delay_ms)
{
    *dyanmic_buf = NULL;
    *delay_ms = 0;
    return (fault->code < 0) ? fault->code : -fault->code;
}

/////
///// kibosh_fault_write_delay
/////
static void kibosh_fault_write_delay_free(struct kibosh_fault_write_delay *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_write_delay *kibosh_fault_write_delay_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_write_delay *fault = NULL;
    json_value *delay_ms_obj = NULL;
    json_value *fraction_obj = NULL;

    delay_ms_obj = get_child(obj, "delay_ms");
    if ((!delay_ms_obj) || (delay_ms_obj->type != json_integer)) {
        INFO("%s: No valid \"delay_ms\" field found in fault object.\n", __func__);
        goto error;
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("%s: No valid \"fraction\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_WRITE_DELAY;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    fault->delay_ms = delay_ms_obj->u.integer;
    fault->fraction = fraction_obj->u.dbl;
    return fault;

error:
    kibosh_fault_write_delay_free(fault);
    return NULL;
}

static char *kibosh_fault_write_delay_unparse(struct kibosh_fault_write_delay *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"delay_ms\":%"PRId32"d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_WRITE_DELAY_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->delay_ms,
                    fault->fraction);
}

static int kibosh_fault_write_delay_matches(struct kibosh_fault_write_delay *fault,
                                            const char *path, const char *op)
{
    if (strcmp(op, "write") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return (drand48() <= fault->fraction);
}

static int kibosh_fault_write_delay_apply(struct kibosh_fault_write_delay *fault,
                                          char **dyanmic_buf, uint32_t *delay_ms, int size)
{
    *dyanmic_buf = NULL;
    *delay_ms = fault->delay_ms;
    return size;
}

/////
///// kibosh_fault_read_corrupt
/////
static void kibosh_fault_read_corrupt_free(struct kibosh_fault_read_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_read_corrupt *kibosh_fault_read_corrupt_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_read_corrupt *fault = NULL;
    json_value *mode_obj = NULL;
    json_value *count_obj = NULL;
    json_value *fraction_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("%s: No valid \"mode\" field found in fault object.\n", __func__);
        goto error;
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("%s: No valid \"fraction\" field found in fault object.\n", __func__);
        goto error;
    }
    count_obj = get_child(obj, "count");
    if ((!count_obj) || (count_obj->type != json_integer)) {
        INFO("%s: No valid \"count\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_READ_CORRUPT;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    fault->mode = mode_obj->u.integer;
    fault->count = count_obj->u.integer;
    fault->fraction = fraction_obj->u.dbl;
    return fault;

error:
    kibosh_fault_read_corrupt_free(fault);
    return NULL;
}

static char *kibosh_fault_read_corrupt_unparse(struct kibosh_fault_read_corrupt *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"mode\":%d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_READ_CORRUPT_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->mode,
                    fault->fraction);
}

static int kibosh_fault_read_corrupt_matches(struct kibosh_fault_read_corrupt *fault,
                                             const char *path, const char *op)
{
    if (strcmp(op, "read") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return 1;
}

static int kibosh_fault_read_corrupt_apply(struct kibosh_fault_read_corrupt *fault,
                                           char *buf, int nread, uint32_t *delay_ms)
{
    *delay_ms = 0;
    // If count > 0, then we will transition to CORRUPT_DROP after 'count' tries.
    // If count is negative, then it is ignored.
    if (fault->count > 0) {
        fault->count--;
    } else if (fault->count == 0) {
        fault->mode = CORRUPT_DROP;
        fault->fraction = 1.0;
    }
    return corrupt_buffer(buf, nread, fault->mode, fault->fraction);
}

/////
///// kibosh_fault_write_corrupt
/////
static void kibosh_fault_write_corrupt_free(struct kibosh_fault_write_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static struct kibosh_fault_write_corrupt *kibosh_fault_write_corrupt_parse(json_value *obj)
{
    int ret;
    struct kibosh_fault_write_corrupt *fault = NULL;
    json_value *mode_obj = NULL;
    json_value *count_obj = NULL;
    json_value *fraction_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("%s: No valid \"mode\" field found in fault object.\n", __func__);
        goto error;
    }
    fraction_obj = get_child(obj, "fraction");
    if ((!fraction_obj) || (fraction_obj->type != json_double)) {
        INFO("%s: No valid \"fraction\" field found in fault object.\n", __func__);
        goto error;
    }
    count_obj = get_child(obj, "count");
    if ((!count_obj) || (count_obj->type != json_integer)) {
        INFO("%s: No valid \"count\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    fault->base.type = KIBOSH_FAULT_TYPE_WRITE_CORRUPT;
    ret = dup_json_str_value(get_child(obj, "prefix"), "/", &fault->prefix);
    if (ret) {
        INFO("%s: error reading \"prefix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    ret = dup_json_str_value(get_child(obj, "suffix"), "", &fault->suffix);
    if (ret) {
        INFO("%s: error reading \"suffix\" field: %s (%d)\n",
             __func__, safe_strerror(ret), ret);
        goto error;
    }
    fault->mode = mode_obj->u.integer;
    fault->count = count_obj->u.integer;
    fault->fraction = fraction_obj->u.dbl;
    return fault;

error:
    kibosh_fault_write_corrupt_free(fault);
    return NULL;
}

static char *kibosh_fault_write_corrupt_unparse(struct kibosh_fault_write_corrupt *fault)
{
    return dynprintf("{\"type\":\"%s\", "
                    "\"prefix\":\"%s\", "
                    "\"suffix\":\"%s\", "
                    "\"mode\":%d, "
                    "\"fraction\":%g}",
                    KIBOSH_FAULT_TYPE_WRITE_CORRUPT_NAME,
                    fault->prefix,
                    fault->suffix,
                    fault->mode,
                    fault->fraction);
}

static int kibosh_fault_write_corrupt_matches(struct kibosh_fault_write_corrupt *fault,
                                              const char *path, const char *op)
{
    if (strcmp(op, "write") != 0) {
        return 0;
    }
    if (strncmp(path, fault->prefix, strlen(fault->prefix)) != 0) {
        return 0;
    }
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return 1;
}

static int kibosh_fault_write_corrupt_apply(struct kibosh_fault_write_corrupt *fault,
                    const char **buf, char **dynamic_buf, uint32_t *delay_ms, int size)
{
    char *dbuf;

    *delay_ms = 0;
    // If count > 0, then we will transition to CORRUPT_DROP after 'count' tries.
    // If count is negative, then it is ignored.
    if (fault->count > 0) {
        fault->count--;
    } else if (fault->count == 0) {
        fault->mode = CORRUPT_DROP;
        fault->fraction = 1.0;
    }
    if (fault->mode == CORRUPT_DROP) {
        *dynamic_buf = 0;
        return drand48() * size;
    }
    dbuf = malloc(size);
    if (!dbuf) {
        return -ENOMEM;
    }
    memcpy(dbuf, *buf, size);
    *buf = dbuf;
    *dynamic_buf = dbuf;
    return corrupt_buffer(dbuf, size, fault->mode, fault->fraction);
}

/////
///// kibosh_fault_base 
/////
struct kibosh_fault_base *kibosh_fault_base_parse(json_value *obj)
{
    json_value *child;

    child = get_child(obj, "type");
    if (!child) {
        INFO("%s: No \"type\" field found in root object.\n", __func__);
        return NULL;
    }
    if (child->type != json_string) {
        INFO("%s: \"type\" field was not a string.\n", __func__);
        return NULL;
    }
    if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNREADABLE_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unreadable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_DELAY_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_delay_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_WRITE_DELAY_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_write_delay_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNWRITABLE_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unwritable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_CORRUPT_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_corrupt_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_WRITE_CORRUPT_NAME) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_write_corrupt_parse(obj);
    }
    INFO("%s: Unknown fault type \"%s\".\n", __func__, child->u.string.ptr);
    return NULL;
}

char *kibosh_fault_base_unparse(struct kibosh_fault_base *fault)
{
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNREADABLE:
            return kibosh_fault_unreadable_unparse(
                    (struct kibosh_fault_unreadable*)fault);
        case KIBOSH_FAULT_TYPE_READ_DELAY:
            return kibosh_fault_read_delay_unparse(
                    (struct kibosh_fault_read_delay*)fault);
        case KIBOSH_FAULT_TYPE_WRITE_DELAY:
            return kibosh_fault_write_delay_unparse(
                    (struct kibosh_fault_write_delay*)fault);
        case KIBOSH_FAULT_TYPE_UNWRITABLE:
            return kibosh_fault_unwritable_unparse(
                    (struct kibosh_fault_unwritable*)fault);
        case KIBOSH_FAULT_TYPE_READ_CORRUPT:
            return kibosh_fault_read_corrupt_unparse(
                    (struct kibosh_fault_read_corrupt*)fault);
        case KIBOSH_FAULT_TYPE_WRITE_CORRUPT:
            return kibosh_fault_write_corrupt_unparse(
                    (struct kibosh_fault_write_corrupt*)fault);
    }
    return NULL;
}

int kibosh_fault_matches(struct kibosh_fault_base *fault, const char *path, const char *op)
{
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNREADABLE:
            return kibosh_fault_unreadable_matches(
                    (struct kibosh_fault_unreadable*)fault, path, op);
        case KIBOSH_FAULT_TYPE_READ_DELAY:
            return kibosh_fault_read_delay_matches(
                        (struct kibosh_fault_read_delay*)fault, path, op);
        case KIBOSH_FAULT_TYPE_WRITE_DELAY:
            return kibosh_fault_write_delay_matches(
                        (struct kibosh_fault_write_delay*)fault, path, op);
        case KIBOSH_FAULT_TYPE_UNWRITABLE:
            return kibosh_fault_unwritable_matches(
                        (struct kibosh_fault_unwritable*)fault, path, op);
        case KIBOSH_FAULT_TYPE_READ_CORRUPT:
            return kibosh_fault_read_corrupt_matches(
                        (struct kibosh_fault_read_corrupt*)fault, path, op);
        case KIBOSH_FAULT_TYPE_WRITE_CORRUPT:
            return kibosh_fault_write_corrupt_matches(
                        (struct kibosh_fault_write_corrupt*)fault, path, op);
    }
    return 0;
}

void kibosh_fault_base_free(struct kibosh_fault_base *fault)
{
    if (!fault)
        return;
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNREADABLE:
            kibosh_fault_unreadable_free((struct kibosh_fault_unreadable*)fault);
            break;
        case KIBOSH_FAULT_TYPE_READ_DELAY:
            kibosh_fault_read_delay_free((struct kibosh_fault_read_delay*)fault);
            break;
        case KIBOSH_FAULT_TYPE_WRITE_DELAY:
            kibosh_fault_write_delay_free((struct kibosh_fault_write_delay*)fault);
            break;
        case KIBOSH_FAULT_TYPE_UNWRITABLE:
            kibosh_fault_unwritable_free((struct kibosh_fault_unwritable*)fault);
            break;
        case KIBOSH_FAULT_TYPE_READ_CORRUPT:
            kibosh_fault_read_corrupt_free((struct kibosh_fault_read_corrupt*)fault);
            break;
        case KIBOSH_FAULT_TYPE_WRITE_CORRUPT:
            kibosh_fault_write_corrupt_free((struct kibosh_fault_write_corrupt*)fault);
            break;
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

const char *kibosh_fault_type_name(struct kibosh_fault_base *fault)
{
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNREADABLE:
            return KIBOSH_FAULT_TYPE_UNREADABLE_NAME;
        case KIBOSH_FAULT_TYPE_READ_DELAY:
            return KIBOSH_FAULT_TYPE_READ_DELAY_NAME;
        case KIBOSH_FAULT_TYPE_UNWRITABLE:
            return KIBOSH_FAULT_TYPE_UNWRITABLE_NAME;
        case KIBOSH_FAULT_TYPE_WRITE_DELAY:
            return KIBOSH_FAULT_TYPE_WRITE_DELAY_NAME;
        case KIBOSH_FAULT_TYPE_READ_CORRUPT:
            return KIBOSH_FAULT_TYPE_READ_CORRUPT_NAME;
        case KIBOSH_FAULT_TYPE_WRITE_CORRUPT:
            return KIBOSH_FAULT_TYPE_WRITE_CORRUPT_NAME;
        default:
            return "(unknown)";
    }
}

/////
///// kibosh_faults
/////
static int fault_array_parse(json_value *arr, struct kibosh_faults **out)
{
    struct kibosh_faults *faults = NULL;
    int ret = -EIO, i, num_faults = 0;

    if (arr->type != json_array) {
        INFO("%s: \"faults\" was not an array.\n", __func__);
        goto done;
    }
    num_faults = arr->u.array.length;

    faults = calloc(1, sizeof(*faults));
    if (!faults) {
        INFO("%s: out of memory when trying to allocate faults structure.\n", __func__);
        goto done;
    }
    faults->list = calloc(num_faults + 1, sizeof(struct kibosh_fault_base *));
    if (!faults->list) {
        INFO("%s: out of memory when trying to allocate a list of %d faults.\n", __func__,
             num_faults);
        goto done;
    }
    for (i = 0; i < num_faults; i++) {
        faults->list[i] = (struct kibosh_fault_base*)
                kibosh_fault_base_parse(arr->u.array.values[i]);
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
        INFO("%s: failed to parse input string of length %zd: %s\n", __func__,
             strlen(str), error);
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

struct kibosh_fault_base *find_first_fault(struct kibosh_faults *faults,
                                      const char *path, const char *op)
{
    struct kibosh_fault_base **iter;

    for (iter = faults->list; *iter; iter++) {
        struct kibosh_fault_base *fault = *iter;
        if (kibosh_fault_matches(fault, path, op)) {
            return fault;
        }
    }
    return NULL;
}

int apply_read_fault(struct kibosh_fault_base *fault, char *buf, int nread,
                     uint32_t *delay_ms)
{
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNREADABLE:
            return kibosh_fault_unreadable_apply((struct kibosh_fault_unreadable *) fault,
                                                 delay_ms);
        case KIBOSH_FAULT_TYPE_READ_DELAY:
            kibosh_fault_read_delay_apply((struct kibosh_fault_read_delay *) fault,
                                          delay_ms);
            return nread;
        case KIBOSH_FAULT_TYPE_READ_CORRUPT:
            return kibosh_fault_read_corrupt_apply((struct kibosh_fault_read_corrupt *) fault,
                                                   buf, nread, delay_ms);
        default:
            *delay_ms = 0;
            return nread;
    }
}

int apply_write_fault(struct kibosh_fault_base *fault, const char **buf, char **dynamic_buf,
                      int size, uint32_t *delay_ms)
{
    switch (fault->type) {
        case KIBOSH_FAULT_TYPE_UNWRITABLE:
            return kibosh_fault_unwritable_apply((struct kibosh_fault_unwritable *) fault,
                                                 dynamic_buf, delay_ms);
        case KIBOSH_FAULT_TYPE_WRITE_DELAY:
            return kibosh_fault_write_delay_apply((struct kibosh_fault_write_delay *) fault,
                                                  dynamic_buf, delay_ms, size);
        case KIBOSH_FAULT_TYPE_WRITE_CORRUPT:
            return kibosh_fault_write_corrupt_apply(
                    (struct kibosh_fault_write_corrupt *) fault, buf, dynamic_buf,
                    delay_ms, size);
        default:
            *delay_ms = 0;
            *dynamic_buf = NULL;
            return size;
    }
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

int corrupt_buffer(char *buf, int size, enum buffer_corruption_type mode, double fraction)
{
    int i;

    switch(mode) {
        case CORRUPT_ZERO:
            for (i = 0; i < size; i++) {
                if (drand48() <= fraction) {
                    buf[i] = '\0';
                }
            }
            return size;

        case CORRUPT_RAND:
            for (i = 0; i < size; i++) {
                if (drand48() <= fraction) {
                    buf[i] = lrand48() & 0xff;
                }
            }
            return size;

        case CORRUPT_RAND_SEQ:
            for (i = drand48() * size; i < size; i++) {
                buf[i] = lrand48() & 0xff;
            }
            return size;

        case CORRUPT_ZERO_SEQ:
            i = drand48() * size;
            memset(buf + i, 0, size - i);
            return size;

        case CORRUPT_DROP:
            return drand48() * size;
    }
    return size;
}

// vim: ts=4:sw=4:tw=99:et
