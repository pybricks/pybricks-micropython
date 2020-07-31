// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include "stm32f446xx.h"

#define MICROPY_HW_BOARD_NAME           "NUCLEO-F446ZE"
#define MICROPY_HW_MCU_NAME             "STM32F446ZE"

#define PYBRICKS_HEAP_KB                64 // half of RAM

#define PYBRICKS_HUB_DEBUG              (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (0)
#define PYBRICKS_PY_COMMON_MOTORS       (0)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (0)
#define PYBRICKS_PY_TOOLS               (0)

#include "../stm32/configport.h"
