#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);

// Class structure for DC Motors
typedef struct _motor_DCMotor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
} motor_DCMotor_obj_t;

// Class structure for Encoded Motors
typedef struct _motor_EncodedMotor_obj_t {
    mp_obj_base_t base;
    uint8_t port;
} motor_EncodedMotor_obj_t;

extern const mp_obj_type_t motor_DCMotor_type;
extern const mp_obj_type_t motor_EncodedMotor_type;
