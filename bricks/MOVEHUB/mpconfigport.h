// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "stm32f070xb.h"

#define MICROPY_HW_BOARD_NAME           "BOOST Move Hub"
#define MICROPY_HW_MCU_NAME             "STM32F070RB"

#define PYBRICKS_HEAP_KB                8 // half of RAM

// Pybricks modules
#define PYBRICKS_PY_BATTERY             (1)
#define PYBRICKS_PY_IODEVICE            (1)
#define PYBRICKS_PY_MOTOR               (1)
#define PYBRICKS_PY_MOVEHUB             (1)
#define PYBRICKS_PY_PUPDEVICES          (1)

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
// Requires about 19K (19568) of flash
#define MICROPY_ENABLE_COMPILER         (1)

// Set to MICROPY_FLOAT_IMPL_FLOAT to enable floating point support in user code or
// set to MICROPY_FLOAT_IMPL_NONE to disable floating point support in user code
// Requires about 20K (21312) of flash
#define MICROPY_FLOAT_IMPL              (MICROPY_FLOAT_IMPL_NONE)

// Set to (1) to include the advanced module or (0) to exclude it. Requires about ... bytes of flash
// This module includes the IODevice class for setting device modes and reading/writing raw data
#define PYBRICKS_MODULE_ADVANCED        (1)

extern const struct _mp_obj_module_t pb_module_movehub;
extern const struct _mp_obj_module_t pb_module_pupdevices;
extern const struct _mp_obj_module_t pb_module_parameters;
extern const struct _mp_obj_module_t pb_module_tools;

#if PYBRICKS_MODULE_ADVANCED
extern const struct _mp_obj_module_t pb_module_advanced;
#define PYBRICKS_MODULE_ADVANCED_DEF { MP_OBJ_NEW_QSTR(MP_QSTR_advanced),    (mp_obj_t)&pb_module_advanced },
#else
#define PYBRICKS_MODULE_ADVANCED_DEF
#endif

#define PYBRICKS_PORT_BUILTIN_MODULES \
    PYBRICKS_MODULE_ADVANCED_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_movehub),     (mp_obj_t)&pb_module_movehub },    \
    { MP_OBJ_NEW_QSTR(MP_QSTR_devices),     (mp_obj_t)&pb_module_pupdevices },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR_parameters),  (mp_obj_t)&pb_module_parameters },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR_tools),       (mp_obj_t)&pb_module_tools     },

#include "../stm32configport.h"
