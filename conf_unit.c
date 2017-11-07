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

#include "conf.h"
#include "log.h"
#include "test.h"
#include "util.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_alloc_free_kibosh_conf(void)
{
    struct kibosh_conf *conf = kibosh_conf_alloc();
    EXPECT_NONNULL(conf);
    kibosh_conf_free(conf);
    return 0;
}

static int test_kibosh_conf_reify(const char *cwd)
{
    struct kibosh_conf *conf;
    char *expected_target = NULL, *expected_pidfile = NULL, *expected_log = NULL;
    
    conf = kibosh_conf_alloc();
    EXPECT_NONNULL(conf);

    // Since the target_path is not set, we should get EINVAL. 
    EXPECT_INT_EQ(-EINVAL, kibosh_conf_reify(conf));

    // Once the target path is set, this should succeed.
    conf->target_path = strdup("/foo");
    EXPECT_NONNULL(conf->target_path);
    EXPECT_INT_EQ(0, kibosh_conf_reify(conf));
    EXPECT_STR_EQ("/foo", conf->target_path);

    // Test absolutizing logic
    free(conf->target_path);
    conf->target_path = strdup("foo");
    EXPECT_NONNULL(conf->target_path);
    conf->pidfile_path = strdup("bar");
    EXPECT_NONNULL(conf->pidfile_path);
    conf->log_path = strdup(".");
    EXPECT_NONNULL(conf->log_path);
    expected_target = dynprintf("%s/%s", cwd, conf->target_path);
    EXPECT_NONNULL(expected_target);
    expected_pidfile = dynprintf("%s/%s", cwd, conf->pidfile_path);
    EXPECT_NONNULL(expected_pidfile);
    expected_log = dynprintf("%s/%s", cwd, conf->log_path);
    EXPECT_NONNULL(expected_log);
    EXPECT_INT_EQ(0, kibosh_conf_reify(conf));
    EXPECT_STR_EQ(expected_target, conf->target_path);
    EXPECT_STR_EQ(expected_pidfile, conf->pidfile_path);
    EXPECT_STR_EQ(expected_log, conf->log_path);
    free(expected_target);
    free(expected_pidfile);
    free(expected_log);

    kibosh_conf_free(conf);
    return 0;
}

int main(void)
{
    char *cwd = get_current_dir_name();
    EXPECT_NONNULL(cwd);
    kibosh_log_init(stdout, 0);
    EXPECT_INT_ZERO(test_alloc_free_kibosh_conf());
    EXPECT_INT_ZERO(test_kibosh_conf_reify(cwd));
    free(cwd);
    return EXIT_SUCCESS;
}

// vim: ts=4:sw=4:tw=99:et
