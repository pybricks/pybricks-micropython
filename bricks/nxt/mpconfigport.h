// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#define MICROPY_HW_BOARD_NAME                   "LEGO MINDSTORMS NXT Brick"
#define MICROPY_HW_MCU_NAME                     "AT91SAM7S256"

#define PYBRICKS_HUB_NAME                       "nxt"
#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_NXTBrick)

#define PYBRICKS_HUB_NXTBRICK                   (1)
// On EV3, allow PUP imports for easy beta testing
#define PYBRICKS_PY_NXT_PUP_ALIAS               (1)
#define PYBRICKS_HUB_CLASS_NAME_ALIAS           (MP_QSTR_PrimeHub)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (0)
#define PYBRICKS_PY_COMMON_CHARGER              (0)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT          (1)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (0)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS   (4)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (0)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_MOTOR_MODEL          (1)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (1)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (0)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (1)
#define PYBRICKS_PY_IODEVICES_ANALOG_SENSOR     (1)
#define PYBRICKS_PY_IODEVICES_DC_MOTOR          (1)
#define PYBRICKS_PY_IODEVICES_I2C_DEVICE        (1)
#define PYBRICKS_PY_IODEVICES_LUMP_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_LWP3_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_PUP_DEVICE        (0)
#define PYBRICKS_PY_IODEVICES_UART_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER   (0)
#define PYBRICKS_PY_NXTDEVICES                  (1)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PARAMETERS_IMAGE            (1)
#define PYBRICKS_PY_PARAMETERS_IMAGE_FILE       (0)
#define PYBRICKS_PY_DEVICES                     (1)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO     (0)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (0)
#define PYBRICKS_PY_TOOLS                       (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU              (0)
#define PYBRICKS_PY_TOOLS_APP_DATA              (0)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_USE_STACK_END_AS_TOP       (1)
#define PYBRICKS_OPT_RAW_REPL                   (1)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_LEVEL1               (1)
#define PYBRICKS_OPT_EXTRA_LEVEL2               (0)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (1)
#define PYBRICKS_OPT_NATIVE_MOD                 (0)

#include "../_common/mpconfigport.h"
