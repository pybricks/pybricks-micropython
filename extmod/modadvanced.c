// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/mpconfig.h"

#if PYBRICKS_PY_ADVANCED

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/hbridge.h>
#include <pbio/error.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

#include "pberror.h"
#include "pbiodevice.h"
#include "pbthread.h"

/*
class IODevice():
    """Docstring."""
*/

// Class structure for IODevices
// TODO: Use generic type for classes that just have a port property. They can also share the get_port.
typedef struct _advanced_IODevice_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
    pbio_hbridge_t *hbridge;
} advanced_IODevice_obj_t;

/*
IODevice
    def __init__(self, port):
        """Docstring."""
*/
STATIC mp_obj_t advanced_IODevice_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    advanced_IODevice_obj_t *self = m_new_obj(advanced_IODevice_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    pbio_port_t port = mp_obj_get_int(args[0]);
    pb_assert(pbdrv_ioport_get_iodev(port, &self->iodev));

    pbio_error_t err;
    pb_thread_enter();   
    err = pbio_hbridge_get(port, &self->hbridge, PBIO_DIRECTION_COUNTERCLOCKWISE, 0, 10000);
    pb_thread_exit();
    pb_assert(err);

    return MP_OBJ_FROM_PTR(self);
}

/*
IODevice
    def __str__(self):
        """String representation of IODevice object."""
*/
STATIC void advanced_IODevice_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_IODevice));
    mp_printf(print, " on Port.%c",  self->iodev->port);
}

STATIC mp_obj_t advanced_IODevice_type_id(const mp_obj_t self_in) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_iodev_type_id_t id;
    pb_assert(pb_iodevice_get_type_id(self->iodev, &id));
    return mp_obj_new_int(id);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(advanced_IODevice_type_id_obj, advanced_IODevice_type_id);

STATIC mp_obj_t advanced_IODevice_mode(size_t n_args, const mp_obj_t *args) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get mode
        uint8_t mode;
        pb_assert(pb_iodevice_get_mode(self->iodev, &mode));
        return mp_obj_new_int(mode);
    }
    else {
        // set mode
        pb_iodevice_set_mode(self->iodev, mp_obj_get_int(args[1]));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(advanced_IODevice_mode_obj, 1, 2, advanced_IODevice_mode);

STATIC mp_obj_t advanced_IODevice_values(size_t n_args, const mp_obj_t *args) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get values
        return pb_iodevice_get_values(self->iodev);
    }
    else {
        // set values
        return pb_iodevice_set_values(self->iodev, args[1]);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(advanced_IODevice_values_obj, 1, 2, advanced_IODevice_values);

STATIC mp_obj_t advanced_IODevice_supply_on(mp_obj_t self_in) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_error_t err;

    pb_thread_enter();   
    
    err = pbio_hbridge_set_duty_cycle_usr(self->hbridge, 100);

    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(advanced_IODevice_supply_on_obj, advanced_IODevice_supply_on);

STATIC mp_obj_t advanced_IODevice_supply_off(mp_obj_t self_in) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_error_t err;

    pb_thread_enter();   
    
    err = pbio_hbridge_coast(self->hbridge);

    pb_thread_exit();

    pb_assert(err);


    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(advanced_IODevice_supply_off_obj, advanced_IODevice_supply_off);


/*
IODevice class tables
*/
STATIC const mp_rom_map_elem_t advanced_IODevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_type_id), MP_ROM_PTR(&advanced_IODevice_type_id_obj)  },
    { MP_ROM_QSTR(MP_QSTR_values),  MP_ROM_PTR(&advanced_IODevice_values_obj)   },
    { MP_ROM_QSTR(MP_QSTR_mode),    MP_ROM_PTR(&advanced_IODevice_mode_obj)     },
    { MP_ROM_QSTR(MP_QSTR_on),    MP_ROM_PTR(&advanced_IODevice_supply_on_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),    MP_ROM_PTR(&advanced_IODevice_supply_off_obj)     },
};
STATIC MP_DEFINE_CONST_DICT(advanced_IODevice_locals_dict, advanced_IODevice_locals_dict_table);

STATIC const mp_obj_type_t advanced_IODevice_type = {
    { &mp_type_type },
    .name = MP_QSTR_IODevice,
    .print = advanced_IODevice_print,
    .make_new = advanced_IODevice_make_new,
    .locals_dict = (mp_obj_dict_t*)&advanced_IODevice_locals_dict,
};

/*
advanced module tables
*/

STATIC const mp_rom_map_elem_t advanced_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_advanced)       },
    { MP_ROM_QSTR(MP_QSTR_IODevice),    MP_ROM_PTR(&advanced_IODevice_type) },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_advanced_globals, advanced_globals_table);

const mp_obj_module_t pb_module_advanced = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_advanced_globals,
};

#endif //PYBRICKS_PY_ADVANCED
