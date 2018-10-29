#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/motorcontrol.h>
#include <pberror.h>
#include <objenum.h>

// Class structure for Motors
typedef struct _motor_Motor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
    bool encoded;
} motor_Motor_obj_t;

static inline pbio_port_t get_port(mp_obj_t self_in) {
    return ((motor_Motor_obj_t*)MP_OBJ_TO_PTR(self_in))->port;
}

const mp_obj_dict_t motor_DCMotor_locals_dict;
const mp_obj_dict_t motor_EncodedMotor_locals_dict;
mp_obj_t motor_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
void motor_Motor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);

// Motor enum types
const mp_obj_type_t motor_Stop_enum;
const mp_obj_type_t motor_Wait_enum;
const mp_obj_type_t motor_Dir_enum;
