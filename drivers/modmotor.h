#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

#define PB_DEFINE_ID_ATTR(motor_attr_handler, device_id) \
void motor_attr_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {\
    if (dest[0] == MP_OBJ_NULL && attr == MP_QSTR_device_id) {\
        mp_obj_t val = MP_OBJ_NEW_SMALL_INT(device_id);\
        dest[0] = val;\
    }\
}

// Allow other modules to import this motor
extern const mp_obj_type_t motor_MovehubMotor_type;
