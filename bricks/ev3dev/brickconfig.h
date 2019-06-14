// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "pbinit.h"

#define MICROPY_HW_BOARD_NAME             "LEGO MINDSTORMS EV3 Intelligent Brick"
#define MICROPY_HW_MCU_NAME               "Texas Instruments AM1808"

// Pybricks modules
#define PYBRICKS_PY_BATTERY             (1)
#define PYBRICKS_PY_BUTTONS             (1)
#define PYBRICKS_PY_MOTOR               (1)
#define PYBRICKS_PY_PUPDEVICES          (1)

// Enable (1) or disable (0) keyword argument support in Pybricks modules
#define PYBRICKS_ENABLE_KWARGS          (1)

#define MICROPY_PORT_INIT_FUNC pybricks_init()
#define MICROPY_PORT_DEINIT_FUNC pybricks_deinit()

#define MICROPY_PY_SYS_PATH_DEFAULT (":~/.micropython/lib:/usr/lib/micropython")

extern const struct _mp_obj_module_t pb_module_ev3brick;
extern const struct _mp_obj_module_t pb_module_ev3devices;
extern const struct _mp_obj_module_t pb_module_robotics;
extern const struct _mp_obj_module_t pb_module_parameters;
extern const struct _mp_obj_module_t pb_module_tools;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_ev3devices_c), MP_ROM_PTR(&pb_module_ev3devices) }, \
    { MP_ROM_QSTR(MP_QSTR_ev3brick_c),   MP_ROM_PTR(&pb_module_ev3brick)   }, \
    { MP_ROM_QSTR(MP_QSTR_robotics_c),   MP_ROM_PTR(&pb_module_robotics)   }, \
    { MP_ROM_QSTR(MP_QSTR_parameters_c), MP_ROM_PTR(&pb_module_parameters) }, \
    { MP_ROM_QSTR(MP_QSTR_tools),        MP_ROM_PTR(&pb_module_tools)      },
