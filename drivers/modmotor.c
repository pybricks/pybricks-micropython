#include "modmotor.h"

//
// DCMotor class methods
//

mp_obj_t motor_DCMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_DCMotor_obj_t *self = m_new_obj(motor_DCMotor_obj_t);
    // give it a type
    self->base.type = &motor_DCMotor_type;
    // set the member number with the first argument of the constructor
    self->port = mp_obj_get_int(args[0]);
    // set the direction member if given
    self->direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    // Apply settings to the motor
    pbio_motor_set_constant_settings(self->port, self->direction);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void motor_DCMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind ) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Print port
    printf("Port: %c\n", self->port);
    // Print direction
    printf("Direction: ");
    if (self->direction == PBIO_MOTOR_DIR_NORMAL) {
        printf("Normal");
    }
    else {
        printf("Inverted");
    }
}

STATIC mp_obj_t motor_DCMotor_coast(mp_obj_t self_in) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_coast(self->port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DCMotor_coast_obj, motor_DCMotor_coast);

STATIC mp_obj_t motor_DCMotor_brake(mp_obj_t self_in) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_set_duty_cycle(self->port, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DCMotor_brake_obj, motor_DCMotor_brake);

STATIC mp_obj_t motor_DCMotor_duty(mp_obj_t self_in, mp_obj_t duty) {    
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_motor_set_duty_cycle(self->port, (int) mp_obj_get_float(duty));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_DCMotor_duty_obj, motor_DCMotor_duty);

//
// DCMotor class tables
//

// creating the table of members
STATIC const mp_rom_map_elem_t motor_DCMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_DCMotor_locals_dict, motor_DCMotor_locals_dict_table);

// create the class-object itself
const mp_obj_type_t motor_DCMotor_type = {
    // "inherit" the type "type"
    { &mp_type_type },
     // class name
    .name = MP_QSTR_DCMotor,
     // give it a print-function
    .print = motor_DCMotor_print,
     // constructor
    .make_new = motor_DCMotor_make_new,
     // class member table
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
};

//
// EncodedMotor class methods
//

mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 3, false);
    motor_EncodedMotor_obj_t *self = m_new_obj(motor_EncodedMotor_obj_t);
    self->base.type = &motor_EncodedMotor_type;
    // Set the port
    self->port = mp_obj_get_int(args[0]);
    // Set the direction member if given
    self->direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    // Apply settings to the motor
    pbio_motor_set_constant_settings(self->port, self->direction);    
    // Set the gear ratio
    self->gear_ratio = (n_args >= 3) ? mp_obj_get_float(args[2]): 1.0;
    return MP_OBJ_FROM_PTR(self);
}

STATIC void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
    // get a pointer to self
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // print the DCMotor attributes
    motor_DCMotor_print(print, self_in, kind);
    // Print other attributes
    printf("\nGear ratio: %f", self->gear_ratio);
}

//
// EncodedMotor class tables
//

STATIC const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict, motor_EncodedMotor_locals_dict_table);

const mp_obj_type_t motor_EncodedMotor_type = {
    { &mp_type_type },
    // { &motor_DCMotor_type }, // or this?
    .name = MP_QSTR_EncodedMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .parent = &motor_DCMotor_type, //Inherit from DCMotor (?)
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

//
// Motor Module tables: put the above classes in a module
//

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR__motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DCMotor), (mp_obj_t)&motor_DCMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncodedMotor), (mp_obj_t)&motor_EncodedMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_motor_globals, motor_globals_table);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};
