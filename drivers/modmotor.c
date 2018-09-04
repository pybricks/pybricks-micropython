#include "modmotor.h"
int global_test;

/*-----------------DcMotor Class----------------------*/


//
// DcMotor Class: new object and self print function
//

// C-structure for object
typedef struct _motor_DcMotor_obj_t {
    mp_obj_base_t base;
    // Port
    uint8_t DcMotor_port;
    // Reverse
    bool DcMotor_clockwise_positive;
} motor_DcMotor_obj_t;

STATIC void motor_DcMotor_print( const mp_print_t *print,
                                  mp_obj_t self_in,
                                  mp_print_kind_t kind ) {
    // get a pointer to self
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // print the attributes
    printf("Port: %c\n", self->DcMotor_port+65);
    printf("Positive speed means: ");
    if(!self->DcMotor_clockwise_positive){
        printf("counter");
    }
    printf("clockwise");
    
}

mp_obj_t motor_DcMotor_make_new( const mp_obj_type_t *type,
                                  size_t n_args,
                                  size_t n_kw,
                                  const mp_obj_t *args ) {
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_DcMotor_obj_t *self = m_new_obj(motor_DcMotor_obj_t);
    // give it a type
    self->base.type = &motor_DcMotor_type;
    // set the member number with the first argument of the constructor
    self->DcMotor_port = mp_obj_get_int(args[0]); // TODO: Check valid range using mp_arg_parse_all
    // set the clockwise_positive member if given
    self->DcMotor_clockwise_positive = (n_args == 2) ? mp_obj_is_true(args[1]) : true;
    // Apply settings to the motor
    pbio_motor_set_direction(self->DcMotor_port, self->DcMotor_clockwise_positive);
    return MP_OBJ_FROM_PTR(self);
}

//
// DcMotor Class: Methods
//

STATIC mp_obj_t motor_DcMotor_coast(mp_obj_t self_in) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_coast(self->DcMotor_port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DcMotor_coast_obj,
                          motor_DcMotor_coast);

STATIC mp_obj_t motor_DcMotor_brake(mp_obj_t self_in) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_set_duty_cycle(self->DcMotor_port, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DcMotor_brake_obj,
                          motor_DcMotor_brake);


STATIC mp_obj_t motor_DcMotor_duty(mp_obj_t self_in, mp_obj_t duty) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_motor_set_duty_cycle(self->DcMotor_port, (int) mp_obj_get_float(duty));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_DcMotor_duty_obj,
                          motor_DcMotor_duty);


//
// DcMotor Class: Tables
//

// creating the table of members
STATIC const mp_rom_map_elem_t motor_DcMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DcMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DcMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DcMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_DcMotor_locals_dict,
                            motor_DcMotor_locals_dict_table);

// create the class-object itself
const mp_obj_type_t motor_DcMotor_type = {
    // "inherit" the type "type"
    { &mp_type_type },
     // class name
    .name = MP_QSTR_DcMotor,
     // give it a print-function
    .print = motor_DcMotor_print,
     // constructor
    .make_new = motor_DcMotor_make_new,
     // class member table
    .locals_dict = (mp_obj_dict_t*)&motor_DcMotor_locals_dict,
};


/*--------------EncodedMotor Class--------------------*/



//
// EncodedMotor Class: new object and self print function
//

// C-structure for object
typedef struct _motor_EncodedMotor_obj_t {
    mp_obj_base_t base;
    // Port
    uint8_t EncodedMotor_port;
    // Reverse
    bool EncodedMotor_clockwise_positive;
} motor_EncodedMotor_obj_t;

mp_obj_t motor_EncodedMotor_make_new( const mp_obj_type_t *type,
                                  size_t n_args,
                                  size_t n_kw,
                                  const mp_obj_t *args ) {
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_EncodedMotor_obj_t *self = m_new_obj(motor_EncodedMotor_obj_t);
    // give it a type
    self->base.type = &motor_EncodedMotor_type;
    // set the member number with the first argument of the constructor
    self->EncodedMotor_port = mp_obj_get_int(args[0]); // TODO: Check valid range using mp_arg_parse_all
    // set the clockwise_positive member if given
    self->EncodedMotor_clockwise_positive = (n_args == 2) ? mp_obj_is_true(args[1]) : true;
    // Apply settings to the motor
    pbio_motor_set_direction(self->EncodedMotor_port, self->EncodedMotor_clockwise_positive);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void motor_EncodedMotor_print( const mp_print_t *print,
                                  mp_obj_t self_in,
                                  mp_print_kind_t kind ) {
    // get a pointer to self
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // print the DC attributes
    motor_DcMotor_print(print, self_in, kind);
    // Print other attributes
    printf("\nOther stuff\n");
    printf("Port: %c", self->DcMotor_port+65);
}

//
// EncodedMotor Class: Tables and  class object
//

// creating the table of members
STATIC const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DcMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DcMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DcMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict,
                            motor_EncodedMotor_locals_dict_table);

const mp_obj_type_t motor_EncodedMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_EncodedMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
     //Inherit from DcMotor
    .parent = &motor_DcMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

/*-----------------Motor Module-----------------------*/

//
// Motor Module tables
//

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DcMotor), (mp_obj_t)&motor_DcMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncodedMotor), (mp_obj_t)&motor_EncodedMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_motor_globals,
    motor_globals_table
);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};


