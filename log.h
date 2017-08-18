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

#ifndef KIBOSH_LOG_H
#define KIBOSH_LOG_H

#include <stdio.h> // for printf

#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
/**
 * Print a debug message.
 */
#define DEBUG_HELPER(fmt, ...) printf(fmt "%s", __VA_ARGS__)
#define DEBUG(...) DEBUG_HELPER(__VA_ARGS__, "")
#else 
#define DEBUG(x, ...) ; 
#endif 

/**
 * Print an info message.
 */
#define INFO_HELPER(fmt, ...) printf(fmt "%s", __VA_ARGS__)
#define INFO(...) INFO_HELPER(__VA_ARGS__, "")

/**
 * A thread-safe strerror alternative.
 *
 * @param errnum        The POSIX error number.  Negative numbers are treated as their positive 
 *                      equivalents.
 *
 * @return              A thread-local error string.  It will remain valid
 *                      until the next call to safe_strerror.
 */
const char *safe_strerror(int err);

#endif

// vim: ts=4:sw=4:et
