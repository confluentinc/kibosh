/**
 * Copyright 2020 Confluent Inc.
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

#ifndef KIBOSH_DROP_CACHE_H
#define KIBOSH_DROP_CACHE_H

#define DROP_CACHES_PATH "/proc/sys/vm/drop_caches"

/**
 * Drop the cache by writing a 1 to the given path.
 *
 * @param path    The path to write a 1 to.
 * @return        0 on sucess; the negative error code otherwise.
 */
int drop_cache(const char *path);

/**
 * Create and start the drop_cache thread.
 *
 * @param path    The path to use.
 * @param period  The number of seconds between cache drops.
 *
 * @return        The thread on success; NULL otherwise.
 */
struct drop_cache_thread *drop_cache_thread_start(const char *path, int period);

/**
 * Stop and join the drop_cache thread.
 *
 * @param thread  The path to join.
 */
void drop_cache_thread_join(struct drop_cache_thread *thread);

#endif

// vim: ts=4:sw=4:tw=99:et
