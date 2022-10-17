// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <stdint.h>

#define MICROPY_BANNER_NAME_AND_VERSION         "Pybricks MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE
#define MICROPY_HW_BOARD_NAME                   "LEGO MINDSTORMS NXT Brick"
#define MICROPY_HW_MCU_NAME                     "AT91SAM7S256"

#define PYBRICKS_HUB_NAME                       "nxt"
#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_NXTBrick)

#define PYBRICKS_HUB_NXTBRICK                   (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_CHARGER              (0)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (0)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (0)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (0)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (0)
#define PYBRICKS_PY_GEOMETRY                    (0)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (0)
#define PYBRICKS_PY_MEDIA                       (0)
#define PYBRICKS_PY_MEDIA_EV3DEV                (0)
#define PYBRICKS_PY_NXTDEVICES                  (1)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PUPDEVICES                  (0)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (0)
#define PYBRICKS_PY_TOOLS                       (1)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_MOD                  (1)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (1)

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

#include <base/interrupts.h>
#include <base/drivers/systick.h>

#define MICROPY_BEGIN_ATOMIC_SECTION()     nx_interrupts_disable()
#define MICROPY_END_ATOMIC_SECTION(state)  if (state) { nx_interrupts_enable(); }

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
    } while (0);

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    mp_obj_dict_t *pb_type_Color_dict; \
    const char *readline_hist[8];
