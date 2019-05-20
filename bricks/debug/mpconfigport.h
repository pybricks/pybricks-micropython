// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "stm32f446xx.h"

#define MICROPY_HW_BOARD_NAME           "NUCLEO-F446ZE"
#define MICROPY_HW_MCU_NAME             "STM32F446ZE"

#define PYBRICKS_HEAP_KB                64 // half of RAM

// Pybricks modules
#define PYBRICKS_PY_DEBUG               (1)
#define PYBRICKS_PY_MOTOR               (0)

extern const struct _mp_obj_module_t pb_module_debug;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_debug),     (mp_obj_t)&pb_module_debug },     \

#include "../stm32configport.h"
