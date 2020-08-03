// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "stm32f070xb.h"

#define MICROPY_HW_BOARD_NAME           "BOOST Move Hub"
#define MICROPY_HW_MCU_NAME             "STM32F070RB"

#define PYBRICKS_HEAP_KB                8 // half of RAM

#define PYBRICKS_HUB_MOVEHUB            (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_ROBOTICS            (1)

// Pybricks STM32 options
#define PYBRICKS_STM32_OPT_COMPILER     (0)
#define PYBRICKS_STM32_OPT_FLOAT        (0)
#define PYBRICKS_STM32_OPT_TERSE_ERR    (1)
#define PYBRICKS_STM32_OPT_EXTRA_MOD    (0)

#include "../stm32/configport.h"
