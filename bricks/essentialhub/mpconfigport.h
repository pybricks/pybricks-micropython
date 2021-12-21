// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include "stm32f413xx.h"

#define MICROPY_HW_BOARD_NAME           "SPIKE Essential Hub"
#define MICROPY_HW_MCU_NAME             "STM32F413RG"

#define PYBRICKS_HUB_CLASS_NAME         (MP_QSTR_EssentialHub)
#undef PYBRICKS_HUB_CLASS_NAME_ALIAS

#define PYBRICKS_HEAP_KB                256

#define PYBRICKS_HUB_ESSENTIALHUB       (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_CHARGER      (1)
#define PYBRICKS_PY_COMMON_IMU          (1)
#define PYBRICKS_PY_COMMON_KEYPAD       (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX (0)
#define PYBRICKS_PY_COMMON_LOGGER       (1)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_COMMON_SPEAKER      (0)
#define PYBRICKS_PY_COMMON_SYSTEM       (1)
#define PYBRICKS_PY_EV3DEVICES          (0)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_GEOMETRY            (1)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_MEDIA               (0)
#define PYBRICKS_PY_MEDIA_EV3DEV        (0)
#define PYBRICKS_PY_NXTDEVICES          (0)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (1)
#define PYBRICKS_PY_PARAMETERS_ICON     (0)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_TOOLS               (1)

// Pybricks STM32 options
#define PYBRICKS_STM32_OPT_COMPILER     (1)
#define PYBRICKS_STM32_OPT_FLOAT        (1)
#define PYBRICKS_STM32_OPT_TERSE_ERR    (0)
#define PYBRICKS_STM32_OPT_EXTRA_MOD    (1)

#include "../stm32/configport.h"
