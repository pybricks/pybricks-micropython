// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2018-2022 The Pybricks Authors

// Pybricks brick specific definitions
#include "brickconfig.h"

static const char pybricks_ev3dev_help_text[] =
    "Welcome to Pybricks MicroPython!\n"
    "\n"
    "For online docs please visit http://docs.pybricks.com/micropython\n"
    "\n"
    "Control commands:\n"
    "  CTRL-C        -- interrupt a running program\n"
    "  CTRL-D        -- on a blank line, exit\n"
    "  CTRL-E        -- on a blank line, enter paste mode\n"
    "\n"
    "For further help on a specific object, type help(obj)\n"
;

// options to control how MicroPython is built

#define MICROPY_BANNER_NAME_AND_VERSION "Pybricks MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE

#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#if !defined(MICROPY_EMIT_X64) && defined(__x86_64__)
    #define MICROPY_EMIT_X64        (1)
#endif
#if !defined(MICROPY_EMIT_X86) && defined(__i386__)
    #define MICROPY_EMIT_X86        (1)
#endif
#if !defined(MICROPY_EMIT_THUMB) && defined(__thumb2__)
    #define MICROPY_EMIT_THUMB      (1)
    #define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))
#endif
// Some compilers define __thumb2__ and __arm__ at the same time, let
// autodetected thumb2 emitter have priority.
#if !defined(MICROPY_EMIT_ARM) && defined(__arm__) && !defined(__thumb2__)
    #define MICROPY_EMIT_ARM        (1)
#endif
#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_RETURN_IF_EXPR (1)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_ENABLE_FINALISER    (1)
#define MICROPY_STACK_CHECK         (1)
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)
#define MICROPY_MEM_STATS           (1)
#define MICROPY_DEBUG_PRINTERS      (1)
// Printing debug to stderr may give tests which
// check stdout a chance to pass, etc.
#define MICROPY_DEBUG_PRINTER       (&mp_stderr_print)
#define MICROPY_READER_POSIX        (1)
#define MICROPY_READER_VFS          (1)
#define MICROPY_USE_READLINE_HISTORY (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_REPL_EMACS_KEYS     (1)
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#define MICROPY_ENABLE_DOC_STRING   (1)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_STREAMS_NON_BLOCK   (1)
#define MICROPY_STREAMS_POSIX_API   (1)
#define MICROPY_OPT_COMPUTED_GOTO   (1)
#define MICROPY_MODULE_OVERRIDE_MAIN_IMPORT (1)
#define MICROPY_VFS                 (1)
#define MICROPY_VFS_POSIX           (1)
#define MICROPY_PY_SYS_PATH_ARGV_DEFAULTS (0)
#ifndef MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#endif
#define MICROPY_MODULE_WEAK_LINKS   (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS (1)
#define MICROPY_PY_FUNCTION_ATTRS   (1)
#define MICROPY_PY_DESCRIPTORS      (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_STR_CENTER (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (1)
#define MICROPY_PY_BUILTINS_INPUT   (1)
#define MICROPY_PY_BUILTINS_HELP    (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT pybricks_ev3dev_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES (1)
#define MICROPY_PY_BUILTINS_POW3    (1)
#define MICROPY_PY_BUILTINS_ROUND_INT    (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS (1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES (1)
#define MICROPY_PY_INSTANCE_ATTRS   (1)
#define MICROPY_PY_SYS_EXIT         (1)
#define MICROPY_PY_SYS_ATEXIT       (1)
#if MICROPY_PY_SYS_SETTRACE
#define MICROPY_PERSISTENT_CODE_SAVE (1)
#define MICROPY_COMP_CONST (0)
#endif
#ifndef MICROPY_PY_SYS_PLATFORM
#if defined(__APPLE__) && defined(__MACH__)
    #define MICROPY_PY_SYS_PLATFORM  "darwin"
#else
    #define MICROPY_PY_SYS_PLATFORM  "linux"
