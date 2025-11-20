// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

#include "stm32f413xx.h"

#define MICROPY_HW_BOARD_NAME                   "SPIKE Essential Hub"
#define MICROPY_HW_MCU_NAME                     "STM32F413RG"

#define PYBRICKS_HUB_NAME                       "essentialhub"
#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_EssentialHub)
#define PYBRICKS_HUB_ESSENTIALHUB               (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (1)
#define PYBRICKS_PY_COMMON_CHARGER              (1)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT          (1)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (1)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS   (1)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_MOTOR_MODEL          (1)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (0)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (1)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (1)
#define PYBRICKS_PY_IODEVICES_ANALOG_SENSOR     (0)
#define PYBRICKS_PY_IODEVICES_DC_MOTOR          (0)
#define PYBRICKS_PY_IODEVICES_I2C_DEVICE        (0)
#define PYBRICKS_PY_IODEVICES_LUMP_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_LWP3_DEVICE       (1)
#define PYBRICKS_PY_IODEVICES_PUP_DEVICE        (1)
#define PYBRICKS_PY_IODEVICES_UART_DEVICE       (1)
#define PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER   (1)
#define PYBRICKS_PY_NXTDEVICES                  (0)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PARAMETERS_IMAGE            (0)
#define PYBRICKS_PY_DEVICES                     (1)
#define PYBRICKS_PY_PUPDEVICES                  (1)
#define PYBRICKS_PY_PUPDEVICES_REMOTE           (1)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO     (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (1)
#define PYBRICKS_PY_TOOLS                       (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU              (0)
#define PYBRICKS_PY_TOOLS_APP_DATA              (1)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_USE_STACK_END_AS_TOP       (1)
#define PYBRICKS_OPT_RAW_REPL                   (0)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_LEVEL1               (1)
#define PYBRICKS_OPT_EXTRA_LEVEL2               (1)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (1)
#define PYBRICKS_OPT_NATIVE_MOD                 (0)

#include "../_common/mpconfigport.h"
