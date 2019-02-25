// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/port.h>

#include "py/obj.h"

// Class structure for Motors
typedef struct _motor_Motor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
    bool encoded;
} motor_Motor_obj_t;

static inline pbio_port_t get_port(mp_obj_t self_in) {
    return ((motor_Motor_obj_t*)MP_OBJ_TO_PTR(self_in))->port;
}

const mp_obj_type_t motor_Motor_type;

// Motor enum types
const mp_obj_type_t motor_Stop_enum;
const mp_obj_type_t motor_Run_enum;
const mp_obj_type_t motor_Dir_enum;