#endif
#endif
#ifndef MICROPY_PY_SYS_PATH_DEFAULT
#define MICROPY_PY_SYS_PATH_DEFAULT ".frozen:~/.pybricks-micropython/lib:/usr/lib/pybricks-micropython"
#endif
#define MICROPY_PY_SYS_MAXSIZE      (1)
#define MICROPY_PY_SYS_STDFILES     (1)
#define MICROPY_PY_SYS_EXC_INFO     (0)
#define MICROPY_PY_COLLECTIONS_DEQUE (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#ifndef MICROPY_PY_MATH_SPECIAL_FUNCTIONS
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (1)
#endif
#define MICROPY_PY_CMATH            (1)
#define MICROPY_PY_IO_IOBASE        (1)
#define MICROPY_PY_IO_FILEIO        (1)
#define MICROPY_PY_GC_COLLECT_RETVAL (1)
#define MICROPY_MODULE_ATTR_DELEGATION (1)
#define MICROPY_MODULE_BUILTIN_INIT (1)

#define MICROPY_PY_THREAD           (1)
#define MICROPY_PY_THREAD_GIL       (1)

#ifndef MICROPY_STACKLESS
#define MICROPY_STACKLESS           (0)
#define MICROPY_STACKLESS_STRICT    (0)
#endif

#define MICROPY_OPT_MATH_FACTORIAL              (0)
#define MICROPY_FLOAT_HIGH_QUALITY_HASH         (1)
#define MICROPY_ENABLE_SCHEDULER                (0)
#define MICROPY_WARNINGS_CATEGORY               (1)
#define MICROPY_MODULE_GETATTR                  (1)
#define MICROPY_PY_DELATTR_SETATTR              (1)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS      (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW_ITEMSIZE (1)
#define MICROPY_PY_BUILTINS_NEXT2               (1)
#define MICROPY_PY_BUILTINS_RANGE_BINOP         (1)
#define MICROPY_PY_SYS_GETSIZEOF                (1)
#define MICROPY_PY_MATH_FACTORIAL               (1)
#define MICROPY_PY_IO_BUFFEREDWRITER            (1)
#define MICROPY_PY_IO_RESOURCE_STREAM           (1)
#define MICROPY_PY_URE_MATCH_GROUPS             (1)
#define MICROPY_PY_URE_MATCH_SPAN_START_END     (1)
#define MICROPY_PY_URE_SUB                      (1)
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT (1)

#define MICROPY_PY_FRAMEBUF         (1)
#define MICROPY_PY_UOS              (1)
#define MICROPY_PY_UOS_INCLUDEFILE  "ports/unix/moduos.c"
#define MICROPY_PY_UOS_ERRNO        (1)
#define MICROPY_PY_UOS_GETENV_PUTENV_UNSETENV (1)
#define MICROPY_PY_UOS_SEP          (1)
#define MICROPY_PY_UOS_STATVFS      (0)
#define MICROPY_PY_UOS_SYSTEM       (1)
#define MICROPY_PY_UOS_URANDOM      (1)
#define MICROPY_PY_UTIME            (1)
#define MICROPY_PY_UTIME_MP_HAL     (1)
#define MICROPY_PY_UERRNO           (1)
#define MICROPY_PY_UERRNO_LIST \
    X(EPERM) \
    X(ENOENT) \
    X(ESRCH) \
    X(EINTR) \
    X(EIO) \
    X(ENXIO) \
    X(E2BIG) \
    X(ENOEXEC) \
    X(EBADF) \
    X(ECHILD) \
    X(EAGAIN) \
    X(ENOMEM) \
    X(EACCES) \
    X(EFAULT) \
    X(ENOTBLK) \
    X(EBUSY) \
    X(EEXIST) \
    X(EXDEV) \
    X(ENODEV) \
    X(ENOTDIR) \
    X(EISDIR) \
    X(EINVAL) \
    X(ENFILE) \
    X(EMFILE) \
    X(ENOTTY) \
    X(ETXTBSY) \
    X(EFBIG) \
    X(ENOSPC) \
    X(ESPIPE) \
    X(EROFS) \
    X(EMLINK) \
    X(EPIPE) \
    X(EDOM) \
    X(ERANGE) \
    X(EOPNOTSUPP) \
    X(EAFNOSUPPORT) \
    X(EADDRINUSE) \
    X(ECONNABORTED) \
    X(ECONNRESET) \
    X(ENOBUFS) \
    X(EISCONN) \
    X(ENOTCONN) \
    X(ETIMEDOUT) \
    X(ECONNREFUSED) \
    X(EHOSTUNREACH) \
    X(EALREADY) \
    X(EINPROGRESS) \
    X(ECANCELED) \

