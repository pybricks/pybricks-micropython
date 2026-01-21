// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

// Common configuration of Pybricks MicroPython ports.

// REVISIT: This file is still largely focused on embedded MicroPython ports.
// Further generalization and possibly a higher option set are needed to use
// this with unix-based ports like the virtual hub.

#include <stdint.h>
#include <pbdrv/config.h>

#define MICROPY_BANNER_NAME_AND_VERSION "Pybricks MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE

#define MICROPY_ENABLE_COMPILER                 (PYBRICKS_OPT_COMPILER)

// Enabled modules
#define MICROPY_PY_IO                           (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_MATH                         (PYBRICKS_OPT_FLOAT)
#define MICROPY_PY_MICROPYTHON                  (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_STRUCT                       (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_SYS                          (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_ERRNO                        (1)
#define MICROPY_PY_JSON                         (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_RANDOM                       (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_SELECT                       (PYBRICKS_OPT_EXTRA_LEVEL1)

#define MICROPY_PY_ERRNO_LIST \
    X(EPERM) \
    X(EIO) \
    X(EBUSY) \
    X(ENODEV) \
    X(EINVAL) \
    X(EOPNOTSUPP) \
    X(EAGAIN) \
    X(ETIMEDOUT) \
    X(ECANCELED) \

#if PYBRICKS_OPT_EXTRA_LEVEL2
#define MICROPY_QSTR_BYTES_IN_HASH              (2)
#else
#define MICROPY_QSTR_BYTES_IN_HASH              (0)
#endif
#define MICROPY_ALLOC_PATH_MAX                  (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT          (16)
#define MICROPY_EMIT_X64                        (PYBRICKS_OPT_NATIVE_MOD && __x86_64__)
#define MICROPY_EMIT_THUMB                      (PYBRICKS_OPT_NATIVE_MOD && __thumb2__)
#define MICROPY_EMIT_INLINE_THUMB               (0)
#define MICROPY_COMP_MODULE_CONST               (0)
#define MICROPY_COMP_CONST                      (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN        (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN        (0)
#define MICROPY_MEM_STATS                       (0)
#define MICROPY_DEBUG_PRINTERS                  (0)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_ENABLE_FINALISER                (1)
#define MICROPY_GC_ALLOC_THRESHOLD              (PYBRICKS_OPT_EXTRA_LEVEL2)
#define MICROPY_STACK_CHECK                     (1)
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_HELPER_LEXER_UNIX               (0)
#define MICROPY_ENABLE_DOC_STRING               (0)
#if PYBRICKS_OPT_TERSE_ERR
#define MICROPY_ENABLE_SOURCE_LINE              (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_NONE)
#else
#define MICROPY_ENABLE_SOURCE_LINE              (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_DETAILED)
#endif
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG   (1)
#define MICROPY_PY_ASYNC_AWAIT                  (1)
#define MICROPY_MULTIPLE_INHERITANCE            (0)
#define MICROPY_PY_ARRAY                        (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY           (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW          (PYBRICKS_OPT_EXTRA_LEVEL2)
#define MICROPY_PY_BUILTINS_ENUMERATE           (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_FILTER              (0)
#define MICROPY_PY_BUILTINS_FROZENSET           (0)
#define MICROPY_PY_BUILTINS_HELP                (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_HELP_MODULES        (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_INPUT               (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_REVERSED            (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_SET                 (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_SLICE               (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_BUILTINS_PROPERTY            (0)
#define MICROPY_PY_BUILTINS_MIN_MAX             (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE         (PYBRICKS_OPT_EXTRA_LEVEL2)
#define MICROPY_PY___FILE__                     (0)
#define MICROPY_PY_MICROPYTHON_MEM_INFO         (1)
#define MICROPY_PY_GC                           (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_ATTRTUPLE                    (0)
#define MICROPY_PY_COLLECTIONS                  (0)
#define MICROPY_PY_CMATH                        (0)
#define MICROPY_PY_ALL_SPECIAL_METHODS          (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS      (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_SYS_EXIT                     (0)
#define MICROPY_PY_SYS_MODULES                  (0)
#define MICROPY_PY_SYS_STDFILES                 (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_SYS_STDIO_BUFFER             (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_SYS_STDIO_FLUSH              (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS           (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PY_RANDOM_SEED_INIT_FUNC        ({ extern uint32_t pbdrv_clock_get_us(void); pbdrv_clock_get_us(); })
#define MICROPY_MODULE_BUILTIN_INIT             (MICROPY_PY_RANDOM)
#define MICROPY_CPYTHON_COMPAT                  (0)
#define MICROPY_LONGINT_IMPL                    (MICROPY_LONGINT_IMPL_NONE)
#if PYBRICKS_OPT_FLOAT
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)
#else
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_NONE)
#endif
#define MICROPY_KBD_EXCEPTION                   (1)
#define MICROPY_ENABLE_VM_ABORT                 (1)
#define MICROPY_ENABLE_SCHEDULER                (0)
#define MICROPY_PY_INSTANCE_ATTRS               (1)

#define MICROPY_MODULE_ATTR_DELEGATION          (PYBRICKS_OPT_EXTRA_LEVEL1)
#define MICROPY_PERSISTENT_CODE_LOAD            (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT          (0)
#define MICROPY_HAS_FILE_READER                 (0)
#define MICROPY_VFS_ROM                         (1)
#define MICROPY_VFS_ROM_IOCTL                   (0)
#if PYBRICKS_OPT_CUSTOM_IMPORT
#define mp_builtin___import__ pb_builtin_import
#endif

#define MICROPY_MPHALPORT_H "../_common/mphalport.h"

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
#define HEX_FMT "%x"
typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size

typedef long mp_off_t;

#include "pbio_os_config.h"

#if PBDRV_CONFIG_STACK_EMBEDDED
#define MICROPY_BEGIN_ATOMIC_SECTION()     pbio_os_hook_disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  pbio_os_hook_enable_irq(state)
#else
#define MICROPY_BEGIN_ATOMIC_SECTION() (0)
#define MICROPY_END_ATOMIC_SECTION(state) (void)(state)
#endif

// Optional extra code to run before MicroPython drives the event loop.
#ifndef PYBRICKS_VM_HOOK_LOOP_EXTRA
#define PYBRICKS_VM_HOOK_LOOP_EXTRA
#endif

#define MICROPY_VM_HOOK_LOOP \
    do { \
        PYBRICKS_VM_HOOK_LOOP_EXTRA \
        extern bool pbio_os_run_processes_once(void); \
        pbio_os_run_processes_once(); \
    } while (0);

#define MICROPY_GC_HOOK_LOOP(i) do { \
        if (((i) & 0xf) == 0) { \
            MICROPY_VM_HOOK_LOOP \
        } \
} while (0)

#define MICROPY_INTERNAL_EVENT_HOOK \
    do { \
        PYBRICKS_VM_HOOK_LOOP_EXTRA \
        extern bool pbio_os_run_processes_once(void); \
        while (pbio_os_run_processes_once()) { \
        } \
    } while (0);

#define MICROPY_INTERNAL_WFE(TIMEOUT_MS) \
    do { \
        extern void pbio_os_run_processes_and_wait_for_event(void); \
        pbio_os_run_processes_and_wait_for_event(); \
    } while (0);

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MP_STATE_PORT MP_STATE_VM
