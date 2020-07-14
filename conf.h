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

#ifndef KIBOSH_CONF_H
#define KIBOSH_CONF_H

#include <fuse.h> // for fuse_opt

struct kibosh_conf {
    /**
     * The path we should write our pidfile to, or NULL if pid files are not enabled.  Malloced.
     */
    char *pidfile_path;

    /**
     * The path we should log message to, or NULL to use stdout.  Note that stdout will not be
     * visible unless the process is running in foreground mode.  Malloced.
     */
    char *log_path;

    /**
     * An existing path which contains files we want to mirror.  This argument is required.  Malloced.
     */
    char *target_path;

    /**
     * Zero to suppress DEBUG logs, nonzero to enable them.
     */
    int verbose;

    /**
     * Mode to use on the control file.
     */
    int control_mode;

    /**
     * Seed for random functions.
     */
    int random_seed;
};

enum kibosh_option_ty {
    KIBOSH_CLI_GENERAL_HELP_KEY,
    KIBOSH_CLI_FUSE_HELP_KEY
};

/**
 * The command-line options accepted by Kibosh.
 *
 * These are in addition to the standard FUSE command-line options.
 */
extern struct fuse_opt kibosh_command_line_options[];

/**
 * Allocate a new kibosh_conf object.
 *
 * @return              The configuration, or NULL on OOM.
 */
struct kibosh_conf *kibosh_conf_alloc(void);

/**
 * Free a kibosh_conf object.
 *
 * @param conf          The configuration to free.
 */
void kibosh_conf_free(struct kibosh_conf *conf);

/**
 * Process a kibosh_conf object to make it suitable for use.
 *
 * Resolves relative paths into absolute paths.
 * Verifies that required fields are present.
 *
 * @param conf          The configuration.  Will be modified.
 *
 * @return              0 on success; a negative error code on error.
 */
int kibosh_conf_reify(struct kibosh_conf *conf);

/**
 * Returns a string representation of this configuration.
 *
 * @param conf          The configuration.
 *
 * @return              A malloced string, or NULL on OOM.
 */
char *kibosh_conf_to_str(const struct kibosh_conf *conf);

#endif

// vim: ts=4:sw=4:tw=99:et
