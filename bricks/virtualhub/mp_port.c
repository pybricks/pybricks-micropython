// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <pbio/main.h>

// MICROPY_PORT_INIT_FUNC
void pb_virtualhub_port_init(void) {
    pbio_init();
}