#define MICROPY_PY_UCTYPES          (1)
#define MICROPY_PY_UZLIB            (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_URE              (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UTIMEQ           (1)
#define MICROPY_PY_USOCKET_LISTEN_BACKLOG_DEFAULT (SOMAXCONN < 128 ? SOMAXCONN : 128)
#define MICROPY_PY_UHASHLIB         (1)
#if MICROPY_PY_USSL
#define MICROPY_PY_UHASHLIB_MD5     (1)
#define MICROPY_PY_UHASHLIB_SHA1    (1)
#define MICROPY_PY_UCRYPTOLIB       (1)
#endif
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_UBINASCII_CRC32  (1)
#define MICROPY_PY_UFCNTL_POSIX     (1)
#define MICROPY_PY_URANDOM          (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS      (1)
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC (mp_urandom_seed_init())
#ifndef MICROPY_PY_USELECT_POSIX
#define MICROPY_PY_USELECT_POSIX    (1)
#endif
#define MICROPY_PY_UWEBSOCKET       (1)
#define MICROPY_PY_MACHINE          (1)
#define MICROPY_PY_MACHINE_PULSE    (1)
#define MICROPY_MACHINE_MEM_GET_READ_ADDR   mod_machine_mem_get_addr
#define MICROPY_MACHINE_MEM_GET_WRITE_ADDR  mod_machine_mem_get_addr

#define MICROPY_FATFS_ENABLE_LFN       (1)
#define MICROPY_FATFS_RPATH            (2)
#define MICROPY_FATFS_MAX_SS           (4096)
#define MICROPY_FATFS_LFN_CODE_PAGE    437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */

// Define to MICROPY_ERROR_REPORTING_DETAILED to get function, etc.
// names in exception messages (may require more RAM).
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_WARNINGS            (1)
#define MICROPY_ERROR_PRINTER       (&mp_stderr_print)
#define MICROPY_PY_STR_BYTES_CMP_WARN (1)

// VFS stat functions should return time values relative to 1970/1/1
#define MICROPY_EPOCH_IS_1970       (1)

extern const struct _mp_print_t mp_stderr_print;

#if !(defined(MICROPY_GCREGS_SETJMP) || defined(__x86_64__) || defined(__i386__) || defined(__thumb2__) || defined(__thumb__) || defined(__arm__))
// Fall back to setjmp() implementation for discovery of GC pointers in registers.
#define MICROPY_GCREGS_SETJMP (1)
#endif

#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF   (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE  (256)
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_ASYNC_KBD_INTR      (0)

#define mp_type_fileio mp_type_vfs_posix_fileio
#define mp_type_textio mp_type_vfs_posix_textio

// type definitions for the specific machine

// For size_t and ssize_t
#include <unistd.h>

// assume that if we already defined the obj repr then we also defined types
#ifndef MICROPY_OBJ_REPR
#ifdef __LP64__
typedef long mp_int_t; // must be pointer size
typedef unsigned long mp_uint_t; // must be pointer size
#else
// These are definitions for machines where sizeof(int) == sizeof(void*),
// regardless of actual size.
typedef int mp_int_t; // must be pointer size
typedef unsigned int mp_uint_t; // must be pointer size
#endif
#endif

// Cannot include <sys/types.h>, as it may lead to symbol name clashes
#if _FILE_OFFSET_BITS == 64 && !defined(__LP64__)
typedef long long mp_off_t;
#else
typedef long mp_off_t;
#endif

void mp_unix_alloc_exec(size_t min_size, void **ptr, size_t *size);
void mp_unix_free_exec(void *ptr, size_t size);
void mp_unix_mark_exec(void);
#define MP_PLAT_ALLOC_EXEC(min_size, ptr, size) mp_unix_alloc_exec(min_size, ptr, size)
#define MP_PLAT_FREE_EXEC(ptr, size) mp_unix_free_exec(ptr, size)
#ifndef MICROPY_FORCE_PLAT_ALLOC_EXEC
// Use MP_PLAT_ALLOC_EXEC for any executable memory allocation, including for FFI
// (overriding libffi own implementation)
#define MICROPY_FORCE_PLAT_ALLOC_EXEC (1)
#endif

