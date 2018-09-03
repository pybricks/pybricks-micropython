#include "modmotor.h"
int global_test;

//
// Hello Class: new object and self print function
//

// C-structure for object
typedef struct _motor_EncoderlessMotor_obj_t {
    mp_obj_base_t base;
    // Port
    uint8_t EncoderlessMotor_port;
    // Reverse
    bool EncoderlessMotor_clockwise_positive;
} motor_EncoderlessMotor_obj_t;

STATIC void motor_EncoderlessMotor_print( const mp_print_t *print,
                                  mp_obj_t self_in,
                                  mp_print_kind_t kind ) {
    // get a pointer to self
    motor_EncoderlessMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // print the attributes
    printf("Port: %c\n", self->EncoderlessMotor_port+65);
    printf("Positive speed means: ");
    if(!self->EncoderlessMotor_clockwise_positive){
        printf("counter");
    }
    printf("clockwise\n");
    
}

extern const mp_obj_type_t motor_EncoderlessMotor_type;

mp_obj_t motor_EncoderlessMotor_make_new( const mp_obj_type_t *type,
                                  size_t n_args,
                                  size_t n_kw,
                                  const mp_obj_t *args ) {
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_EncoderlessMotor_obj_t *self = m_new_obj(motor_EncoderlessMotor_obj_t);
    // give it a type
    self->base.type = &motor_EncoderlessMotor_type;
    // set the member number with the first argument of the constructor
    self->EncoderlessMotor_port = mp_obj_get_int(args[0]); // TODO: Check valid range using mp_arg_parse_all
    // set the clockwise_positive member if given
    self->EncoderlessMotor_clockwise_positive = (n_args == 2) ? mp_obj_is_true(args[1]) : true;
    // Apply settings to the motor
    set_positive_direction(self->EncoderlessMotor_port, self->EncoderlessMotor_clockwise_positive);
    return MP_OBJ_FROM_PTR(self);
}

//
// Hello Class: Methods
//

STATIC mp_obj_t motor_EncoderlessMotor_coast(mp_obj_t self_in) {
    motor_EncoderlessMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    set_coast(self->EncoderlessMotor_port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_EncoderlessMotor_coast_obj,
                          motor_EncoderlessMotor_coast);


STATIC mp_obj_t motor_EncoderlessMotor_duty(mp_obj_t self_in, mp_obj_t duty) {
    motor_EncoderlessMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    set_duty(self->EncoderlessMotor_port, (int) mp_obj_get_float(duty));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_EncoderlessMotor_duty_obj,
                          motor_EncoderlessMotor_duty);


//
// Hello Class: Tables
//

// creating the table of members
STATIC const mp_rom_map_elem_t motor_EncoderlessMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_EncoderlessMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_EncoderlessMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_EncoderlessMotor_locals_dict,
                            motor_EncoderlessMotor_locals_dict_table);

// create the class-object itself
const mp_obj_type_t motor_EncoderlessMotor_type = {
    // "inherit" the type "type"
    { &mp_type_type },
     // class name
    .name = MP_QSTR_EncoderlessMotor,
     // give it a print-function
    .print = motor_EncoderlessMotor_print,
     // constructor
    .make_new = motor_EncoderlessMotor_make_new,
     // class member table
    .locals_dict = (mp_obj_dict_t*)&motor_EncoderlessMotor_locals_dict,
};


//
// Motor Module tables
//

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncoderlessMotor), (mp_obj_t)&motor_EncoderlessMotor_type},    
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_motor_globals,
    motor_globals_table
);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};