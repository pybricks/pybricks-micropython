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
#include "adc.h"
#include "button.h"
#include "uartadr.h"

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[8 * 1024];
#endif

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    uart_init();

    button_init();
    adc_init();
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
    pyexec_frozen_module("frozentest.py");
    #endif
    mp_deinit();

    pbsys_unprepare_user_program();

    goto soft_reset;

    accel_deinit();
    adc_deinit();
    button_deinit();
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
    #if MICROPY_PY_THREAD
    gc_collect_root((void**)sp, ((uint32_t)MP_STATE_THREAD(stack_top) - sp) / sizeof(uint32_t));
    #else
    gc_collect_root((void**)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));
    #endif

    // trace root pointers from any threads
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif

    // end the GC
    gc_collect_end();

    // for debug during development
    gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
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
