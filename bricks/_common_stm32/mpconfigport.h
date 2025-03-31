// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2018-2023 The Pybricks Authors

// Contains the MicroPython configuration for all STM32-based Pybricks ports.

// Start with config shared by all Pybricks ports.
#include "../_common/mpconfigport.h"

#define MICROPY_MPHALPORT_H "../_common/mphalport.h"

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

#include "pbio_os_config.h"

#define MICROPY_BEGIN_ATOMIC_SECTION()     pbio_os_hook_disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  pbio_os_hook_enable_irq(state)

#define MICROPY_VM_HOOK_LOOP \
    do { \
        extern bool pbio_os_run_processes_once(void); \
        pbio_os_run_processes_once(); \
    } while (0);

#define MICROPY_GC_HOOK_LOOP(i) do { \
        if (((i) & 0xf) == 0) { \
            MICROPY_VM_HOOK_LOOP \
        } \
} while (0)

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void pb_event_poll_hook(void); \
        pb_event_poll_hook(); \
    } while (0);

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MP_STATE_PORT MP_STATE_VM
