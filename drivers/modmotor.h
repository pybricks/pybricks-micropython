#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/motorcontrol.h>
#include <pbio/motor.h>

extern const mp_obj_type_t motor_DcMotor_type;
extern const mp_obj_type_t motor_EncodedMotor_type;