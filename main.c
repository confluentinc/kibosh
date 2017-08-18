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

#include "file.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
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

static struct fuse_operations kibosh_oper;

// FUSE options which we always set.
static const char const *MANDATORY_OPTIONS[] = {
    "-oallow_other", // Allow all users to access the mount point.
    "-odefault_permissions", // tell FUSE to do permission checking for us based on the reported permissions
    "-odirect_io", // Don't cache data in FUSE.
    "-ohard_remove", // Do not translate unlink into renames to .fuse_hiddenXXX
    "-oatomic_o_trunc", // Pass O_TRUNC to open()
};

#define NUM_MANDATORY_OPTIONS \
    (int)(sizeof(MANDATORY_OPTIONS)/sizeof(MANDATORY_OPTIONS[0]))

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
    kibosh_fs_free(fs);
}

static void kibosh_usage(const char *argv0)
{
    fprintf(stderr, "\
usage:  %s [FUSE and mount options] <root> <mount_point>\n", argv0);
}

/**
 * Set up the fuse arguments we want to pass.
 *
 * We start with something like this:
 *     ./myapp [fuse-options] <overfs> <underfs>
 *
 * and end up with this:
 *     ./myapp [mandatory-fuse-options] [fuse-options] <overfs>
 */
static int setup_kibosh_args(int argc, char **argv, struct fuse_args *args,
                          char **mount_point)
{
    int i, ret = 0;

    *mount_point = NULL;
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
        kibosh_usage(argv[0]);
        ret = -EINVAL;
        goto error;
    }
    *mount_point = strdup(strdup(argv[argc - 1]));
    if (!*mount_point) {
        INFO("setup_kibosh_args: OOM\n");
        ret = -ENOMEM;
        goto error;
    }
    args->argc = NUM_MANDATORY_OPTIONS + argc - 2;
    args->argv = calloc(sizeof(char*), args->argc + 1);
    if (!args->argv) {
        INFO("setup_kibosh_args: OOM\n");
        ret = -ENOMEM;
        goto error;
    }
    args->argv[0] = argv[0];
    for (i = 0; i < NUM_MANDATORY_OPTIONS; i++) {
        args->argv[i + 1] = (char*)MANDATORY_OPTIONS[i];
    }
    for (i = 1; i < argc - 2; i++) {
        args->argv[NUM_MANDATORY_OPTIONS - 1 + i] = argv[i];
    }
    args->argv[args->argc - 1] = argv[argc - 2];
    args->argv[args->argc] = NULL;
    INFO("running fuse_main with: ");
    for (i = 0; i < args->argc; i++) {
        INFO("%s ", args->argv[i]);
    }
    INFO("\n ");
    return 0;

error:
    if (*mount_point) {
        free(*mount_point);
        *mount_point = NULL;
    }
    if (args->argv) {
        free(args->argv);
        args->argv = NULL;
    }
    args->argc = 0;
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct kibosh_fs *fs = NULL;
    struct fuse_args args;
    char *root = NULL;

    memset(&args, 0, sizeof(args));

    if (chdir("/") < 0) {
        perror("kibosh_main: failed to change directory to /");
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

    /* Ignore SIGPIPE because it is annoying. */
    if (signal(SIGPIPE, SIG_DFL) == SIG_ERR) {
        INFO("kibosh_main: failed to set the disposition of EPIPE "
                "to SIG_DFL (ignored.)\n"); 
        goto done;
    }

    /* Set up mandatory arguments. */
    if (setup_kibosh_args(argc, argv, &args, &root) < 0) {
        goto done;
    }

    if (kibosh_fs_alloc(&fs, root) < 0) {
        INFO("kibosh_main: error initializing FS\n");
        goto done;
    }

    /* Run main FUSE loop. */
    ret = fuse_main(args.argc, args.argv, &kibosh_oper, fs);

done:
    free(args.argv);
    free(root);
    INFO("kibosh_main exiting with error code %d\n", ret);
    return ret;
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
