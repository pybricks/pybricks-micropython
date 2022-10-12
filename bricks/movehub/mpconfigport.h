// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "stm32f070xb.h"

#define MICROPY_HW_BOARD_NAME           "BOOST Move Hub"
#define MICROPY_HW_MCU_NAME             "STM32F070RB"

#define PYBRICKS_HUB_CLASS_NAME         (MP_QSTR_MoveHub)

#define PYBRICKS_HUB_MOVEHUB            (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_CHARGER      (0)
#define PYBRICKS_PY_COMMON_CONTROL      (0)
#define PYBRICKS_PY_COMMON_IMU          (0)
#define PYBRICKS_PY_COMMON_KEYPAD       (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX (0)
#define PYBRICKS_PY_COMMON_LOGGER       (0)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_COMMON_SYSTEM       (1)
#define PYBRICKS_PY_EXPERIMENTAL        (0)
#define PYBRICKS_PY_GEOMETRY            (0)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (0)
#define PYBRICKS_PY_MEDIA               (0)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (1)
#define PYBRICKS_PY_PARAMETERS_ICON     (0)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE (0)
#define PYBRICKS_PY_TOOLS               (1)

// Pybricks STM32 options
#define PYBRICKS_STM32_OPT_COMPILER     (0)
#define PYBRICKS_STM32_OPT_FLOAT        (0)
#define PYBRICKS_STM32_OPT_TERSE_ERR    (1)
#define PYBRICKS_STM32_OPT_EXTRA_MOD    (0)

#include "../common_stm32/mpconfigport.h"
