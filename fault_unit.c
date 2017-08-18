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
#include "log.h"
#include "test.h"
#include "util.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct kibosh_fault_unreadable *kibosh_fault_unreadable_alloc(int code)
{
    struct kibosh_fault_unreadable *fault;
    fault = calloc(1, sizeof(*fault));
    snprintf(fault->base.type, KIBOSH_FAULT_TYPE_STR_LEN, "%s", KIBOSH_FAULT_TYPE_UNREADABLE);
    fault->code = code;
    return fault;
}

static struct kibosh_faults *kibosh_faults_alloc(void *first, ...)
{
    int i, num_faults = 1;
    void *base;
    struct kibosh_faults *faults;
    va_list ap, ap2;

    va_start(ap, first);
    while (1) {
        base = va_arg(ap, void*);
        if (!base)
            break;
        num_faults++;
    }
    va_end(ap);

    faults = calloc(1, sizeof(*faults));
    if (!faults)
        return NULL;
    faults->list = calloc(num_faults + 1, sizeof(struct kibosh_fault_base*));
    if (!faults->list) {
        free(faults);
        return NULL;
    }

    faults->list[0] = first;;
    va_start(ap2, first);
    for (i = 1; i < num_faults; i++) {
        base = va_arg(ap2, void*);
        faults->list[i] = base;
    }
    va_end(ap2);
    return faults;
}

static int test_fault_unparse(void)
{
    struct kibosh_fault_unreadable *fault = kibosh_fault_unreadable_alloc(101);
    char *str;

    EXPECT_NONNULL(fault);
    EXPECT_INT_EQ(101, fault->code);
    str = kibosh_fault_base_unparse((struct kibosh_fault_base*)fault);
    EXPECT_STR_EQ("\{\"type\":\"unreadable\", \"code\":101}", str);
    free(str);
    kibosh_fault_base_free((struct kibosh_fault_base*)fault);
    return 0;
}

static int test_faults_unparse(void)
{
    struct kibosh_fault_unreadable *fault, *fault2;
    struct kibosh_faults *faults;
    char *str;

    fault = kibosh_fault_unreadable_alloc(101);
    EXPECT_NONNULL(fault);
    fault2 = kibosh_fault_unreadable_alloc(102);
    EXPECT_NONNULL(fault2);
    faults = kibosh_faults_alloc(fault, fault2, NULL);
    EXPECT_NONNULL(faults);

    str = faults_unparse(faults);
    EXPECT_STR_EQ("{\"faults\":["
                           "{\"type\":\"unreadable\", \"code\":101}, "
                           "{\"type\":\"unreadable\", \"code\":102}]}", str);
    free(str);
    faults_free(faults);
    return 0;
}

static int test_fault_parse(void)
{
    struct kibosh_fault_unreadable *unreadable;
    const char *str = "{\"faults\":["
                           "{\"type\":\"unreadable\", \"code\":1}, "
                           "{\"type\":\"unreadable\", \"code\":2}]}";
    struct kibosh_faults *faults = NULL;
    EXPECT_INT_ZERO(faults_parse(str, &faults));
    EXPECT_NONNULL(faults);
    EXPECT_STR_EQ(KIBOSH_FAULT_TYPE_UNREADABLE, faults->list[0]->type);
    unreadable = (struct kibosh_fault_unreadable*)faults->list[0];
    EXPECT_INT_EQ(1, unreadable->code);
    EXPECT_STR_EQ(KIBOSH_FAULT_TYPE_UNREADABLE, faults->list[1]->type);
    unreadable = (struct kibosh_fault_unreadable*)faults->list[1];
    EXPECT_INT_EQ(2, unreadable->code);
    faults_free(faults);
    return 0;
}

int main(void)
{
    EXPECT_INT_ZERO(test_fault_unparse());
    EXPECT_INT_ZERO(test_faults_unparse());
    EXPECT_INT_ZERO(test_fault_parse());

    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
