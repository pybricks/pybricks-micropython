// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "stm32f446xx.h"

#define MICROPY_HW_BOARD_NAME           "NUCLEO-F446ZE"
#define MICROPY_HW_MCU_NAME             "STM32F446ZE"

#define PYBRICKS_HEAP_KB                64 // half of RAM

// Pybricks modules
#define PYBRICKS_PY_ADVANCED            (1)
#define PYBRICKS_PY_BATTERY             (0)
#define PYBRICKS_PY_DEBUG               (1)
#define PYBRICKS_PY_MOTOR               (0)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (0)
#define PYBRICKS_PY_TOOLS               (0)

#include "../stm32configport.h"
