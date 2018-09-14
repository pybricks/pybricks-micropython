#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/motorcontrol.h>
#include <pberror.h>

// Extend _mp_obj_type_t with a device_id field.
typedef struct _mp_obj_type_id_t mp_obj_type_id_t;
typedef mp_obj_t (*mp_make_new_fun_id_t)(const mp_obj_type_id_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
struct _mp_obj_type_id_t {
    mp_obj_base_t base;
    uint16_t flags;
    uint16_t name;
    mp_print_fun_t print;
    mp_make_new_fun_id_t make_new; // Modified type compared to MicroPython
    mp_call_fun_t call;
    mp_unary_op_fun_t unary_op;
    mp_binary_op_fun_t binary_op;
    mp_attr_fun_t attr;
    mp_subscr_fun_t subscr;
    mp_getiter_fun_t getiter;
    mp_fun_1_t iternext;
    mp_buffer_p_t buffer_p;
    const void *protocol;
    const void *parent;
    struct _mp_obj_dict_t *locals_dict;
    // The following was added
    pbio_id_t device_id;
};

const mp_obj_type_id_t motor_DCMotor_type;
const mp_obj_dict_t motor_DCMotor_locals_dict;
mp_obj_t motor_DCMotor_make_new(const mp_obj_type_id_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
void motor_DCMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);

const mp_obj_type_id_t motor_EncodedMotor_type;
const mp_obj_dict_t motor_EncodedMotor_locals_dict;
mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_id_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);
