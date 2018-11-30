/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mphal.h"
#include "py/runtime.h"

#include "modmotor.h"

#if PBIO_CONFIG_ENABLE_MOTORS
/*
class Mechanism():
    """Class to control a motor with predefined defined target angles."""

    def __init__(self, motor, targets):
        """Initialize the mechanism.

        Arguments:
            motor {Motor} -- Motor object
            targets {dict} -- Dictionary of keys (e.g. strings, colors, or sensor values) with corresponding motor target values
            speed {int} -- Speed used by all movement commands for this mechanism
        """
*/

// Class structure for Mechanism
typedef struct _robotics_Mechanism_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
    mp_obj_dict_t *targets;
    int32_t speed;
} robotics_Mechanism_obj_t;

STATIC mp_obj_t robotics_Mechanism_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 3, 3, false);
    robotics_Mechanism_obj_t *self = m_new_obj(robotics_Mechanism_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    /*
    TODO: below, store the motor object (pointer) instead of the port, equivalent to self.motor = motor
    Currently not (quite) supported by MicroPython unless we modify the attribute handler
    */
    self->port = get_port(args[0]);
    self->targets = MP_OBJ_TO_PTR(args[1]);
    self->speed = mp_obj_get_int(args[2]);
    return MP_OBJ_FROM_PTR(self);
}

/*
Mechanism
    def __str__(self):
        """String representation of Mechanism object."""
*/
STATIC void robotics_Mechanism_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    robotics_Mechanism_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Mechanism on PORT_%c",  self->port);
}

/*
Mechanism
    def targets(self):
        """Return the previously configured targets.
        Returns:
            dict -- Dictionary of (identifier, target) pairs
        """
*/
STATIC mp_obj_t robotics_Mechanism_targets(mp_obj_t self_in) {
    robotics_Mechanism_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_FROM_PTR(self->targets);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(robotics_Mechanism_targets_obj, robotics_Mechanism_targets);

/*
Mechanism class tables
*/

STATIC const mp_rom_map_elem_t robotics_Mechanism_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_targets), MP_ROM_PTR(&robotics_Mechanism_targets_obj) },
};
STATIC MP_DEFINE_CONST_DICT(robotics_Mechanism_locals_dict, robotics_Mechanism_locals_dict_table);

STATIC const mp_obj_type_t robotics_Mechanism_type = {
    { &mp_type_type },
    .name = MP_QSTR_Mechanism,
    .print = robotics_Mechanism_print,
    .make_new = robotics_Mechanism_make_new,
    .locals_dict = (mp_obj_dict_t*)&robotics_Mechanism_locals_dict,
};

#endif // PBIO_CONFIG_ENABLE_MOTORS

/*
robotics module tables
*/

STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics) },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Mechanism),   MP_ROM_PTR(&robotics_Mechanism_type)    },
#endif // PBIO_CONFIG_ENABLE_MOTORS
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_robotics_globals,
};
