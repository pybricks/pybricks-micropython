#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/runtime.h"

#include "modiodevice.h"
#include "modmotor.h"
#include "pberror.h"

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_t motor_InteractiveMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InteractiveMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
const mp_obj_type_t motor_TrainMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TrainMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS


/*
class ColorAndDistSensor():
    """Class for the Powered Up ColorAndDistSensor."""

    def __init__(self, port):
        """Initialize the ColorAndDistSensor.

        Arguments:
            port -- Port on the hub: Port.A, Port.B, etc.
        """
*/

// Class structure for ColorAndDistSensor
typedef struct _pupdevices_ColorAndDistSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
} pupdevices_ColorAndDistSensor_obj_t;

STATIC mp_obj_t pupdevices_ColorAndDistSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    pupdevices_ColorAndDistSensor_obj_t *self = m_new_obj(pupdevices_ColorAndDistSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
    pb_assert(pb_iodevice_set_mode(self->port, 8));
    return MP_OBJ_FROM_PTR(self);
}

STATIC uint8_t pupdevices_ColorAndDistSensor_combined_mode(pbio_port_t port, uint8_t idx) {
    pb_assert(pb_iodevice_set_mode(port, 8));
    pbio_iodev_t *iodev;
    uint8_t *data;
    pb_assert(pbdrv_ioport_get_iodev(port, &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    return data[idx];
}

/*
ColorAndDistSensor
    def __str__(self):
        """String representation of ColorAndDistSensor object."""
*/
STATIC void pupdevices_ColorAndDistSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_ColorDistanceSensor));
    mp_printf(print, " on Port.%c",  self->port);
}

/*
ColorAndDistSensor
    def color(self):
        """Return the detected color.""""
*/

STATIC mp_obj_t pupdevices_ColorAndDistSensor_color(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t color;
    switch(pupdevices_ColorAndDistSensor_combined_mode(self->port, 0)) {
        case 0:
            color = PBIO_LIGHT_COLOR_BLACK;
            break;        
        case 3:
            color = PBIO_LIGHT_COLOR_BLUE;
            break;
        case 6:
            color = PBIO_LIGHT_COLOR_GREEN;
            break;
        case 7:
            color = PBIO_LIGHT_COLOR_YELLOW;
            break;
        case 8:
            color = PBIO_LIGHT_COLOR_ORANGE;
            break;
        case 9:
            color = PBIO_LIGHT_COLOR_RED;
            break;
        case 10:
            color = PBIO_LIGHT_COLOR_WHITE;
            break;
        default:
            color = PBIO_LIGHT_COLOR_NONE;
    }
    return mp_obj_new_int(color);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_color_obj, pupdevices_ColorAndDistSensor_color);

/*
ColorAndDistSensor
    def distance(self):
        """Returns distance to detected object, ranging from 0 (very close) to 10 (very far)."""
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_distance(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(pupdevices_ColorAndDistSensor_combined_mode(self->port, 1));
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_distance_obj, pupdevices_ColorAndDistSensor_distance);


/*
ColorAndDistSensor
    def reflection(self):
        """Returns surface reflection, ranging from 0 (no reflection) to 100 (very high reflection)."""
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(pupdevices_ColorAndDistSensor_combined_mode(self->port, 3));
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_reflection_obj, pupdevices_ColorAndDistSensor_reflection);

/*
ColorAndDistSensor
    def ambient(self):
        """Returns ambient light intensity, ranging from 0 (darkness) to 100 (very bright light)."""
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pb_iodevice_set_mode(self->port, 4));
    return pb_iodevice_get_values(self->port);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_ambient_obj, pupdevices_ColorAndDistSensor_ambient);


/*
ColorAndDistSensor
    def rgb(self):
        """Returns surface reflection of red, green, and blue light, each ranging from 0 (no reflection) to 100 (very high reflection)."""
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_rgb(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pb_iodevice_set_mode(self->port, 6));
    pbio_iodev_t *iodev;
    uint8_t *data;
    pb_assert(pbdrv_ioport_get_iodev(self->port, &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    mp_obj_t rgb[3];
    for (uint8_t col = 0; col < 3; col++) {
        int16_t intensity = ((*(int16_t *)(data + col * 2))*10)/44;
        rgb[col] = mp_obj_new_int(intensity < 100 ? intensity : 100);
    }
    return mp_obj_new_tuple(3, rgb);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_rgb_obj, pupdevices_ColorAndDistSensor_rgb);

/*
ColorAndDistSensor class tables
*/
STATIC const mp_rom_map_elem_t pupdevices_ColorAndDistSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_rgb_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorAndDistSensor_locals_dict, pupdevices_ColorAndDistSensor_locals_dict_table);

STATIC const mp_obj_type_t pupdevices_ColorAndDistSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorDistanceSensor,
    .print = pupdevices_ColorAndDistSensor_print,
    .make_new = pupdevices_ColorAndDistSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&pupdevices_ColorAndDistSensor_locals_dict,
};

/*
pupdevices module table
*/
STATIC const mp_map_elem_t pupdevices_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_devices) },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_InteractiveMotor), (mp_obj_t)&motor_InteractiveMotor_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TrainMotor),       (mp_obj_t)&motor_TrainMotor_type       },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Stop),             (mp_obj_t)&motor_Stop_enum             },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Dir),              (mp_obj_t)&motor_Dir_enum              },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Run),              (mp_obj_t)&motor_Run_enum              },
#endif //PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_ColorDistanceSensor), (mp_obj_t)&pupdevices_ColorAndDistSensor_type },
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_pupdevices_globals,
    pupdevices_globals_table
);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_pupdevices_globals,
};
