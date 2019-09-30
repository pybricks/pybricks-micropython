// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/servo.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modlogger.h"

#include "pbthread.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.tools.Logger class object
typedef struct _tools_Logger_obj_t {
    mp_obj_base_t base;
    pbio_servo_t *srv;
} tools_Logger_obj_t;

STATIC mp_obj_t tools_Logger_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(duration)
    );
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_int_t duration_arg = pb_obj_get_int(duration);

    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_log_start(self->srv, duration_arg);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_start_obj, 0, tools_Logger_start);

STATIC mp_obj_t tools_Logger_get(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(index)
    );
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_int_t index_val = pb_obj_get_default_int(index, -1);
    pbio_log_data_t data;
    pbio_error_t err;
    
    pb_thread_enter();
    err = pbio_servo_log_get(self->srv, index_val, &data);
    pb_thread_exit();
    pb_assert(err);

    // Convert data to user objects
    mp_obj_t ret[3];
    ret[0] = mp_obj_new_int(data.time/1000);
    ret[1] = mp_obj_new_int(data.count);
    ret[2] = mp_obj_new_int(data.rate);

    return mp_obj_new_tuple(3, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_get_obj, 0, tools_Logger_get);

STATIC mp_obj_t tools_Logger_stop(mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_log_stop(self->srv);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_Logger_stop_obj, tools_Logger_stop);

STATIC mp_obj_t tools_Logger_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_LEN:
            pb_thread_enter();
            uint32_t len = self->srv->log.sampled;
            pb_thread_exit();
            return MP_OBJ_NEW_SMALL_INT(len);
        default:
            return MP_OBJ_NULL;
    }
}

// dir(pybricks.tools.Logger)
STATIC const mp_rom_map_elem_t tools_Logger_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&tools_Logger_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&tools_Logger_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&tools_Logger_stop_obj) },
};
STATIC MP_DEFINE_CONST_DICT(tools_Logger_locals_dict, tools_Logger_locals_dict_table);

// type(pybricks.tools.Logger)
STATIC const mp_obj_type_t tools_Logger_type = {
    { &mp_type_type },
    .locals_dict = (mp_obj_dict_t*)&tools_Logger_locals_dict,
    .unary_op = tools_Logger_unary_op,
};

mp_obj_t logger_obj_make_new(pbio_servo_t *srv) {
    // Create new light instance
    tools_Logger_obj_t *logger = m_new_obj(tools_Logger_obj_t);
    // Set type and iodev
    logger->base.type = (mp_obj_type_t*) &tools_Logger_type;
    logger->srv = srv;
    return logger;
}
