#include <stdint.h>


#define MICROPY_HW_BOARD_NAME           "LEGO MINDSTORMS NXT Brick"
#define MICROPY_HW_MCU_NAME             "AT91SAM7S256"

#define PYBRICKS_HEAP_KB                32 // half of RAM

// Pybricks modules
#define PYBRICKS_PY_BATTERY             (1)
#define PYBRICKS_PY_NXTDEVICES          (1)
#define PYBRICKS_PY_MOTOR               (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_ROBOTICS            (1)

// options to control how MicroPython is built

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER     (1)

#define MICROPY_QSTR_BYTES_IN_HASH  (1)
#define MICROPY_QSTR_EXTRA_POOL     mp_qstr_frozen_const_pool
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
#define MICROPY_REPL_EVENT_DRIVEN   (0)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (0)
#define MICROPY_ENABLE_SOURCE_LINE  (0)
#define MICROPY_ENABLE_DOC_STRING   (0)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT      (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (0)
#define MICROPY_PY_BUILTINS_DICT_FROMKEYS (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#define MICROPY_PY_BUILTINS_ENUMERATE (0)
#define MICROPY_PY_BUILTINS_FILTER  (0)
#define MICROPY_PY_BUILTINS_FROZENSET (0)
#define MICROPY_PY_BUILTINS_REVERSED (0)
#define MICROPY_PY_BUILTINS_SET     (0)
#define MICROPY_PY_BUILTINS_SLICE   (0)
#define MICROPY_PY_BUILTINS_PROPERTY (0)
#define MICROPY_PY_BUILTINS_MIN_MAX (0)
#define MICROPY_PY_BUILTINS_STR_COUNT (0)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (0)
#define MICROPY_PY___FILE__         (0)
#define MICROPY_PY_GC               (0)
#define MICROPY_PY_ARRAY            (0)
#define MICROPY_PY_ATTRTUPLE        (0)
#define MICROPY_PY_COLLECTIONS      (0)
#define MICROPY_PY_MATH             (0)
#define MICROPY_PY_CMATH            (0)
#define MICROPY_PY_IO               (0)
#define MICROPY_PY_STRUCT           (0)
#define MICROPY_PY_SYS              (0)
#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_NONE)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_NONE)
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_ENABLE_SCHEDULER    (0)
#define MICROPY_PY_INSTANCE_ATTRS   (1)

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn(str, len)

#if PYBRICKS_PY_PARAMETERS
extern const struct _mp_obj_module_t pb_module_parameters;
#define _PYBRICKS_MODULE_PARAMETERS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_parameters), (mp_obj_t)&pb_module_parameters },
#else
#define _PYBRICKS_MODULE_PARAMETERS
#endif
#if PYBRICKS_PY_TOOLS
extern const struct _mp_obj_module_t pb_module_tools;
#define _PYBRICKS_MODULE_TOOLS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_tools), (mp_obj_t)&pb_module_tools },
#else
#define _PYBRICKS_MODULE_TOOLS
#endif
#if PYBRICKS_PY_ROBOTICS
extern const struct _mp_obj_module_t pb_module_robotics;
#define _PYBRICKS_MODULE_ROBOTICS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_robotics), (mp_obj_t)&pb_module_robotics },
#else
#define _PYBRICKS_MODULE_ROBOTICS
#endif
#if PYBRICKS_PY_NXTDEVICES
extern const struct _mp_obj_module_t pb_module_nxtdevices;
#define _PYBRICKS_MODULE_NXTDEVICES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_nxtdevices), (mp_obj_t)&pb_module_nxtdevices },
#else
#define _PYBRICKS_MODULE_NXTDEVICES
#endif

#define MICROPY_PORT_BUILTIN_MODULES \
    _PYBRICKS_MODULE_PARAMETERS     \
    _PYBRICKS_MODULE_NXTDEVICES     \
    _PYBRICKS_MODULE_TOOLS          \
    _PYBRICKS_MODULE_ROBOTICS       \


// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#include <nxt/interrupts.h>
#include <nxt/systick.h>

#define MICROPY_BEGIN_ATOMIC_SECTION()     interrupts_get_and_disable()
#define MICROPY_END_ATOMIC_SECTION(state)  if (state) { interrupts_enable(); }

#define MICROPY_VM_HOOK_LOOP \
    do { \
        extern int pbio_do_one_event(void); \
        pbio_do_one_event(); \
    } while (0);

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
        extern int pbio_do_one_event(void); \
        while (pbio_do_one_event()) { } \
    } while (0);

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8];