#ifdef MICROPY_PY_URANDOM_SEED_INIT_FUNC
// Support for seeding the random module on import.
#include <stddef.h>
void mp_hal_get_random(size_t n, void *buf);
static inline unsigned long mp_urandom_seed_init(void) {
    unsigned long r;
    mp_hal_get_random(sizeof(r), &r);
    return r;
}
#endif

#ifdef __linux__
// Can access physical memory using /dev/mem
#define MICROPY_PLAT_DEV_MEM  (1)
#endif

// Assume that select() call, interrupted with a signal, and erroring
// with EINTR, updates remaining timeout value.
#define MICROPY_SELECT_REMAINING_TIME (1)

#ifdef __ANDROID__
#include <android/api-level.h>
#if __ANDROID_API__ < 4
// Bionic libc in Android 1.5 misses these 2 functions
#define MP_NEED_LOG2 (1)
#define nan(x) NAN
#endif
#endif

#define MICROPY_PORT_INIT_FUNC pybricks_init()
#define MICROPY_PORT_DEINIT_FUNC pybricks_deinit()
#define MICROPY_MPHALPORT_H "ev3dev_mphal.h"

#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },

#define MP_STATE_PORT MP_STATE_VM

#if MICROPY_PY_BLUETOOTH
#if MICROPY_BLUETOOTH_BTSTACK
struct _mp_bluetooth_btstack_root_pointers_t;
#define MICROPY_BLUETOOTH_ROOT_POINTERS struct _mp_bluetooth_btstack_root_pointers_t *bluetooth_btstack_root_pointers;
#endif
#if MICROPY_BLUETOOTH_NIMBLE
struct _mp_bluetooth_nimble_root_pointers_t;
struct _mp_bluetooth_nimble_malloc_t;
#define MICROPY_BLUETOOTH_ROOT_POINTERS struct _mp_bluetooth_nimble_malloc_t *bluetooth_nimble_memory; struct _mp_bluetooth_nimble_root_pointers_t *bluetooth_nimble_root_pointers;
#endif
#else
#define MICROPY_BLUETOOTH_ROOT_POINTERS
#endif

#define MICROPY_PORT_ROOT_POINTERS \
    mp_obj_dict_t *pb_type_Color_dict; \
    const char *readline_hist[50]; \
    void *mmap_region_head; \
    MICROPY_BLUETOOTH_ROOT_POINTERS \

// We need to provide a declaration/definition of alloca()
// unless support for it is disabled.
#if !defined(MICROPY_NO_ALLOCA) || MICROPY_NO_ALLOCA == 0
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif
#endif

// From "man readdir": "Under glibc, programs can check for the availability
// of the fields [in struct dirent] not defined in POSIX.1 by testing whether
// the macros [...], _DIRENT_HAVE_D_TYPE are defined."
// Other libc's don't define it, but proactively assume that dirent->d_type
// is available on a modern *nix system.
#ifndef _DIRENT_HAVE_D_TYPE
#define _DIRENT_HAVE_D_TYPE (1)
#endif
// This macro is not provided by glibc but we need it so ports that don't have
// dirent->d_ino can disable the use of this field.
#ifndef _DIRENT_HAVE_D_INO
#define _DIRENT_HAVE_D_INO (1)
#endif

#ifndef __APPLE__
// For debugging purposes, make printf() available to any source file.
#include <stdio.h>
#endif

#if MICROPY_PY_THREAD
#define MICROPY_BEGIN_ATOMIC_SECTION() (mp_thread_unix_begin_atomic_section(), 0xffffffff)
#define MICROPY_END_ATOMIC_SECTION(x) (void)x; mp_thread_unix_end_atomic_section()
#endif

#define MICROPY_VM_HOOK_LOOP do { \
        extern int pbio_do_one_event(void); \
        pbio_do_one_event(); \
} while (0);

#include <glib.h>

#define MICROPY_EVENT_POLL_HOOK do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
        extern int pbio_do_one_event(void); \
        while (pbio_do_one_event()) { } \
        MP_THREAD_GIL_EXIT(); \
        g_main_context_iteration(g_main_context_get_thread_default(), TRUE); \
        MP_THREAD_GIL_ENTER(); \
} while (0);


#include <sched.h>
#define MICROPY_UNIX_MACHINE_IDLE sched_yield();

#include "../pybricks_config.h"
