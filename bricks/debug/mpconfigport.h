// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "stm32f446xx.h"

#define MICROPY_HW_BOARD_NAME           "NUCLEO-F446RE"
#define MICROPY_HW_MCU_NAME             "STM32F446xx"

#define PYBRICKS_HEAP_KB                64 // half of RAM

// Pybricks modules
#define PYBRICKS_PY_MOTOR               (0)

#define PYBRICKS_PORT_BUILTIN_MODULES

#include "../stm32configport.h"
