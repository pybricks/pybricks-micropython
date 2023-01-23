// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

// Common configuration of Pybricks MicroPython ports.

// REVISIT: This file is still largely focused on embedded MicroPython ports.
// Further generalization and possibly a higher option set are needed to use
// this with unix-based ports like the virtual hub or ev3dev.

#include <stdint.h>
#include <pbdrv/config.h>

#define MICROPY_BANNER_NAME_AND_VERSION "Pybricks MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE

#define MICROPY_ENABLE_COMPILER                 (PYBRICKS_OPT_COMPILER)

// Enabled modules
#define MICROPY_PY_IO                           (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_MATH                         (PYBRICKS_OPT_FLOAT)
#define MICROPY_PY_MICROPYTHON                  (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_STRUCT                       (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_SYS                          (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_UERRNO                       (1)
#define MICROPY_PY_UJSON                        (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_URANDOM                      (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_USELECT                      (PYBRICKS_OPT_EXTRA_MOD)

#define MICROPY_PY_UERRNO_LIST \
    X(EPERM) \
    X(EIO) \
    X(EBUSY) \
    X(ENODEV) \
    X(EINVAL) \
    X(EOPNOTSUPP) \
    X(EAGAIN) \
    X(ETIMEDOUT) \
    X(ECANCELED) \

#define MICROPY_QSTR_BYTES_IN_HASH              (1)
#define MICROPY_ALLOC_PATH_MAX                  (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT          (16)
#define MICROPY_EMIT_X64                        (0)
#define MICROPY_EMIT_THUMB                      (0)
#define MICROPY_EMIT_INLINE_THUMB               (0)
#define MICROPY_COMP_MODULE_CONST               (0)
#define MICROPY_COMP_CONST                      (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN        (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN        (0)
#define MICROPY_MEM_STATS                       (0)
#define MICROPY_DEBUG_PRINTERS                  (0)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_GC_ALLOC_THRESHOLD              (0)
#define MICROPY_STACK_CHECK                     (1)
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_HELPER_LEXER_UNIX               (0)
#define MICROPY_ENABLE_DOC_STRING               (0)
#if PYBRICKS_OPT_TERSE_ERR
#define MICROPY_ENABLE_SOURCE_LINE              (0)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_TERSE)
#else
#define MICROPY_ENABLE_SOURCE_LINE              (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_DETAILED)
#endif
#define MICROPY_ENABLE_SYSTEM_ABORT             (1)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG   (1)
#define MICROPY_PY_ASYNC_AWAIT                  (0)
#define MICROPY_MULTIPLE_INHERITANCE            (0)
#define MICROPY_PY_ARRAY                        (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY           (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_MEMORYVIEW          (0)
#define MICROPY_PY_BUILTINS_ENUMERATE           (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_FILTER              (0)
#define MICROPY_PY_BUILTINS_FROZENSET           (0)
#define MICROPY_PY_BUILTINS_HELP                (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_HELP_MODULES        (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_INPUT               (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_REVERSED            (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_SET                 (0)
#define MICROPY_PY_BUILTINS_SLICE               (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_BUILTINS_PROPERTY            (0)
#define MICROPY_PY_BUILTINS_MIN_MAX             (1)
#define MICROPY_PY___FILE__                     (0)
#define MICROPY_PY_MICROPYTHON_MEM_INFO         (1)
#define MICROPY_PY_GC                           (0)
#define MICROPY_PY_ARRAY                        (0)
#define MICROPY_PY_ATTRTUPLE                    (0)
#define MICROPY_PY_COLLECTIONS                  (0)
#define MICROPY_PY_CMATH                        (0)
#define MICROPY_PY_ALL_SPECIAL_METHODS          (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS      (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_SYS_EXIT                     (0)
#define MICROPY_PY_SYS_MODULES                  (0)
#define MICROPY_PY_SYS_STDFILES                 (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_SYS_STDIO_BUFFER             (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_SYS_STDIO_FLUSH              (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS          (PYBRICKS_OPT_EXTRA_MOD)
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC       ({ extern uint32_t pbdrv_clock_get_us(void); pbdrv_clock_get_us(); })
#define MICROPY_PY_UTIME_MP_HAL                 (0)
#define MICROPY_MODULE_BUILTIN_INIT             (1)
#define MICROPY_MODULE_WEAK_LINKS               (0)
#define MICROPY_CPYTHON_COMPAT                  (0)
#define MICROPY_LONGINT_IMPL                    (MICROPY_LONGINT_IMPL_NONE)
#if PYBRICKS_OPT_FLOAT
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)
#else
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_NONE)
#endif
#define MICROPY_KBD_EXCEPTION                   (1)
#define MICROPY_ENABLE_SCHEDULER                (0)
#define MICROPY_PY_INSTANCE_ATTRS               (1)

#define MICROPY_PERSISTENT_CODE_LOAD            (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT          (0)
#define MICROPY_HAS_FILE_READER                 (0)
#define MICROPY_VFS_MAP_MINIMAL                 (1)
#if PYBRICKS_OPT_CUSTOM_IMPORT
#define mp_builtin___import__ pb_builtin_import
#endif
