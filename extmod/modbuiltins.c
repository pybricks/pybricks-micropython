// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/control.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"
#include "py/mphal.h"

#include "modbuiltins.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Control class object structure
typedef struct _builtins_Control_obj_t {
    mp_obj_base_t base;
    pbio_control_t *control;
} builtins_Control_obj_t;

// pybricks.builtins.Control.__init__/__new__
mp_obj_t builtins_Control_obj_make_new(pbio_control_t *control) {

    builtins_Control_obj_t *self = m_new_obj(builtins_Control_obj_t);
    self->base.type = &builtins_Control_type;

    self->control = control;
    
    return self;
}

// pybricks.builtins.Control.limits
STATIC mp_obj_t builtins_Control_limits(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        builtins_Control_obj_t, self,
        PB_ARG_REQUIRED(test)
    );

    (void) self;
    (void) test;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Control_limits_obj, 0, builtins_Control_limits);

// dir(pybricks.builtins.Control)
STATIC const mp_rom_map_elem_t builtins_Control_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_limits ), MP_ROM_PTR(&builtins_Control_limits_obj) },
};
STATIC MP_DEFINE_CONST_DICT(builtins_Control_locals_dict, builtins_Control_locals_dict_table);

// type(pybricks.builtins.Control)
const mp_obj_type_t builtins_Control_type = {
    { &mp_type_type },
    .name = MP_QSTR_Control,
    .locals_dict = (mp_obj_dict_t*)&builtins_Control_locals_dict,
};
