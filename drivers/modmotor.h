#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/builtin.h"
#include <stdio.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

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

// Allow other modules to import this motor
extern const mp_obj_type_t motor_MovehubMotor_type;
