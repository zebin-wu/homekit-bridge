// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.
//
// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <app.h>
#include <pal/ssl.h>
#include <pal/ssl_int.h>
#include <pal/dns.h>
#include <pal/nvs_int.h>
#include <pal/net_if_int.h>

#include <HAPPlatformRunLoop+Init.h>

#ifndef BRIDGE_WORK_DIR
#error Please set the macro "BRIDGE_WORK_DIR"
#endif

static const char *help = \
    "usage: %s [options] [script [args]]\n"
    "options:\n"
    "  -d, --dir    set the working directory\n"
    "  -h, --help   display this help and exit\n";

static const char *progname = "homekit-bridge";
static const char *workdir = BRIDGE_WORK_DIR;

static void usage(const char* message) {
    if (message) {
        if (*message == '-') {
            fprintf(stderr, "%s: unrecognized option '%s'\n", progname, message);
        } else {
            fprintf(stderr, "%s: %s\n", progname, message);
        }
    }
    fprintf(stderr, help, progname);
}

static int doargs(int argc, char *argv[]) {
    if (argv[0] && *argv[0] != 0) {
        progname = argv[0];
    }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(NULL);
            exit(EXIT_SUCCESS);
        } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir")) {
            workdir = argv[++i];
            if (!workdir || *workdir == 0 || (*workdir == '-' && workdir[1] != 0)) {
                usage("'-d' needs argument");
                exit(EXIT_FAILURE);
            }
        } else if (argv[i][0] == '-') {
            usage(argv[i]);
            exit(EXIT_FAILURE);
        } else {
            return i;
        }
    }

    return argc;
}

static void sigint(int signum) {
    app_exit();
}

static void app_default_returned(pal_err err, void *arg) {
    if (err == PAL_ERR_UNKNOWN) {
        app_exit();
    }
}

static void app_returned(pal_err err, void *arg) {
    app_exit();
}

int main(int argc, char *argv[]) {
    // Parse arguments.
    int parsed = doargs(argc, argv);

    // Initialize pal modules.
    HAPPlatformRunLoopCreate();
    pal_ssl_init();
    pal_dns_init();
    pal_nvs_init(".nvs");
    pal_net_if_init();

    // Initialize application.
    app_init(workdir);

    // Execute command.
    if (argc == parsed) {
        app_exec(APP_EXEC_DEFAULT_CMD, APP_EXEC_DEFAULT_ARGC, APP_EXEC_DEFAULT_ARGV, app_default_returned, NULL);
    } else {
        app_exec(argv[1], argc - parsed - 1, (const char **)argv + parsed + 1, app_returned, NULL);
    }

    // Use 'ctrl + C' to exit the application.
    signal(SIGINT, sigint);

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // De-initialize application.
    app_deinit();

    // De-initialize pal modules.
    pal_net_if_deinit();
    pal_nvs_deinit();
    pal_dns_deinit();
    pal_ssl_deinit();
    HAPPlatformRunLoopRelease();

    return 0;
}
