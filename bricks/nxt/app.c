// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbsys/main.h>

// For now, this file is the main entry point for NXT. Eventually, this
// file can be dropped and main_task() can be mapped to main() in pbsys/main.
// For now it enters the MicroPython REPL directly for convenient debugging.

static char heap[32 * 1024];

void main_task(void) {

    while (true) {
        pbsys_main_program_t program = {
            .run_builtin = true,
            .code_end = heap,
            .data_end = heap + sizeof(heap),
        };
        pbsys_main_run_program(&program);
    }
}
