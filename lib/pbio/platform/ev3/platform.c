// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <stdio.h>

// Called from assembly code in startup.s. After this, the "main" function in
// lib/pbio/sys/main.c is called. That contains all calls to the driver
// initialization (low level in pbdrv, high level in pbio), and system level
// functions for running user code (currently a hardcoded MicroPython script).
void SystemInit(void) {
    printf("System init in platform.c called from startup.s\n\n");

    // TODO: TIAM1808 system init
}
