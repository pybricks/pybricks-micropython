// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES

#include <pbio/iodev.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

#include <ev3dev_stretch/sysfs.h>

// Class structure for Ev3devSensor
typedef struct _iodevices_Ev3devSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
    mp_obj_t sensor_index;
    mp_obj_t port_index;
} iodevices_Ev3devSensor_obj_t;

// pybricks.iodevices.Ev3devSensor.__init__
STATIC mp_obj_t iodevices_Ev3devSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    iodevices_Ev3devSensor_obj_t *self = m_new_obj(iodevices_Ev3devSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_EV3DEV_LEGO_SENSOR);

    // Get the sysfs index. This is not currently exposed through pb_device,
    // so read it again by searching through the sysfs tree.
    int32_t sensor_index, port_index;
    pb_assert(sysfs_get_number(port, "/sys/class/lego-sensor", &sensor_index));
    pb_assert(sysfs_get_number(port, "/sys/class/lego-port", &port_index));
    self->sensor_index = mp_obj_new_int(sensor_index);
    self->port_index = mp_obj_new_int(port_index);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.Ev3devSensor.read
STATIC mp_obj_t iodevices_Ev3devSensor_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_Ev3devSensor_obj_t, self,
        PB_ARG_REQUIRED(mode));

    // Get mode index from mode string
    uint8_t mode_idx = pb_device_get_mode_id_from_str(self->pbdev, mp_obj_str_get_str(mode_in));

    // Get data already in correct data format
    int32_t data[PBIO_IODEV_MAX_DATA_SIZE];
    mp_obj_t objs[PBIO_IODEV_MAX_DATA_SIZE];
    pb_device_get_values(self->pbdev, mode_idx, data);

    uint8_t num_values = pb_device_get_num_values(self->pbdev);

    // Return as MicroPython objects
    for (uint8_t i = 0; i < num_values; i++) {
        objs[i] = mp_obj_new_int(data[i]);
    }

    return mp_obj_new_tuple(num_values, objs);
}
MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_Ev3devSensor_read_obj, 1, iodevices_Ev3devSensor_read);

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(iodevices_Ev3devSensor_obj_t, MP_QSTR_sensor_index, sensor_index),
    PB_DEFINE_CONST_ATTR_RO(iodevices_Ev3devSensor_obj_t, MP_QSTR_port_index, port_index),
};
STATIC MP_DEFINE_CONST_DICT(attribute_dict, attribute_table);

// dir(pybricks.iodevices.Ev3devSensor)
STATIC const mp_rom_map_elem_t iodevices_Ev3devSensor_locals_dict_table[] = {
    PB_ATTRIBUTE_TABLE(attribute_dict),
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&iodevices_Ev3devSensor_read_obj)                        },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_Ev3devSensor_locals_dict, iodevices_Ev3devSensor_locals_dict_table);

// type(pybricks.iodevices.Ev3devSensor)
const mp_obj_type_t pb_type_iodevices_Ev3devSensor = {
    { &mp_type_type },
    .make_new = iodevices_Ev3devSensor_make_new,
    .attr = pb_attribute_handler,
    .locals_dict = (mp_obj_dict_t *)&iodevices_Ev3devSensor_locals_dict,
};

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES
