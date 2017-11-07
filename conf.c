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
#include "util.h"

#include <errno.h>
#include <fuse.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KIBOSH_CONF_OPT(t, p, v) { t, offsetof(struct kibosh_conf, p), v }

struct fuse_opt kibosh_command_line_options[] = {
     KIBOSH_CONF_OPT("--pidfile=%s", pidfile_path, 0),
     KIBOSH_CONF_OPT("--log=%s", log_path, 0),
     KIBOSH_CONF_OPT("--target=%s", target_path, 0),
     KIBOSH_CONF_OPT("-v", verbose, 0),
     KIBOSH_CONF_OPT("--verbose", verbose, 0),
     FUSE_OPT_KEY("-h", KIBOSH_CLI_GENERAL_HELP_KEY),
     FUSE_OPT_KEY("--help", KIBOSH_CLI_GENERAL_HELP_KEY),
     FUSE_OPT_KEY("--fuse-help", KIBOSH_CLI_FUSE_HELP_KEY),
     FUSE_OPT_END
};

struct kibosh_conf *kibosh_conf_alloc(void)
{
    struct kibosh_conf *conf;
    return calloc(1, sizeof(*conf));
}

void kibosh_conf_free(struct kibosh_conf *conf)
{
    if (conf) {
        free(conf->pidfile_path);
        free(conf->log_path);
        free(conf->target_path);
        free(conf);
    }
}

static int absolutize(const char *path, char **out)
{
    char *result = NULL, *cwd = NULL;

    if (!path) {
        // The path is NULL.  Do not try to absolutize it.
        *out = NULL;
        return 0;
    }
    if (path[0] == '/') {
        // The path is already absolute.  Copy it and return.
        result = strdup(path);
        if (!result) {
            INFO("absolutize: strdup failed: OOM.\n");
            return -ENOMEM;
        }
        *out = result;
        return 0;
    }
    // Make the path absolute by prepending the current working directory.
    // Note that this does not necessarily make the path canonical, just absolute.
    cwd = get_current_dir_name();
    if (!cwd) {
        int err = -errno;
        INFO("absolutize: get_current_dir_name failed with error %d (%s)\n",
             -err, safe_strerror(-err));
        return err;
    }
    result = dynprintf("%s/%s", cwd, path);
    free(cwd);
    if (!result) {
        INFO("absolutize: strdup failed: OOM.\n");
        return -ENOMEM;
    }
    *out = result;
    return 0;
}

int kibosh_conf_reify(struct kibosh_conf *conf)
{
    int ret = absolutize(conf->pidfile_path, &conf->pidfile_path);
    if (ret < 0) {
        INFO("Unable to find realpath of pidfile: error %d (%s)\n",
             -ret, safe_strerror(-ret));
        return ret;
    }
    ret = absolutize(conf->log_path, &conf->log_path);
    if (ret < 0) {
        INFO("Unable to find realpath of log file: error %d (%s)\n",
             -ret, safe_strerror(-ret));
        return ret;
    }
    ret = absolutize(conf->target_path, &conf->target_path);
    if (ret < 0) {
        INFO("Unable to find realpath of target path: error %d (%s)\n",
             -ret, safe_strerror(-ret));
        return ret;
    }
    if (!conf->target_path) {
        INFO("You must supply a target path.  Type --help for help.\n");
        return -1;
    }
    return 0;
}

// vim: ts=4:sw=4:tw=99:et
