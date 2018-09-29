#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/main.h>
#include <pbsys/sys.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"

#include "accel.h"
#include "uartadr.h"

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
#endif

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    uart_init();

    accel_init();
    pbio_init();

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

soft_reset:
    pbsys_prepare_user_program();

    mp_init();
    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif
    #else
    #if MICROPY_PERSISTENT_CODE_LOAD
    run_user_program();
    #else
    pyexec_frozen_module("frozentest.py");
    #endif
    #endif
    mp_deinit();

    pbsys_unprepare_user_program();

    goto soft_reset;

    accel_deinit();
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
