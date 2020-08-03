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
#include "file.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
#include "signal.h"
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <fuse.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <pthread.h>

static struct fuse_operations kibosh_oper;

// FUSE options which we always set.
static const char * const MANDATORY_FUSE_OPTIONS[] = {
    "-oallow_other", // Allow all users to access the mount point.
    "-odefault_permissions", // tell FUSE to do permission checking for us based on the reported permissions
    "-ohard_remove", // Do not translate unlink into renames to .fuse_hiddenXXX
    "-oatomic_o_trunc", // Pass O_TRUNC to open()
};

#define NUM_MANDATORY_FUSE_OPTIONS \
    (int)(sizeof(MANDATORY_FUSE_OPTIONS)/sizeof(MANDATORY_FUSE_OPTIONS[0]))

static void kibosh_usage(const char *argv0)
{
    fprintf(stderr,
"Kibosh: the fault-injecting filesystem.\n"
"\n"
"Kibosh is a FUSE daemon which allows you to inject arbitrary filesystem\n"
"errors into your applications for testing purposes.\n"
"\n"
"It exports a view of an existing directory.  By default this view is an\n"
"exact recreation of what is at the directory.  However, we can add\n"
"distortions to the view by enabling faults.  This allows injecting I/O\n"
"errors, slow behavior, and so forth, without modifying the underlying\n"
"target directory.\n"
"\n"
"usage:\n"
"    %s [options] <mirror>\n"
"\n"
"    The mirror directory is the mount point for the FUSE fs.\n"
"\n"
"options:\n"
"    -f                      Enable foreground operation rather than daemonizing.\n"
"    --log <path>            Write logs to the given path.\n"
"    --pidfile <path>        Write a process ID file to the given path.\n"
"                            This will be deleted if the process exits normally.\n"
"    --target <path>         The directory which we are mirroring (required)\n"
"    --control-mode <mode>   The octal mode to use on the root-owned control file.\n"
"                            Defaults to 0600.\n"
"    --random-seed <seed>    The seed for random generator.\n"
"                            Defaults to current time.\n"
"    -v/--verbose            Turn on verbose logging.\n\n"
"    -h/--help               This help text.\n\n"
"    --fuse-help             Get help about possible FUSE options.\n"
, argv0);
}

