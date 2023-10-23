// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include <string.h>

#include <pbdrv/legodev.h>
#include <pbdrv/legodev.h>
#include <pbio/int_math.h>

#include "py/objstr.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for PUPDevice
typedef struct _iodevices_PUPDevice_obj_t {
    pb_type_device_obj_base_t device_base;
    // Mode used when initiating awaitable read. REVISIT: This should be stored
    // on the awaitable instead, as extra context. For now, it is safe since
    // concurrent reads with the same sensor are not permitten.
    uint8_t last_mode;
} iodevices_PUPDevice_obj_t;

// pybricks.iodevices.PUPDevice.__init__
STATIC mp_obj_t iodevices_PUPDevice_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    iodevices_PUPDevice_obj_t *self = mp_obj_malloc(iodevices_PUPDevice_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_ANY_LUMP_UART);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.PUPDevice.info
STATIC mp_obj_t iodevices_PUPDevice_info(mp_obj_t self_in) {
    iodevices_PUPDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbdrv_legodev_info_t *info;
    pb_assert(pbdrv_legodev_get_info(self->device_base.legodev, &info));

    mp_obj_t info_dict = mp_obj_new_dict(2);

    // Store device ID.
    mp_obj_dict_store(info_dict, MP_ROM_QSTR(MP_QSTR_id), MP_OBJ_NEW_SMALL_INT(info->type_id));

    // Store mode info.
    mp_obj_t modes[PBDRV_LEGODEV_MAX_NUM_MODES];
    for (uint8_t m = 0; m < info->num_modes; m++) {
        mp_obj_t values[] = {
            mp_obj_new_str(info->mode_info[m].name, strlen(info->mode_info[m].name)),
            mp_obj_new_int(info->mode_info[m].num_values),
            mp_obj_new_int(info->mode_info[m].data_type),
        };
        modes[m] = mp_obj_new_tuple(MP_ARRAY_SIZE(values), values);
    }
    mp_obj_dict_store(info_dict, MP_ROM_QSTR(MP_QSTR_modes), mp_obj_new_tuple(info->num_modes, modes));

    return info_dict;
}
MP_DEFINE_CONST_FUN_OBJ_1(iodevices_PUPDevice_info_obj, iodevices_PUPDevice_info);

STATIC mp_obj_t get_pup_data_tuple(mp_obj_t self_in) {
    iodevices_PUPDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    void *data = pb_type_device_get_data(self_in, self->last_mode);

    pbdrv_legodev_info_t *info;
    pb_assert(pbdrv_legodev_get_info(self->device_base.legodev, &info));

    mp_obj_t values[PBDRV_LEGODEV_MAX_DATA_SIZE];

    for (uint8_t i = 0; i < info->mode_info[info->mode].num_values; i++) {
        switch (info->mode_info[info->mode].data_type) {
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

    return mp_obj_new_tuple(info->mode_info[info->mode].num_values, values);
}

// pybricks.iodevices.PUPDevice.read
STATIC mp_obj_t iodevices_PUPDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_PUPDevice_obj_t, self,
        PB_ARG_REQUIRED(mode));

    self->last_mode = mp_obj_get_int(mode_in);

    // We can re-use the same code as for specific sensor types, only the mode
    // is not hardcoded per call, so we create that object here.
    const pb_type_device_method_obj_t method = {
        {&pb_type_device_method},
        .mode = self->last_mode,
        .get_values = get_pup_data_tuple,
    };
    return pb_type_device_method_call(MP_OBJ_FROM_PTR(&method), 1, 0, pos_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_PUPDevice_read_obj, 1, iodevices_PUPDevice_read);

// pybricks.iodevices.PUPDevice.write
STATIC mp_obj_t iodevices_PUPDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_PUPDevice_obj_t, self,
        PB_ARG_REQUIRED(mode),
        PB_ARG_REQUIRED(data));

    // Get requested mode.
    uint8_t mode = mp_obj_get_int(mode_in);

    // Unpack the user data tuple
    mp_obj_t *values;
    size_t num_values;
    mp_obj_get_array(data_in, &num_values, &values);
    if (num_values == 0 || num_values > PBDRV_LEGODEV_MAX_DATA_SIZE) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    uint8_t data[PBDRV_LEGODEV_MAX_DATA_SIZE];
    pbdrv_legodev_info_t *info;
    pb_assert(pbdrv_legodev_get_info(self->device_base.legodev, &info));

    pbdrv_legodev_mode_info_t *mode_info = &info->mode_info[mode];
    if (!mode_info->writable) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    uint8_t size = 0;

    for (uint8_t i = 0; i < mode_info->num_values; i++) {
        switch (mode_info->data_type) {
            case PBDRV_LEGODEV_DATA_TYPE_INT8:
                *(int8_t *)(data + i) = pbio_int_math_clamp(mp_obj_get_int(values[i]), INT8_MAX);
                size = sizeof(int8_t) * mode_info->num_values;
                break;
            case PBDRV_LEGODEV_DATA_TYPE_INT16:
                *(int16_t *)(data + i * 2) = pbio_int_math_clamp(mp_obj_get_int(values[i]), INT16_MAX);
                size = sizeof(int16_t) * mode_info->num_values;
                break;
            case PBDRV_LEGODEV_DATA_TYPE_INT32:
                *(int32_t *)(data + i * 4) = pbio_int_math_clamp(mp_obj_get_int(values[i]), INT32_MAX);
                size = sizeof(int32_t) * mode_info->num_values;
                break;
            #if MICROPY_PY_BUILTINS_FLOAT
            case PBDRV_LEGODEV_DATA_TYPE_FLOAT:
                *(float *)(data + i * 4) = mp_obj_get_float_to_f(values[i]);
                size = sizeof(float) * mode_info->num_values;
                break;
            #endif
            default:
                pb_assert(PBIO_ERROR_IO);
        }
    }
    // Set the data.
    return pb_type_device_set_data(&self->device_base, mode, data, size);
}
MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_PUPDevice_write_obj, 1, iodevices_PUPDevice_write);

// dir(pybricks.iodevices.PUPDevice)
STATIC const mp_rom_map_elem_t iodevices_PUPDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),       MP_ROM_PTR(&iodevices_PUPDevice_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),      MP_ROM_PTR(&iodevices_PUPDevice_write_obj)},
    { MP_ROM_QSTR(MP_QSTR_info),       MP_ROM_PTR(&iodevices_PUPDevice_info_obj)},
};
STATIC MP_DEFINE_CONST_DICT(iodevices_PUPDevice_locals_dict, iodevices_PUPDevice_locals_dict_table);

// type(pybricks.iodevices.PUPDevice)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_PUPDevice,
    MP_QSTR_PUPDevice,
    MP_TYPE_FLAG_NONE,
    make_new, iodevices_PUPDevice_make_new,
    locals_dict, &iodevices_PUPDevice_locals_dict);

#endif // PYBRICKS_PY_IODEVICES
