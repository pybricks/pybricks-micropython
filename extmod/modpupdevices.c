#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/runtime.h"

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

/**
 * Modes for the BOOST Color and Distance Sensor. TODO: move elsewhere and talk about modes
 */
typedef enum {
    PBIO_IODEV_MODE_COLOR_DIST_SENSOR_COLOR = 0,
    PBIO_IODEV_MODE_COLOR_DIST_SENSOR_PROXIMITY = 1,
    PBIO_IODEV_MODE_COLOR_DIST_SENSOR_REFLECTED = 3,
    PBIO_IODEV_MODE_COLOR_DIST_SENSOR_AMBIENT = 4,
    PBIO_IODEV_MODE_COLOR_DIST_SENSOR_SPEC = 8,
} pbio_iodev_mode_color_dist_sensor_t;


// TODO: Probably not handle modes at mpy level; this is a placeholder
pbio_error_t set_mode(pbio_port_t port, uint8_t *current_mode, uint8_t new_mode) {
    if (*current_mode == new_mode) {
        return PBIO_SUCCESS;
    }
    pbio_iodev_t *iodev;
    pbio_error_t err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS){
        return err;
    }
    err = pbio_iodev_set_mode(iodev, new_mode);
    if (err != PBIO_SUCCESS){
        return err;
    }    
    *current_mode = new_mode;
    return err;
}

// Class structure for ColorAndDistSensor
typedef struct _pupdevices_ColorAndDistSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
    uint8_t mode; // Temporary: Probably not handle modes at mpy level; this is a placeholder
} pupdevices_ColorAndDistSensor_obj_t;

mp_obj_t pupdevices_ColorAndDistSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    pupdevices_ColorAndDistSensor_obj_t *self = m_new_obj(pupdevices_ColorAndDistSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
    self->mode = 255; // Probably not handle modes at mpy level; this is a placeholder. If we keep it, this line should read the current mode the sensor is in
    return MP_OBJ_FROM_PTR(self);
}

/*
ColorAndDistSensor
    def __str__(self):
        """String representation of ColorAndDistSensor object."""
*/
void pupdevices_ColorAndDistSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_ColorDistanceSensor));
    mp_printf(print, " on Port.%c",  self->port);
}

/*
ColorAndDistSensor
    def color(self):
        """Return the detected color.
        Returns:
            int -- The detected color
        """
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_color(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(set_mode(self->port, &self->mode, PBIO_IODEV_MODE_COLOR_DIST_SENSOR_COLOR));
    pbio_iodev_t *iodev;
    uint8_t *data;
    uint8_t color;
    pb_assert(pbdrv_ioport_get_iodev(self->port, &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    switch(data[0]) {
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
        case 10:
            color = PBIO_LIGHT_COLOR_WHITE;
            break;
        default:
            color = PBIO_LIGHT_COLOR_NONE;
    }

    return mp_obj_new_int(color);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_color_obj, pupdevices_ColorAndDistSensor_color);

/*
ColorAndDistSensor
    def reflection(self):
        """// TODO: discuss the word reflection
        """
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(set_mode(self->port, &self->mode, PBIO_IODEV_MODE_COLOR_DIST_SENSOR_REFLECTED));
    pbio_iodev_t *iodev;
    uint8_t *data;
    pb_assert(pbdrv_ioport_get_iodev(self->port, &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    return mp_obj_new_int(data[0]);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_reflection_obj, pupdevices_ColorAndDistSensor_reflection);

/*
ColorAndDistSensor
    def ambient(self):
        """// TODO:
        """
*/
STATIC mp_obj_t pupdevices_ColorAndDistSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(set_mode(self->port, &self->mode, PBIO_IODEV_MODE_COLOR_DIST_SENSOR_AMBIENT));
    // TODO: use generic read method here
    pbio_iodev_t *iodev;
    uint8_t *data;
    pb_assert(pbdrv_ioport_get_iodev(self->port, &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    return mp_obj_new_int(data[0]);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_ambient_obj, pupdevices_ColorAndDistSensor_ambient);


/*
ColorAndDistSensor class tables
*/
STATIC const mp_rom_map_elem_t pupdevices_ColorAndDistSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_ambient_obj) },
};
MP_DEFINE_CONST_DICT(pupdevices_ColorAndDistSensor_locals_dict, pupdevices_ColorAndDistSensor_locals_dict_table);

const mp_obj_type_t pupdevices_ColorAndDistSensor_type = {
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
