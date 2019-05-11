// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbsys/sys.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "lib/utils/interrupt_char.h"

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[8 * 1024];
#endif

#if MICROPY_PERSISTENT_CODE_LOAD
static void run_user_program() {
    nlr_buf_t nlr;

    if (nlr_push(&nlr) == 0) {
        mp_call_function_0(mp_import_name(QSTR_FROM_STR_STATIC(PYBRICKS_MPY_MAIN_MODULE),
            mp_const_none, MP_OBJ_NEW_SMALL_INT(0)));
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

// _binary_build_main_mpy_start is defined by objcopy during make
extern uint32_t _binary_build_main_mpy_start;
#define main_mpy ((const uint8_t *)_binary_build_main_mpy_start)

static uint32_t main_mpy_pos;

static mp_uint_t main_mpy_readbyte(void *data) {
    // TODO: do we need to handle end of file?
    return main_mpy[main_mpy_pos];
}

static void main_mpy_close(void *data) {
    main_mpy_pos = 0;
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    reader->data = NULL;
    reader->readbyte = main_mpy_readbyte;
    reader->close = main_mpy_close;
}
#endif // MICROPY_PERSISTENT_CODE_LOAD

#if !MICROPY_ENABLE_COMPILER
typedef enum {
    WAITING_FOR_FIRST_RELEASE,
    WAITING_FOR_PRESS,
    WAITING_FOR_SECOND_RELEASE
} waiting_for_t;

// wait for button to be pressed/released before starting program
static void wait_for_button_press(void) {
    pbio_button_flags_t btn;
    waiting_for_t wait_for = WAITING_FOR_FIRST_RELEASE;

    mp_print_str(&mp_plat_print, "\nPress green button to start...");

    // wait for button rising edge, then falling edge
    for (;;) {
        pbio_button_is_pressed(PBIO_PORT_SELF, &btn);
        if (btn & PBIO_BUTTON_CENTER) {
            // step 2:
            // once we are sure the button is released, we wait for it to be
            // pressed (rising edge).
            if (wait_for == WAITING_FOR_PRESS) {
                wait_for = WAITING_FOR_SECOND_RELEASE;
            }
        }
        else {
            // step 1:
            // we have to make sure the button is released before waiting for
            // it to be pressed, otherwise programs would be restarted as soon
            // as they are stopped because the button is already pressed.
            if (wait_for == WAITING_FOR_FIRST_RELEASE) {
                wait_for = WAITING_FOR_PRESS;
            }
            // step 3:
            // after the button has been pressed, we need to wait for it to be
            // released (falling edge), otherwise programs would stop as soon
            // as they were started because the button is already pressed.
            else if (wait_for == WAITING_FOR_SECOND_RELEASE) {
                break;
            }
        }
        while (pbio_do_one_event()) { }
        __WFI();
    }

    // add some space so user knows where their output starts
    mp_print_str(&mp_plat_print, "\n\n");
}
#endif // MICROPY_ENABLE_COMPILER

// callback for when stop button is pressed
static void user_program_stop_func(void) {
    // we can only raise an exception if the VM is running
    // mp_interrupt_char will be either -1 or 0 when VM is not running
    if (mp_interrupt_char > 0) {
        mp_keyboard_interrupt();
    }
}

static bool user_program_stdin_event_func(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_keyboard_interrupt();
        return true;
    }

    return false;
}

static const pbsys_user_program_callbacks_t user_program_callbacks = {
    .stop           = user_program_stop_func,
    .stdin_event    = user_program_stdin_event_func,
};

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    pbio_init();

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

soft_reset:
    #if !MICROPY_ENABLE_COMPILER
    wait_for_button_press();
    #endif

    pbsys_prepare_user_program(&user_program_callbacks);

    mp_init();

    pyexec_frozen_module("boot.py");

    #if MICROPY_ENABLE_COMPILER
    pyexec_friendly_repl();
    #else // MICROPY_ENABLE_COMPILER
    #if MICROPY_PERSISTENT_CODE_LOAD
    run_user_program();
    #else // MICROPY_PERSISTENT_CODE_LOAD
    pyexec_frozen_module("main.py");
    #endif // MICROPY_PERSISTENT_CODE_LOAD
    #endif // MICROPY_ENABLE_COMPILER
    mp_deinit();

    pbsys_unprepare_user_program();

    goto soft_reset;

    pbio_deinit();

    return 0;
}

// defined in linker script
extern uint32_t _estack;
// defined in ports/stm32/gchelper_m0.s
uintptr_t gc_helper_get_regs_and_sp(uintptr_t *regs);

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void**)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();

    // for debug during development
    gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
#if MICROPY_PERSISTENT_CODE_LOAD
    if (strcmp(path, PYBRICKS_MPY_MAIN_MODULE ".mpy") == 0) {
        return MP_IMPORT_STAT_FILE;
    }
#endif
    return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
