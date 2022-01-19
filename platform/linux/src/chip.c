// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/chip.h>

const char *pal_chip_get_manufacturer(void) {
    return "Unknown";
}

const char *pal_chip_get_model(void) {
    return "Unknown";
}

const char *pal_chip_get_serial_number(void) {
    return "Unknown";
}

const char *pal_chip_get_hardware_version(void) {
    return "Unknown";
}
