#include "modmotor.h"

//
// DcMotor class methods
//

mp_obj_t motor_DcMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_DcMotor_obj_t *self = m_new_obj(motor_DcMotor_obj_t);
    // give it a type
    self->base.type = &motor_DcMotor_type;
    // set the member number with the first argument of the constructor
    self->port = mp_obj_get_int(args[0]); // TODO: Check valid range using mp_arg_parse_all
    // set the inverted member if given
    self->inverted = (n_args == 2) ? mp_obj_is_true(args[1]) : false;
    // Apply settings to the motor
    pbio_motor_set_direction(self->port, self->inverted);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void motor_DcMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind ) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Print port
    printf("Port: %c\n", self->port);
    // Print inverted (yes or no)
    printf("Inverted: ");
    if (self->inverted) {
        printf("yes");
    }
    else {
        printf("no");
    }
}

STATIC mp_obj_t motor_DcMotor_coast(mp_obj_t self_in) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_coast(self->port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DcMotor_coast_obj, motor_DcMotor_coast);

STATIC mp_obj_t motor_DcMotor_brake(mp_obj_t self_in) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_set_duty_cycle(self->port, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DcMotor_brake_obj, motor_DcMotor_brake);

STATIC mp_obj_t motor_DcMotor_duty(mp_obj_t self_in, mp_obj_t duty) {
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_motor_set_duty_cycle(self->port, (int) mp_obj_get_float(duty));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_DcMotor_duty_obj, motor_DcMotor_duty);

//
// DcMotor class tables
//

// creating the table of members
STATIC const mp_rom_map_elem_t motor_DcMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DcMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DcMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DcMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_DcMotor_locals_dict, motor_DcMotor_locals_dict_table);

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

//
// EncodedMotor class methods
//

mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    // check the number of argument
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    // create a new C-struct type object
    motor_EncodedMotor_obj_t *self = m_new_obj(motor_EncodedMotor_obj_t);
    // give it a type
    self->base.type = &motor_EncodedMotor_type;
    // set the member number with the first argument of the constructor
    self->port = mp_obj_get_int(args[0]); // TODO: Check valid range using mp_arg_parse_all
    // set the inverted member if given
    self->inverted = (n_args == 2) ? mp_obj_is_true(args[1]) : false;
    // Apply settings to the motor
    pbio_motor_set_direction(self->port, self->inverted);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
    // get a pointer to self
    motor_DcMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // print the DC attributes
    motor_DcMotor_print(print, self_in, kind);
    // Print other attributes
    printf("\nOther stuff\n");
    printf("Port: %c", self->port);
}

//
// EncodedMotor class tables
//

STATIC const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DcMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DcMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DcMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict, motor_EncodedMotor_locals_dict_table);

const mp_obj_type_t motor_EncodedMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_EncodedMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
     //Inherit from DcMotor
    .parent = &motor_DcMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

//
// Motor Module tables: put the above classes in a module
//

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR__motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DcMotor), (mp_obj_t)&motor_DcMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncodedMotor), (mp_obj_t)&motor_EncodedMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_motor_globals, motor_globals_table);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};
