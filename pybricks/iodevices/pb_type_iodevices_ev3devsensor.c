// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES

#include <string.h>

#include <pbdrv/legodev.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/util_pb/pb_error.h>

#include <ev3dev_stretch/sysfs.h>

// Class structure for Ev3devSensor
typedef struct _iodevices_Ev3devSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    mp_obj_t sensor_index;
    mp_obj_t port_index;
} iodevices_Ev3devSensor_obj_t;

// pybricks.iodevices.Ev3devSensor.__init__
static mp_obj_t iodevices_Ev3devSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    iodevices_Ev3devSensor_obj_t *self = mp_obj_malloc(iodevices_Ev3devSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_EV3DEV_LEGO_SENSOR);

    // Get the sysfs index. This is not currently exposed through pb_device,
    // so read it again by searching through the sysfs tree.
    int32_t sensor_index, port_index;
    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(sysfs_get_number(port, "/sys/class/lego-sensor", &sensor_index));
    pb_assert(sysfs_get_number(port, "/sys/class/lego-port", &port_index));
    self->sensor_index = mp_obj_new_int(sensor_index);
    self->port_index = mp_obj_new_int(port_index);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.Ev3devSensor.read
static mp_obj_t iodevices_Ev3devSensor_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_Ev3devSensor_obj_t, self,
        PB_ARG_REQUIRED(mode));

    pbdrv_legodev_info_t *info;
    pb_assert(pbdrv_legodev_get_info(self->device_base.legodev, &info));

    uint8_t mode = 0;
    for (mode = 0; mode < info->num_modes; mode++) {
        if (!strncmp(info->mode_info[mode].name, mp_obj_str_get_str(mode_in), LUMP_MAX_NAME_SIZE)) {
            break;
        }
    }
    if (mode == info->num_modes) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    void *data = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), mode);
    mp_obj_t values[PBDRV_LEGODEV_MAX_DATA_SIZE];

    for (uint8_t i = 0; i < info->mode_info[mode].num_values; i++) {
        switch (info->mode_info[mode].data_type) {
            case PBDRV_LEGODEV_DATA_TYPE_INT8:
                values[i] = mp_obj_new_int(((int8_t *)data)[i]);
                break;
            case PBDRV_LEGODEV_DATA_TYPE_INT16:
                values[i] = mp_obj_new_int(((int16_t *)data)[i]);
                break;
            case PBDRV_LEGODEV_DATA_TYPE_INT32:
                values[i] = mp_obj_new_int(((int32_t *)data)[i]);
                break;
            #if MICROPY_PY_BUILTINS_FLOAT
            case PBDRV_LEGODEV_DATA_TYPE_FLOAT:
                values[i] = mp_obj_new_float_from_f(((float *)data)[i]);
                break;
            #endif
            default:
                pb_assert(PBIO_ERROR_IO);
        }
    }
    return mp_obj_new_tuple(info->mode_info[mode].num_values, values);
}
MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_Ev3devSensor_read_obj, 1, iodevices_Ev3devSensor_read);

static const pb_attr_dict_entry_t iodevices_Ev3devSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_sensor_index, iodevices_Ev3devSensor_obj_t, sensor_index),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_port_index, iodevices_Ev3devSensor_obj_t, port_index),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.iodevices.Ev3devSensor)
static const mp_rom_map_elem_t iodevices_Ev3devSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&iodevices_Ev3devSensor_read_obj)                        },
};
static MP_DEFINE_CONST_DICT(iodevices_Ev3devSensor_locals_dict, iodevices_Ev3devSensor_locals_dict_table);

// type(pybricks.iodevices.Ev3devSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_Ev3devSensor,
    MP_QSTR_Ev3devSensor,
    MP_TYPE_FLAG_NONE,
    make_new, iodevices_Ev3devSensor_make_new,
    attr, pb_attribute_handler,
    protocol, iodevices_Ev3devSensor_attr_dict,
    locals_dict, &iodevices_Ev3devSensor_locals_dict);

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES
