// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/light.h>
#include <pbsys/sys.h>

#include "pbobj.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/persistentcode.h"
#include "lib/utils/pyexec.h"
#include "lib/utils/interrupt_char.h"

#include "py/mphal.h"

// FIXME: Decide whether or not to pre-allocate
// memory for logging instead or just disable
// logging altogether on some constrained ports.
void *malloc(size_t n) {
    return m_malloc(n);
}
void free(void *p) {
    m_free(p);
}

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[PYBRICKS_HEAP_KB * 1024];
#endif


#define MPY_MAX_BYTES (1024)

typedef enum {
    WAITING_FOR_FIRST_RELEASE,
    WAITING_FOR_PRESS,
    WAITING_FOR_SECOND_RELEASE
} waiting_for_t;

static bool timed_out(uint32_t time_start, uint32_t time_out) {
    // Return true if more than time_out ms have elapsed
    // since time_start, otherwise return false.
    return clock_usecs()/1000 - time_start > time_out;
}

// wait for button to be pressed/released before starting program
static bool wait_for_button_press(uint32_t time_out) {
    pbio_button_flags_t btn;
    waiting_for_t wait_for = WAITING_FOR_FIRST_RELEASE;
    uint32_t time_start = clock_usecs()/1000;

    // wait for button rising edge, then falling edge
    while (!timed_out(time_start, time_out)) {
        pbio_button_is_pressed(&btn);
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
                if (time_out == 0) {
                    return true;
                }
                else {
                    return !timed_out(time_start, time_out);
                }
            }
        }
        while (pbio_do_one_event()) { }
        __WFI();
    }
    return false;
}

static uint32_t get_user_program(uint8_t **buf) {

    #ifdef PYBRICKS_MPY_MAIN_MODULE
    mp_print_str(&mp_plat_print, "\nLoading built-in user program from flash.\n");
    // TODO: set buf to address in flash and return len stored in flash
    return 0;
    #endif

    // Empty rx buffer
    uint8_t c;
    while (pbsys_stdin_get_char(&c) != PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }

    mp_print_str(&mp_plat_print, "\nReady to receive program.\n");

    // Get the length of the mpy file
    uint32_t len = 0;
    for (uint8_t i = 0; i < 4; i++) {
        c = mp_hal_stdin_rx_chr();
        len |= c << (3-i)*8;
    }

    // Assert that the length is allowed
    if (len > MPY_MAX_BYTES) {
        return 0;
    }

    // Allocate buffer for MPY file with known length
    uint8_t *mpy = m_malloc(len);
    if (mpy == NULL) {
        return 0;
    }

    // We are ready for the main program
    mp_print_str(&mp_plat_print, "\nReceiving program...\n");

    // Receive program over Bluetooth
    for (uint32_t i = 0; i < len; i++) {
        mpy[i] = mp_hal_stdin_rx_chr();
    }

    // TODO: On error/timeout, free buf and return 0

    *buf = mpy;
    return len;
}

static void run_user_program(uint32_t len, uint8_t *buf) {
    mp_print_str(&mp_plat_print, "Starting user program now.\n");

    #ifdef PYBRICKS_MPY_MAIN_MODULE
    uint32_t free_len = 0;
    #else
    uint32_t free_len = len;
    #endif

    mp_reader_t reader;
    mp_reader_new_mem(&reader, buf, len, free_len);

    // Convert buf to raw code and do m_free(buf) in the process
    mp_raw_code_t *raw_code = mp_raw_code_load(&reader);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t module_fun = mp_make_function_from_raw_code(raw_code, MP_OBJ_NULL, MP_OBJ_NULL);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // nlr_jump(nlr.ret_val);
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }

    mp_print_str(&mp_plat_print, "Done running user program.\n");
}

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

static void pb_imports() {
    #if PYBRICKS_PY_ADVANCED
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_advanced);
    #endif
    #if PYBRICKS_PY_CITYHUB
    PB_IMPORT_MODULE(MP_QSTR_cityhub);
    #endif
    #if PYBRICKS_PY_PUPDEVICES
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_devices);
    #endif
    #if PYBRICKS_PY_DEBUG
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_debug);
    #endif
    #if PYBRICKS_PY_MOVEHUB
    PB_IMPORT_MODULE(MP_QSTR_movehub);
    #endif
    #if PYBRICKS_PY_PARAMETERS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_parameters);
    #endif
    #if PYBRICKS_PY_TOOLS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_tools);
    #endif
    #if PYBRICKS_PY_ROBOTICS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_robotics);
    #endif
}

static void run_repl_program() {
#if MICROPY_ENABLE_COMPILER
    mp_print_str(&mp_plat_print, "Entering REPL. CTRL+D to exit.\n");
    pyexec_friendly_repl();
#else
    mp_print_str(&mp_plat_print, "REPL unavailable.\n");
#endif // MICROPY_ENABLE_COMPILER
}

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    pbio_init();

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

    bool pressed_during_boot;

soft_reset:
    // For debuggging purposes, we enter the REPL if the button is clicked
    // within 0.5 seconds after reset/boot. Later, we can enable activation of
    // the REPL through an IDE and avoid this artificial boot time here.
    // In other words, for now:
    // - Single click to boot Download & Run mode
    // - Double click to boot REPL
    _pbio_light_set_user_mode(1);
    pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_RED);
    mp_print_str(&mp_plat_print, "\n\n----------------\n"
                                     "Booting Pybricks\n"
                                     "----------------\n");
    pressed_during_boot = wait_for_button_press(500);
    pbsys_prepare_user_program(&user_program_callbacks);

    mp_init();

    // Import standard Pybricks modules
    pb_imports();

    // Enter the REPL if button was pressed while red light was on
    if (pressed_during_boot) {
        run_repl_program();
    }
    // Otherwise load and run a pre-compiled MPY program
    else {
        uint8_t *program;
        // FIXME: Correctly handle 0x00
        pbsys_unprepare_user_program();
        uint32_t len = get_user_program(&program);
        pbsys_prepare_user_program(&user_program_callbacks);
        if (len > 0) {
            run_user_program(len, program);
        }
        else {
            mp_print_str(&mp_plat_print, "No valid program received. Reboot.\n");
        }
    }

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

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
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
