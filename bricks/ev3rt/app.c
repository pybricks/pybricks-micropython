// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include "ev3api.h"
#include "app.h"

#include <pbsys/main.h>

// For now, this file is the main entry point from EV3RT. Eventually, this
// file can be dropped and main_task() can be mapped to main() in pbsys/main.
// For now it enters the MicroPython REPL directly for convenient debugging.

static char heap[1024 * 256];

void main_task(intptr_t unused) {

    // Show the Pybricks logo on the screen so we know that something is
    // running. This should be replaced by an appropriate driver in pbio and
    // called from the system hmi interface in pbsys.
    memfile_t memfile;
    image_t image;
    if (ev3_memfile_load("/pybricks.bmp", &memfile) == E_OK &&
        ev3_image_load(&memfile, &image) == E_OK) {
        ev3_lcd_draw_image(&image, 0, 0);
    }

    while (true) {
        pbsys_main_program_t program = {
            .run_builtin = true,
            .code_end = heap,
            .data_end = heap + sizeof(heap),
        };
        pbsys_main_run_program(&program);
    }
}
