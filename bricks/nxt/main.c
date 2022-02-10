// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/main.h>
#include <pbio/servo.h>
#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/light.h>
#include <pbsys/user_program.h>

#include <pybricks/util_mp/pb_obj_helper.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/persistentcode.h"
#include "py/stackctrl.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/interrupt_char.h"

#include "py/mphal.h"

#include <nxt/display.h>

#define _INIT_H_
#include "interrupts.h"
#include "aic.h"
#include "at91sam7.h"
#include "uart.h"
#include "systick.h"
#include "stdio.h"
#include "flashprog.h"
#include "nxt_avr.h"
#include "twi.h"
#include "sensors.h"

#include "nxt_avr.h"
#include "nxt_lcd.h"
#include "i2c.h"
#include "nxt_motors.h"

#include "lejos_nxt.h"

#include "display.h"
#include "sound.h"
#include "bt.h"
#include "udp.h"
#include "flashprog.h"
#include "hs.h"

#include <string.h>

void shutdown(int update_mode) {
    nxt_lcd_enable(0);
    for (;;) {
        if (update_mode) {
            nxt_avr_firmware_update_mode();
        } else {
            nxt_avr_power_down();
        }
    }
}

void nxt_init() {
    aic_initialise();
    sp_init();
    interrupts_enable();
    systick_init();
    sound_init();
    nxt_avr_init();
    nxt_motor_init();
    i2c_init();
    bt_init();
    hs_init();
    udp_init();
    systick_wait_ms(100);
    sound_freq(500, 100, 30);
    systick_wait_ms(100);
    display_init();
    sp_init();
    display_set_auto_update_period(DEFAULT_UPDATE_PERIOD);
}

void nxt_deinit() {

    sound_freq(1000, 100, 30);
    display_reset();
    nxt_motor_reset_all();
    udp_reset();
    bt_reset();
    bt_disable();
    hs_disable();
    i2c_disable_all();
    sound_reset();

    // Erase firmware on shutdown, for easy development
    shutdown(1);
}

// defined in linker script
extern uint32_t _estack;
extern uint32_t __bss_end__;

#if MICROPY_ENABLE_GC
static char heap[PYBRICKS_HEAP_KB * 1024];
#endif


// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    display_string(str);
    display_update();
}

// callback for when stop button is pressed
static void user_program_stop_func(void) {
    // we can only raise an exception if the VM is running
    // mp_interrupt_char will be either -1 or 0 when VM is not running
    if (mp_interrupt_char > 0) {
        mp_sched_keyboard_interrupt();
    }
}

static bool user_program_stdin_event_func(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_sched_keyboard_interrupt();
        return true;
    }

    return false;
}

static const pbsys_user_program_callbacks_t user_program_callbacks = {
    .stop = user_program_stop_func,
    .stdin_event = user_program_stdin_event_func,
};

// Defined in linker script
extern uint32_t _pb_user_mpy_size;
extern uint8_t _pb_user_mpy_data;

int main(int argc, char **argv) {

    pbio_init();
    nxt_init();

    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&__bss_end__ - 1024);

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    // Get system hardware ready
    pbsys_user_program_prepare(&user_program_callbacks);

    // Initialize MicroPython and run default imports
    mp_init();

    // Run a program
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // run user .mpy file
        mp_reader_t reader;
        mp_reader_new_mem(&reader, &_pb_user_mpy_data, _pb_user_mpy_size, 0);
        mp_raw_code_t *raw_code = mp_raw_code_load(&reader);
        mp_obj_t module_fun = mp_make_function_from_raw_code(raw_code, MP_OBJ_NULL, MP_OBJ_NULL);
        mp_hal_set_interrupt_char(3);
        mp_call_function_0(module_fun);
        mp_hal_set_interrupt_char(-1);
        mp_handle_pending(true);
        nlr_pop();
    } else {
        // uncaught exception
        mp_hal_set_interrupt_char(-1); // disable interrupt
        mp_handle_pending(false); // clear any pending exceptions (and run any callbacks)

        pbsys_user_program_unprepare();
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }

    // Uninitialize MicroPython and the system hardware
    mp_deinit();

    nxt_deinit();
    return 0;
}

// defined in linker script
extern uint32_t _estack;
// defined in shared/runtime/gchelper_arm7tdmi.s
extern uintptr_t gc_helper_get_regs_and_sp(uintptr_t *regs);

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
