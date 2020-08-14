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

/**
 * fault free functions
 */
static void kibosh_fault_unreadable_free(struct kibosh_fault_unreadable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static void kibosh_fault_read_delay_free(struct kibosh_fault_read_delay *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static void kibosh_fault_unwritable_free(struct kibosh_fault_unwritable *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static void kibosh_fault_write_delay_free(struct kibosh_fault_write_delay *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static void kibosh_fault_read_corrupt_free(struct kibosh_fault_read_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

static void kibosh_fault_write_corrupt_free(struct kibosh_fault_write_corrupt *fault)
{
    if (fault) {
        free(fault->prefix);
        free(fault->suffix);
        free(fault);
    }
}

/**
 * kibosh_fault_unreadable 
 */
static struct kibosh_fault_unreadable *kibosh_fault_unreadable_parse(json_value *obj)
{
    struct kibosh_fault_unreadable *fault = NULL;
    json_value *code_obj = NULL;
    json_value *prefix_obj = NULL;
    json_value *suffix_obj = NULL;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("%s: No valid \"code\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNREADABLE);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_UNREADABLE,
                    fault->prefix,
                    fault->suffix,
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
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
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
    json_value *delay_ms_obj = NULL;
    json_value *prefix_obj = NULL;
    json_value *suffix_obj = NULL;
    json_value *fraction_obj = NULL;

    delay_ms_obj = get_child(obj, "delay_ms");
    if ((!delay_ms_obj) || (delay_ms_obj->type != json_integer)) {
        INFO("%s: No valid \"delay_ms\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
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
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_READ_DELAY);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_READ_DELAY,
                    fault->prefix,
                    fault->suffix,
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
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    // apply fraction
    if (drand48() <= fault->fraction) {
        INFO("[read_delay fault injected] {path=%s, prefix=%s, suffix=%s, fraction=%g, delay_ms=%d}\n",
            path, fault->prefix, fault->suffix, fault->fraction, fault->delay_ms);
        milli_sleep(fault->delay_ms);
    }
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
    json_value *suffix_obj = NULL;

    code_obj = get_child(obj, "code");
    if ((!code_obj) || (code_obj->type != json_integer)) {
        INFO("%s: No valid \"code\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
        goto error;
    }
    fault = calloc(1, sizeof(*fault));
    if (!fault) {
        INFO("%s: OOM\n", __func__);
        return NULL;
    }
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNWRITABLE);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_UNWRITABLE,
                    fault->prefix,
                    fault->suffix,
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
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    return fault->code;
}

/**
 * kibosh_fault_write_delay
 */
static struct kibosh_fault_write_delay *kibosh_fault_write_delay_parse(json_value *obj)
{
    struct kibosh_fault_write_delay *fault = NULL;
    json_value *delay_ms_obj = NULL;
    json_value *prefix_obj = NULL;
    json_value *suffix_obj = NULL;
    json_value *fraction_obj = NULL;

    delay_ms_obj = get_child(obj, "delay_ms");
    if ((!delay_ms_obj) || (delay_ms_obj->type != json_integer)) {
        INFO("%s: No valid \"delay_ms\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
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
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_WRITE_DELAY);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_WRITE_DELAY,
                    fault->prefix,
                    fault->suffix,
                    fault->delay_ms,
                    fault->fraction);
}

static int kibosh_fault_write_delay_check(struct kibosh_fault_write_delay *fault, const char *path,
                                            const char *op)
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
    // apply fraction
    if (drand48() <= fault->fraction) {
        INFO("[write_delay fault injected] {path=%s, prefix=%s, suffix=%s, fraction=%g, delay_ms=%d}\n",
            path, fault->prefix, fault->suffix, fault->fraction, fault->delay_ms);
        milli_sleep(fault->delay_ms);
    }
    return 0;
}

/**
 * kibosh_fault_read_corrupt
 */
static struct kibosh_fault_read_corrupt *kibosh_fault_read_corrupt_parse(json_value *obj)
{
    struct kibosh_fault_read_corrupt *fault = NULL;
    json_value *mode_obj = NULL;
    json_value *count_obj = NULL;
    json_value *prefix_obj = NULL;
    json_value *fraction_obj = NULL;
    json_value *suffix_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("%s: No valid \"mode\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
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
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_READ_CORRUPT);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_READ_CORRUPT,
                    fault->prefix,
                    fault->suffix,
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
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    if (fault->count > 0) {
        fault->count--;
    } else if (fault->count == 0) {
        fault->mode = CORRUPT_DROP;
        fault->fraction = 1.0;
    }
    return fault->mode;
}

/**
 * kibosh_fault_write_corrupt
 */
static struct kibosh_fault_write_corrupt *kibosh_fault_write_corrupt_parse(json_value *obj)
{
    struct kibosh_fault_write_corrupt *fault = NULL;
    json_value *mode_obj = NULL;
    json_value *count_obj = NULL;
    json_value *prefix_obj = NULL;
    json_value *fraction_obj = NULL;
    json_value *suffix_obj = NULL;

    mode_obj = get_child(obj, "mode");
    if ((!mode_obj) || (mode_obj->type != json_integer)) {
        INFO("%s: No valid \"mode\" field found in fault object.\n", __func__);
        goto error;
    }
    prefix_obj = get_child(obj, "prefix");
    if ((!prefix_obj) || (prefix_obj->type != json_string)) {
        INFO("%s: No valid \"prefix\" field found in fault object.\n", __func__);
        goto error;
    }
    suffix_obj = get_child(obj, "suffix");
    if ((!suffix_obj) || (suffix_obj->type != json_string)) {
        INFO("%s: No valid \"suffix\" field found in fault object.\n", __func__);
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
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_WRITE_CORRUPT);
    fault->prefix = strdup(prefix_obj->u.string.ptr);
    if (!fault->prefix) {
        INFO("%s: OOM\n", __func__);
        goto error;
    }
    fault->suffix = strdup(suffix_obj->u.string.ptr);
    if (!fault->suffix) {
        INFO("%s: OOM\n", __func__);
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
                    KIBOSH_FAULT_TYPE_WRITE_CORRUPT,
                    fault->prefix,
                    fault->suffix,
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
    if (strlen(fault->suffix) > strlen(path) ||
        strcmp(path+(strlen(path)-strlen(fault->suffix)), fault->suffix) != 0) {
        return 0;
    }
    if (fault->count > 0) {
        fault->count--;
    } else if (fault->count == 0) {
        fault->mode = CORRUPT_DROP;
        fault->fraction = 1.0;
    }
    return fault->mode;
}

/**
 * kibosh_fault_base 
 */
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
    if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unreadable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_delay_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_WRITE_DELAY) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_write_delay_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_UNWRITABLE) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_unwritable_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_READ_CORRUPT) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_read_corrupt_parse(obj);
    } else if (strcmp(child->u.string.ptr, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) == 0) {
        return (struct kibosh_fault_base *)kibosh_fault_write_corrupt_parse(obj);
    }
    INFO("%s: Unknown fault type \"%s\".\n", __func__, child->u.string.ptr);
    return NULL;
}

char *kibosh_fault_base_unparse(struct kibosh_fault_base *fault)
{
    if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNREADABLE) == 0) {
        return kibosh_fault_unreadable_unparse((struct kibosh_fault_unreadable*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_DELAY) == 0) {
        return kibosh_fault_read_delay_unparse((struct kibosh_fault_read_delay*)fault);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_DELAY) == 0) {
        return kibosh_fault_write_delay_unparse((struct kibosh_fault_write_delay*)fault);
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
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_DELAY) == 0) {
        return kibosh_fault_write_delay_check((struct kibosh_fault_write_delay*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_UNWRITABLE) == 0) {
        return kibosh_fault_unwritable_check((struct kibosh_fault_unwritable*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_READ_CORRUPT) == 0) {
        return kibosh_fault_read_corrupt_check((struct kibosh_fault_read_corrupt*)fault, path, op);
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_CORRUPT) == 0) {
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
    } else if (strcmp(fault->type, KIBOSH_FAULT_TYPE_WRITE_DELAY) == 0) {
        kibosh_fault_write_delay_free((struct kibosh_fault_write_delay*)fault);
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
        INFO("%s: failed to parse input string of length %zd: %s\n", __func__, strlen(str), error);
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
        if (err_code) {
            // get the smallest err_codes
            // i.e. when unreadable and read_corrupt faults both present, we want to execute unreadable
            if (!ret || err_code < ret) {
                ret = err_code;
            }
        }
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
