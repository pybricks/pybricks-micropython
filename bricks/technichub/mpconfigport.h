// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "stm32l431xx.h"

#define MICROPY_HW_BOARD_NAME           "LEGO Technic Hub"
#define MICROPY_HW_MCU_NAME             "STM32L431RC"

#define PYBRICKS_HEAP_KB                48

#define PYBRICKS_HUB_TECHNICHUB         (1)

// Pybricks modules
#define PYBRICKS_PY_GEOMETRY            (1)
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX   (0)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (0)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_ROBOTICS            (1)

// Pybricks STM32 options
#define PYBRICKS_STM32_OPT_COMPILER     (1)
#define PYBRICKS_STM32_OPT_FLOAT        (1)
#define PYBRICKS_STM32_OPT_TERSE_ERR    (0)
#define PYBRICKS_STM32_OPT_EXTRA_MOD    (1)

#include "../stm32/configport.h"
