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

#include "signal.h"
#include "log.h"
#include "util.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static int KIBOSH_HANDLED_SIGNALS[] = {
    SIGABRT,
    SIGBUS,
    SIGFPE,
    SIGILL,
    SIGINT,
    SIGQUIT,
    SIGSEGV,
    SIGTERM
};

#define NUM_KIBOSH_HANDLED_SIGNALS (sizeof(KIBOSH_HANDLED_SIGNALS) / sizeof(int))

static void kibosh_signal_handler(int signum)
{
    emit_shutdown_message(signum);
    _exit(128 + signum);
}

int install_signal_handlers(void)
{
    size_t i;
    struct sigaction sa;
    char fatal_str[4096];
    const char *prefix = "";

    for (i = 0; i < NUM_KIBOSH_HANDLED_SIGNALS; i++) {
        int signum = KIBOSH_HANDLED_SIGNALS[i];
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = kibosh_signal_handler;
        sigemptyset(&(sa.sa_mask));
        sa.sa_flags = 0;
        if (sigaction(signum, &sa, NULL) == -1) {
            int err = -errno;
            INFO("install_signal_handlers: failed to install handler for "
                "signal %s: %d (%s)\n", strsignal(signum), -err, safe_strerror(-err));
            return err;
        }
    }
    fatal_str[0] = '\0';
    for (i = 0; i < NUM_KIBOSH_HANDLED_SIGNALS; i++) {
        snappend(fatal_str, sizeof(fatal_str), "%s%s", prefix, 
                 strsignal(KIBOSH_HANDLED_SIGNALS[i]));
        prefix = ", ";
    }

    /* Ignore SIGPIPE because it is annoying. */
    if (signal(SIGPIPE, SIG_DFL) == SIG_ERR) {
        int err = -errno;
        INFO("kibosh_main: failed to set the disposition of EPIPE "
                "to SIG_DFL (ignored): error %d (%s)\n",
                -err, safe_strerror(-err));
        return err;
    }
    INFO("install_signal_handlers: ignoring SIGPIPE and handling fatal signals: %s\n",
         fatal_str);
    return 0;
}

// vim: ts=4:sw=4:tw=99:et
