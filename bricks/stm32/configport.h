// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>
#include <pbdrv/config.h>

// options to control how MicroPython is built

#define MICROPY_ENABLE_COMPILER     (PYBRICKS_STM32_OPT_COMPILER)

#define MICROPY_QSTR_BYTES_IN_HASH  (1)
#define MICROPY_ALLOC_PATH_MAX      (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)
#define MICROPY_EMIT_X64            (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)
#define MICROPY_COMP_MODULE_CONST   (0)
#define MICROPY_COMP_CONST          (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (0)
#define MICROPY_MEM_STATS           (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_GC_ALLOC_THRESHOLD  (0)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (0)
#define MICROPY_ENABLE_DOC_STRING   (0)
#if PYBRICKS_STM32_OPT_TERSE_ERR
#define MICROPY_ENABLE_SOURCE_LINE  (0)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#else
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#endif
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT      (0)
#define MICROPY_MULTIPLE_INHERITANCE (0)
#define MICROPY_PY_ARRAY (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_FILTER  (0)
#define MICROPY_PY_BUILTINS_FROZENSET (0)
#define MICROPY_PY_BUILTINS_HELP            (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_HELP_MODULES    (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_INPUT           (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_REVERSED (0)
#define MICROPY_PY_BUILTINS_SET     (0)
#define MICROPY_PY_BUILTINS_SLICE   (0)
#define MICROPY_PY_BUILTINS_PROPERTY (0)
#define MICROPY_PY_BUILTINS_MIN_MAX (1)
#define MICROPY_PY___FILE__         (0)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_PY_GC               (0)
#define MICROPY_PY_ARRAY            (0)
#define MICROPY_PY_ATTRTUPLE        (0)
#define MICROPY_PY_COLLECTIONS      (0)
#define MICROPY_PY_MATH                     (PYBRICKS_STM32_OPT_FLOAT)
#define MICROPY_PY_CMATH            (0)
#define MICROPY_PY_IO               (0)
#define MICROPY_PY_STRUCT           (0)
#define MICROPY_PY_SYS              (0)
#define MICROPY_PY_SYS_EXIT         (0)
#define MICROPY_PY_SYS_MODULES      (0)
#define MICROPY_PY_URANDOM                  (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS      (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC   { extern unsigned long clock_usecs(); clock_usecs(); }
#define MICROPY_PY_UTIME_MP_HAL             (0)
// MICROPY_MODULE_BUILTIN_INIT is needed for urandom.__init__
#define MICROPY_MODULE_BUILTIN_INIT         (PYBRICKS_STM32_OPT_EXTRA_MOD)
#define MICROPY_MODULE_WEAK_LINKS   (0)
#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_NONE)
#if PYBRICKS_STM32_OPT_FLOAT
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
#else
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_NONE)
#endif
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_ENABLE_SCHEDULER    (0)
#define MICROPY_PY_UERRNO           (1)
#define MICROPY_PY_INSTANCE_ATTRS   (1)

#define MICROPY_PERSISTENT_CODE_LOAD    (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT  (0)

#define MICROPY_PY_UERRNO_LIST \
    X(EPERM) \
    X(EIO) \
    X(ENODEV) \
    X(EINVAL) \
    X(EOPNOTSUPP) \
    X(EAGAIN) \
    X(ETIMEDOUT) \
    X(ECANCELED) \

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

// Pybricks modules

extern const struct _mp_obj_module_t pb_package_pybricks;
#define _PYBRICKS_PACKAGE_PYBRICKS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_pybricks), (mp_obj_t)&pb_package_pybricks },

extern const struct _mp_obj_module_t pb_module_hubs;
#define _PYBRICKS_MODULE_HUBS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hubs), (mp_obj_t)&pb_module_hubs},

#if PYBRICKS_PY_EXPERIMENTAL
extern const struct _mp_obj_module_t pb_module_experimental;
#define _PYBRICKS_MODULE_EXPERIMENTAL \
    { MP_OBJ_NEW_QSTR(MP_QSTR_experimental), (mp_obj_t)&pb_module_experimental },
#else
#define _PYBRICKS_MODULE_EXPERIMENTAL
#endif
#if PYBRICKS_PY_IODEVICES
extern const struct _mp_obj_module_t pb_module_iodevices;
#define _PYBRICKS_MODULE_IODEVICES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_iodevices), (mp_obj_t)&pb_module_iodevices },
#else
#define _PYBRICKS_MODULE_IODEVICES
#endif
#if PYBRICKS_PY_PUPDEVICES
extern const struct _mp_obj_module_t pb_module_pupdevices;
#define _PYBRICKS_MODULE_PUPDEVICES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_pupdevices), (mp_obj_t)&pb_module_pupdevices },
#else
#define _PYBRICKS_MODULE_PUPDEVICES
#endif
#if PYBRICKS_PY_PARAMETERS
extern const struct _mp_obj_module_t pb_module_parameters;
#define _PYBRICKS_MODULE_PARAMETERS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_parameters), (mp_obj_t)&pb_module_parameters },
#else
#define _PYBRICKS_MODULE_PARAMETERS
#endif

#define MICROPY_PORT_BUILTIN_MODULES \
    _PYBRICKS_PACKAGE_PYBRICKS      \
    _PYBRICKS_MODULE_EXPERIMENTAL   \
    _PYBRICKS_MODULE_HUBS           \
    _PYBRICKS_MODULE_IODEVICES      \
    _PYBRICKS_MODULE_PARAMETERS     \
    _PYBRICKS_MODULE_PUPDEVICES     \


// We have inlined IRQ functions for efficiency (they are generally
// 1 machine instruction).
//
// Note on IRQ state: you should not need to know the specific
// value of the state variable, but rather just pass the return
// value from disable_irq back to enable_irq.  If you really need
// to know the machine-specific values, see irq.h.

static inline void enable_irq(mp_uint_t state) {
    __set_PRIMASK(state);
}

static inline mp_uint_t disable_irq(void) {
    mp_uint_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

#define MICROPY_BEGIN_ATOMIC_SECTION()     disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  enable_irq(state)

#define MICROPY_VM_HOOK_LOOP \
    do { \
        extern int pbio_do_one_event(void); \
        pbio_do_one_event(); \
    } while (0);

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
        extern int pbio_do_one_event(void); \
        while (pbio_do_one_event()) { } \
        __WFI(); \
    } while (0);

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8];

#include "../pybricks_config.h"
