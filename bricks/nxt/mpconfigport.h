// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <stdint.h>

#define MICROPY_BANNER_NAME_AND_VERSION         "Pybricks MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE
#define MICROPY_HW_BOARD_NAME                   "LEGO MINDSTORMS NXT Brick"
#define MICROPY_HW_MCU_NAME                     "AT91SAM7S256"

#define PYBRICKS_HUB_NAME                       "nxt"
#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_NXTBrick)

#define PYBRICKS_HUB_NXTBRICK                   (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (0)
#define PYBRICKS_PY_COMMON_CHARGER              (0)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT          (1)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (0)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS   (4)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (0)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_MOTOR_MODEL          (1)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (1)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (0)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (0)
#define PYBRICKS_PY_MEDIA                       (0)
#define PYBRICKS_PY_NXTDEVICES                  (0)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PARAMETERS_IMAGE            (0)
#define PYBRICKS_PY_DEVICES                     (1)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO     (0)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (0)
#define PYBRICKS_PY_TOOLS                       (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU              (0)
#define PYBRICKS_PY_TOOLS_APP_DATA              (0)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_USE_STACK_END_AS_TOP       (1)
#define PYBRICKS_OPT_RAW_REPL                   (1)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_LEVEL1               (1)
#define PYBRICKS_OPT_EXTRA_LEVEL2               (0)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (1)
#define PYBRICKS_OPT_NATIVE_MOD                 (0)

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

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

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

#define MP_STATE_PORT MP_STATE_VM
