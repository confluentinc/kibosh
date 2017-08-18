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

// Get the POSIX versions of strerror_r
#define _POSIX_C_SOURCE 200112L
#undef _GNU_SOURCE

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRERROR_BUFFER_SIZE 128

__thread char strerror_buffer[STRERROR_BUFFER_SIZE];

const char* safe_strerror(int errnum)
{
    if (errnum < 0) {
        errnum = -errnum;
    }
    if (strerror_r(errnum, strerror_buffer, STRERROR_BUFFER_SIZE)) {
        snprintf(strerror_buffer, STRERROR_BUFFER_SIZE, "Unknown error %d", errnum);
    }
    return strerror_buffer;
}

// vim: ts=4:sw=4:tw=99:et