static int kibosh_process_option(void *data UNUSED, const char *arg UNUSED,
                                 int key, struct fuse_args *outargs)
{
    // Process options which are not automatically handled by fuse_opt_parse.
    switch (key) {
        case KIBOSH_CLI_GENERAL_HELP_KEY:
            kibosh_usage(outargs->argv[0]);
            exit(0);
            break;
        case KIBOSH_CLI_FUSE_HELP_KEY:
            fprintf(stderr, "Here are some FUSE options that can be supplied to Kibosh.\n"
                    "Note that not all options here are usable.\n\n");
            fuse_opt_add_arg(outargs, "-ho");
            fuse_main(outargs->argc, outargs->argv, &kibosh_oper, NULL);
            exit(0);
            break;
        default:
            break;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int i, ret = EXIT_FAILURE;
    struct kibosh_fs *fs = NULL;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct kibosh_conf *conf = NULL;
    FILE *log_file = NULL;
    char *conf_str = NULL;

    /*
     * If the stdout and stderr of Kibosh are being redirected to a file, they will be fully
     * buffered by default.  Here, we switch them to be line-buffered instead, so that we can see
     * any messages as they occur.  In general, we prefer to send messages to the configured log
     * file, but it is sometimes necessary for them to go to stdout or stderr.  One reason is that
     * FUSE writes some error messages to the standard streams.  Another reason is because if we
     * can't open our desired logfile, we need to log the failure message somewhere.
     */
    setvbuf(stdout, NULL, _IOLBF, 8192);
    setvbuf(stderr, NULL, _IOLBF, 8192);

    INFO("kibosh_main: starting Kibosh.\n");
    conf = kibosh_conf_alloc();
    if (!conf) {
        INFO("kibosh_main: OOM while allocating kibosh_conf.\n");
        goto done;
    }

    /*
     * We set our process umask to 0 so that we can create inodes with any
     * permissions we want.  However, we need to be careful to honor the umask
     * of the process we're performing our requests on behalf of.
     *
     * Note: The umask(2) call cannot fail.
     */
    umask(0);

    /*
     * Install some basic signal handlers.  We want fatal signals to be logged, so that we know
     * what happened, and SIGPIPE to be ignored.
     */
    if (install_signal_handlers()) {
        INFO("kibosh_main: failed to install signal handlers.\n");
        goto done;
    }

    if (fuse_opt_parse(&args, conf, kibosh_command_line_options, 
                        kibosh_process_option) < 0) {
        INFO("kibosh_main: fuse_opt_parse failed.\n");
        goto done;
    }

    if (kibosh_conf_reify(conf) < 0) {
        INFO("kibosh_main: kibosh_conf_reify failed.\n");
        goto done;
    }
    conf_str = kibosh_conf_to_str(conf);
    if (!conf_str) {
        INFO("kibosh_main: kibosh_conf_to_str got out of memory.\n");
        goto done;
    }
    INFO("kibosh_main: configured %s.\n", conf_str);

    /*
     * Change the current working directory to be the filesystem root.  We do this so that we don't
     * hold on to a reference to the current working directory while running our daemon.  All
     * configured relative paths should have been translated into absolute paths by
     * kibosh_conf_reify prior to this point.
     */
    if (chdir("/") < 0) {
        int err = errno;
        INFO("kibosh_main: failed to change directory to /: error %d (%s)\n",
             err, safe_strerror(err));
        goto done;
    }

    if (kibosh_fs_alloc(&fs, conf) < 0) {
        INFO("kibosh_main: error initializing FS\n");
        goto done;
    }

    for (i = 0; i < NUM_MANDATORY_FUSE_OPTIONS; i++) {
        if (fuse_opt_add_arg(&args, MANDATORY_FUSE_OPTIONS[i]) < 0) {
            INFO("kibosh_main: out of memory allocating fuse args.\n");
            goto done;
        }
    }

    if (conf->log_path) {
        log_file = fopen(conf->log_path, "w");
        if (!log_file) {
            int err = -errno;
            INFO("kibosh_main: failed to open log file %s: error %d  (%s)\n",
                 conf->log_path, -err, safe_strerror(-err));
            goto done;
        }
        setvbuf(log_file, NULL, _IOLBF, 8192);
    }

    kibosh_log_init((log_file ? log_file : stdout),
                    (conf->verbose ? KIBOSH_LOG_ALL_ENABLED : KIBOSH_LOG_INFO_ENABLED));

    if (conf->log_path) {
        /*
         * If a logfile is configured, repeat the earlier log message that describes our
         * configuration.
         */
        INFO("kibosh_main: configured %s.\n", conf_str);
    }

    /* Reset random seed for the process. */
    if (conf->random_seed) {
        srand(conf->random_seed);
        INFO("kibosh_main: random seed is set to %d.\n", conf->random_seed);
    } else {
        int s = (int) round(time(0));
        srand(s);
        INFO("kibosh_main: random seed is set to %d.\n", s);
    }

    /* Start a process to clear page cache every 5 seconds. */
    int sret = system("while true; do sleep .5; sudo sh -c \"echo 1 > /proc/sys/vm/drop_caches\"; done &");
    INFO("kibosh_main: started clear cache process. %d.\n", sret);

    /* Run main FUSE loop. */
    ret = fuse_main(args.argc, args.argv, &kibosh_oper, fs);

done:
    fuse_opt_free_args(&args);
    kibosh_conf_free(conf);
    free(conf_str);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    INFO("kibosh_main exiting with error code %d.\n", ret);
    return ret;
}

static void *kibosh_init(struct fuse_conn_info *conn)
{
    conn->want = FUSE_CAP_ASYNC_READ |
        FUSE_CAP_ATOMIC_O_TRUNC	|
        FUSE_CAP_BIG_WRITES	|
        FUSE_CAP_SPLICE_WRITE |
        FUSE_CAP_SPLICE_MOVE |
        FUSE_CAP_SPLICE_READ;
    return fuse_get_context()->private_data;
}

static void kibosh_destroy(void *fs)
{
    INFO("kibosh shut down gracefully.\n");

    int sret = system("sudo sh -c \"kill -9 $(ps aux | grep -i '/proc/sys/vm/drop_caches' | grep -Po '^.*?\\K[0-9]+' -m 1)\"");
    INFO("clear cache process is killed. %d.\n", sret);

    kibosh_fs_free(fs);
}

static struct fuse_operations kibosh_oper = {
    .getattr = kibosh_getattr,
    .readlink = kibosh_readlink,
    .getdir = NULL, // deprecated
    .mknod = kibosh_mknod,
    .mkdir = kibosh_mkdir,
    .unlink = kibosh_unlink,
    .rmdir = kibosh_rmdir,
    .symlink = kibosh_symlink,
    .rename = kibosh_rename,
    .link = kibosh_link,
    .chmod = kibosh_chmod,
    .chown = kibosh_chown,
    .truncate = kibosh_truncate,
    .utime = kibosh_utime,
    .open = kibosh_open,
    .read = kibosh_read,
    .write = kibosh_write,
    .statfs = kibosh_statfs,
    .flush = kibosh_flush,
    .release = kibosh_release,
    .fsync = kibosh_fsync,
    .setxattr = kibosh_setxattr,
    .getxattr = kibosh_getxattr,
    .listxattr = kibosh_listxattr,
    .removexattr = kibosh_removexattr,
    .opendir = kibosh_opendir,
    .readdir = kibosh_readdir,
    .releasedir = kibosh_releasedir,
    .fsyncdir = kibosh_fsyncdir,
    .init = kibosh_init,
    .destroy = kibosh_destroy,
    .access = NULL, // never called because we use 'default_permissions'
    .create = kibosh_create,
    .ftruncate = kibosh_ftruncate,
    .fgetattr = kibosh_fgetattr,
    .lock = NULL, // delegate to kernel
    .flock = NULL, // delegate to kernel
	.utimens = kibosh_utimens,
    .bmap = NULL, // We are not a block-device-backed filesystem
	.fallocate = kibosh_fallocate,

    // Allow various operations to continue to work on unlinked files.
    .flag_nullpath_ok = 1,

    // Don't bother with paths for operations where we're using fds.
    .flag_nopath = 1,

    // Allow UTIME_OMIT and UTIME_NOW to be used with kibosh_utimens.
    .flag_utime_omit_ok = 1,
};

// vim: ts=4:sw=4:tw=99:et
