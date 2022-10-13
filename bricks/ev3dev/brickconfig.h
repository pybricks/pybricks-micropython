// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbdrv/config.h>
#include "pbinit.h"

#define MICROPY_HW_BOARD_NAME             "LEGO MINDSTORMS EV3 Intelligent Brick"
#define MICROPY_HW_MCU_NAME               "Texas Instruments AM1808"

#define PYBRICKS_HUB_NAME               "ev3"
#define PYBRICKS_HUB_CLASS_NAME         (MP_QSTR_EV3Brick)
#define PYBRICKS_HUB_EV3BRICK           (1)

// In this port, Pybricks runs on top of ev3dev.
#define PYBRICKS_RUNS_ON_EV3DEV         (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_CHARGER      (0)
#define PYBRICKS_PY_COMMON_CONTROL      (1)
#define PYBRICKS_PY_COMMON_IMU          (0)
#define PYBRICKS_PY_COMMON_KEYPAD       (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX (0)
#define PYBRICKS_PY_COMMON_LOGGER       (1)
#define PYBRICKS_PY_COMMON_LOGGER_REAL_FILE (1)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_COMMON_SPEAKER      (0)
#define PYBRICKS_PY_COMMON_SYSTEM       (1)
#define PYBRICKS_PY_EV3DEVICES          (1)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_GEOMETRY            (1)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_MEDIA               (0)
#define PYBRICKS_PY_MEDIA_EV3DEV        (1)
#define PYBRICKS_PY_NXTDEVICES          (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (1)
#define PYBRICKS_PY_PARAMETERS_ICON     (0)
#define PYBRICKS_PY_PUPDEVICES          (0)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE (0)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_USIGNAL             (1)
