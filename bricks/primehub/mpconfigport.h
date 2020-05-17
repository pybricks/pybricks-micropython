// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include "stm32f413xx.h"

#define MICROPY_HW_BOARD_NAME           "SPIKE Prime Hub"
#define MICROPY_HW_MCU_NAME             "STM32F413VG"

#define PYBRICKS_HEAP_KB                64 // half of RAM

#define PYBRICKS_HUB_PRIMEHUB           (1)

// Pybricks modules
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_ROBOTICS            (1)

#include "../stm32/configport.h"
