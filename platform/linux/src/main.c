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
    "usage: %s [options]\n"
    "options:\n"
    "  -d, --dir    set the working directory\n"
    "  -e, --entry  set the entry script name\n"
    "  -h, --help   display this help and exit\n";

static const char *progname = "homekit-bridge";
static const char *workdir = BRIDGE_WORK_DIR;
static const char *entry = BRIDGE_LUA_ENTRY_DEFAULT;

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

static void doargs(int argc, char *argv[]) {
    if (argv[0] && *argv[0] != 0) {
        progname = argv[0];
    }
    for (int i = 1; i < argc; i++) {
        if (HAPStringAreEqual(argv[i], "-h") || HAPStringAreEqual(argv[i], "--help")) {
            usage(NULL);
            exit(EXIT_SUCCESS);
        } else if (HAPStringAreEqual(argv[i], "-d") || HAPStringAreEqual(argv[i], "--dir")) {
            workdir = argv[++i];
            if (!workdir || *workdir == 0 || (*workdir == '-' && workdir[1] != 0)) {
                usage("'-d' needs argument");
                exit(EXIT_FAILURE);
            }
        } else if (HAPStringAreEqual(argv[i], "-e") || HAPStringAreEqual(argv[i], "--entry")) {
            entry = argv[++i];
            if (!entry || *entry == 0 || (*entry == '-' && entry[1] != 0)) {
                usage("'-e' needs argument");
                exit(EXIT_FAILURE);
            }
        } else {
            usage(argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}

void sigint(int signum) {
    app_exit();
}

int main(int argc, char *argv[]) {
    // Parse arguments.
    doargs(argc, argv);

    // Initialize pal modules.
    HAPPlatformRunLoopCreate();
    pal_ssl_init();
    pal_dns_init();
    pal_nvs_init(".nvs");
    pal_net_if_init();

    // Initialize application.
    app_init(workdir, entry);

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
