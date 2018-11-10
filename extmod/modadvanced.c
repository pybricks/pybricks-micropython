#include "mpconfigbrick.h"
#if PYBRICKS_MODULE_ADVANCED

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

#include "modiodevice.h"
#include "pberror.h"

/*
class IODevice():
    """Docstring."""
*/

// Class structure for IODevices 
// TODO: Use generic type for classes that just have a port property. They can also share the get_port.
typedef struct _advanced_IODevice_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
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
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
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
    mp_printf(print, " on Port.%c",  self->port);    
}


STATIC mp_obj_t advanced_IODevice_mode(size_t n_args, const mp_obj_t *args) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get mode
        uint8_t mode;
        pb_assert(pb_iodevice_get_mode(self->port, &mode));
        return mp_obj_new_int(mode);
    }
    else {
        // set mode
        pb_assert(pb_iodevice_set_mode(self->port, mp_obj_get_int(args[1])));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(advanced_IODevice_mode_obj, 1, 2, advanced_IODevice_mode);

STATIC mp_obj_t advanced_IODevice_values(size_t n_args, const mp_obj_t *args) {
    advanced_IODevice_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get values
        return pb_iodevice_get_values(self->port);
    }
    else {
        // set values
        return pb_iodevice_set_values(self->port, args[1]);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(advanced_IODevice_values_obj, 1, 2, advanced_IODevice_values);

/*
IODevice class tables
*/
STATIC const mp_rom_map_elem_t advanced_IODevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_values), MP_ROM_PTR(&advanced_IODevice_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&advanced_IODevice_mode_obj) },   
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

STATIC const mp_map_elem_t advanced_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_advanced) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_IODevice), (mp_obj_t)&advanced_IODevice_type},
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_advanced_globals,
    advanced_globals_table
);


const mp_obj_module_t pb_module_advanced = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_advanced_globals,
};

#endif //PYBRICKS_MODULE_ADVANCED
