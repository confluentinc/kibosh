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

/**
 * Platform-specific configuration.
 */

#ifndef KIBOSH_CONFIG_H
#define KIBOSH_CONFIG_H

// This is defined if we can use the memfd_create system call.
#cmakedefine HAVE_MEMFD_CREATE

#endif

// vim: ts=4:sw=4:tw=99:et
