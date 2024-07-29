// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors


#define MICROPY_HW_BOARD_NAME                   "MINDSTORMS EV3 Brick"
#define MICROPY_HW_MCU_NAME                     "TI Sitara AM1808"

#define PYBRICKS_HUB_NAME                       "ev3"
#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_EV3Brick)
#define PYBRICKS_HUB_EV3BRICK                   (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (0)
#define PYBRICKS_PY_COMMON_CHARGER              (0)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT          (1)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (0)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS   (6)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (0)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_MOTOR_MODEL          (0)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (1)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (0)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (0)
#define PYBRICKS_PY_MEDIA                       (0)
#define PYBRICKS_PY_MEDIA_EV3DEV                (0)
#define PYBRICKS_PY_NXTDEVICES                  (1)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_DEVICES                     (1)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO     (0)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (0)
#define PYBRICKS_PY_TOOLS                       (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU              (0)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_FLOAT                      (0)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_MOD                  (1)
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

// We have inlined IRQ functions for efficiency (they are generally
// 1 machine instruction).
//
// Note on IRQ state: you should not need to know the specific
// value of the state variable, but rather just pass the return
// value from disable_irq back to enable_irq.  If you really need
// to know the machine-specific values, see irq.h.

#include <arm920t.h>

static inline void enable_irq(mp_uint_t state) {
    arm_intr_enable();
}

static inline mp_uint_t disable_irq(void) {
    arm_intr_disable();
    return 0;
}

#define MICROPY_BEGIN_ATOMIC_SECTION()     disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  enable_irq(state)

#define MICROPY_VM_HOOK_LOOP \
    do { \
        extern int pbio_do_one_event(void); \
        pbio_do_one_event(); \
    } while (0);

#define MICROPY_GC_HOOK_LOOP(i) do { \
        if ((i & 0xf) == 0) { \
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
