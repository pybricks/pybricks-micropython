// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include <string.h>

#include <pbio/port.h>
#include <pbio/port_interface.h>
#include <pbio/port_lump.h>
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
    // concurrent reads with the same sensor are not permitted.
    uint8_t last_mode;
    // ID of a passive device, if any.
    lego_device_type_id_t passive_id;
} iodevices_PUPDevice_obj_t;

/**
 * Tests if the given device is a passive device and stores ID.
 *
 * @param [in]  self        The PUP device.
 * @param [in]  port_in     The port.
 * @return                  True if passive device, false otherwise.
 */
static bool init_passive_pup_device(iodevices_PUPDevice_obj_t *self, mp_obj_t port_in) {
    pb_module_tools_assert_blocking();

    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get the port instance.
    pbio_port_t *port;
    pb_assert(pbio_port_get_port(port_id, &port));

    lego_device_type_id_t type_id = LEGO_DEVICE_TYPE_ID_ANY_DC_MOTOR;
    pbio_dcmotor_t *dcmotor;
    pbio_error_t err = pbio_port_get_dcmotor(port, &type_id, &dcmotor);

    if (err == PBIO_SUCCESS) {
        self->passive_id = type_id;
        return true;
    }
    return false;

    self->passive_id = type_id;
}

// pybricks.iodevices.PUPDevice.__init__
static mp_obj_t iodevices_PUPDevice_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    iodevices_PUPDevice_obj_t *self = mp_obj_malloc(iodevices_PUPDevice_obj_t, type);

    // For backwards compatibility, allow class to be used with passive devices.
    if (init_passive_pup_device(self, port_in)) {
        return MP_OBJ_FROM_PTR(self);
    }

    // Initialize any UART PUP device.
    pb_type_device_init_class(&self->device_base, port_in, LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART);
    self->passive_id = LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART;
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.PUPDevice.info
static mp_obj_t iodevices_PUPDevice_info(mp_obj_t self_in) {
    iodevices_PUPDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Passive devices only have an ID.
    if (self->passive_id != LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART) {
        mp_obj_t info_dict = mp_obj_new_dict(1);
        mp_obj_dict_store(info_dict, MP_ROM_QSTR(MP_QSTR_id), MP_OBJ_NEW_SMALL_INT(self->passive_id));
        return info_dict;
    }

    pbio_port_lump_mode_info_t *mode_info;
    uint8_t current_mode;
    uint8_t num_modes;
    lego_device_type_id_t type_id;
    pb_assert(pbio_port_lump_assert_type_id(self->device_base.lump_dev, &type_id));
    pb_assert(pbio_port_lump_get_info(self->device_base.lump_dev, &num_modes, &current_mode, &mode_info));

    mp_obj_t info_dict = mp_obj_new_dict(2);

    // Store device ID.
    mp_obj_dict_store(info_dict, MP_ROM_QSTR(MP_QSTR_id), MP_OBJ_NEW_SMALL_INT(type_id));

    // Store mode info.
    mp_obj_t modes[(LUMP_MAX_EXT_MODE + 1)];
    for (uint8_t m = 0; m < num_modes; m++) {
        mp_obj_t values[] = {
            mp_obj_new_str(mode_info[m].name, strlen(mode_info[m].name)),
            mp_obj_new_int(mode_info[m].num_values),
            mp_obj_new_int(mode_info[m].data_type),
        };
        modes[m] = mp_obj_new_tuple(MP_ARRAY_SIZE(values), values);
    }
    mp_obj_dict_store(info_dict, MP_ROM_QSTR(MP_QSTR_modes), mp_obj_new_tuple(num_modes, modes));

    return info_dict;
}
MP_DEFINE_CONST_FUN_OBJ_1(iodevices_PUPDevice_info_obj, iodevices_PUPDevice_info);

static mp_obj_t get_pup_data_tuple(mp_obj_t self_in) {
    iodevices_PUPDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    void *data = pb_type_device_get_data(self_in, self->last_mode);

    pbio_port_lump_mode_info_t *mode_info;
    uint8_t current_mode;
    uint8_t num_modes;
    lego_device_type_id_t type_id;
    pb_assert(pbio_port_lump_assert_type_id(self->device_base.lump_dev, &type_id));
    pb_assert(pbio_port_lump_get_info(self->device_base.lump_dev, &num_modes, &current_mode, &mode_info));


    mp_obj_t values[LUMP_MAX_MSG_SIZE];

    for (uint8_t i = 0; i < mode_info[current_mode].num_values; i++) {
        switch (mode_info[current_mode].data_type) {
            case LUMP_DATA_TYPE_DATA8:
                values[i] = mp_obj_new_int(((int8_t *)data)[i]);
                break;
            case LUMP_DATA_TYPE_DATA16:
                values[i] = mp_obj_new_int(((int16_t *)data)[i]);
                break;
            case LUMP_DATA_TYPE_DATA32:
                values[i] = mp_obj_new_int(((int32_t *)data)[i]);
                break;
            #if MICROPY_PY_BUILTINS_FLOAT
            case LUMP_DATA_TYPE_DATAF:
                values[i] = mp_obj_new_float_from_f(((float *)data)[i]);
                break;
            #endif
            default:
                pb_assert(PBIO_ERROR_IO);
        }
    }

    return mp_obj_new_tuple(mode_info[current_mode].num_values, values);
}

// pybricks.iodevices.PUPDevice.read
static mp_obj_t iodevices_PUPDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_PUPDevice_obj_t, self,
        PB_ARG_REQUIRED(mode));

    // Passive devices don't support reading.
    if (self->passive_id != LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    self->last_mode = mp_obj_get_int(mode_in);

    // We can re-use the same code as for specific sensor types, only the mode
    // is not hardcoded per call, so we create that object here.
    const pb_type_device_method_obj_t method = {
        {&pb_type_device_method},
        .mode = self->last_mode,
        .get_values = get_pup_data_tuple,
    };

    // This will take care of checking that the requested mode exist and raise
    // otherwise, so no need to check here.
    return pb_type_device_method_call(MP_OBJ_FROM_PTR(&method), 1, 0, pos_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_PUPDevice_read_obj, 1, iodevices_PUPDevice_read);

// pybricks.iodevices.PUPDevice.write
static mp_obj_t iodevices_PUPDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_PUPDevice_obj_t, self,
        PB_ARG_REQUIRED(mode),
        PB_ARG_REQUIRED(data));

    // Passive devices don't support writing.
    if (self->passive_id != LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    // Get requested mode.
    uint8_t mode = mp_obj_get_int(mode_in);

    pbio_port_lump_mode_info_t *mode_info;
    uint8_t current_mode;
    uint8_t num_modes;
    lego_device_type_id_t type_id;
    pb_assert(pbio_port_lump_assert_type_id(self->device_base.lump_dev, &type_id));
    pb_assert(pbio_port_lump_get_info(self->device_base.lump_dev, &num_modes, &current_mode, &mode_info));

    // Gets expected format for currently connected device.
    uint8_t data[LUMP_MAX_MSG_SIZE];
    if (mode >= num_modes) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Invalid mode"));
    }

    if (!mode_info[mode].writable) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Mode not writable"));
    }

    // Unpack the user data tuple
    mp_obj_t *values;
    size_t num_values_given;
    mp_obj_get_array(data_in, &num_values_given, &values);
    if (num_values_given == 0 || num_values_given != mode_info[mode].num_values) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Expected %d values"), mode_info[mode].num_values);
    }

    uint8_t size = 0;

    for (uint8_t i = 0; i < mode_info[mode].num_values; i++) {
        switch (mode_info[mode].data_type) {
            case LUMP_DATA_TYPE_DATA8: {
                mp_int_t value = mp_obj_get_int(values[i]);
                if (value > INT8_MAX || value < INT8_MIN) {
                    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Value out of range for int8"));
                }
                *(int8_t *)(data + i) = value;
                size = sizeof(int8_t) * mode_info[mode].num_values;
                break;
            }
            case LUMP_DATA_TYPE_DATA16: {
                mp_int_t value = mp_obj_get_int(values[i]);
                if (value > INT16_MAX || value < INT16_MIN) {
                    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Value out of range for int16"));
                }
                *(int16_t *)(data + i * 2) = value;
                size = sizeof(int16_t) * mode_info[mode].num_values;
                break;
            }
            case LUMP_DATA_TYPE_DATA32: {
                mp_int_t value = mp_obj_get_int(values[i]);
                if (value > INT32_MAX || value < INT32_MIN) {
                    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Value out of range for int32"));
                }
                *(int32_t *)(data + i * 4) = value;
                size = sizeof(int32_t) * mode_info[mode].num_values;
                break;
            }
            #if MICROPY_PY_BUILTINS_FLOAT
            case LUMP_DATA_TYPE_DATAF:
                *(float *)(data + i * 4) = mp_obj_get_float_to_f(values[i]);
                size = sizeof(float) * mode_info[mode].num_values;
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

// pybricks.iodevices.PUPDevice.reset
static mp_obj_t iodevices_PUPDevice_reset(mp_obj_t self_in) {
    iodevices_PUPDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Passive devices don't do any reset.
    if (self->passive_id != LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    pb_assert(pbio_port_lump_request_reset(self->device_base.lump_dev));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(iodevices_PUPDevice_reset_obj, iodevices_PUPDevice_reset);

// dir(pybricks.iodevices.PUPDevice)
static const mp_rom_map_elem_t iodevices_PUPDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),       MP_ROM_PTR(&iodevices_PUPDevice_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),      MP_ROM_PTR(&iodevices_PUPDevice_write_obj)},
    { MP_ROM_QSTR(MP_QSTR_info),       MP_ROM_PTR(&iodevices_PUPDevice_info_obj)},
    { MP_ROM_QSTR(MP_QSTR_reset),      MP_ROM_PTR(&iodevices_PUPDevice_reset_obj)},
};
static MP_DEFINE_CONST_DICT(iodevices_PUPDevice_locals_dict, iodevices_PUPDevice_locals_dict_table);

// type(pybricks.iodevices.PUPDevice)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_PUPDevice,
    MP_QSTR_PUPDevice,
    MP_TYPE_FLAG_NONE,
    make_new, iodevices_PUPDevice_make_new,
    locals_dict, &iodevices_PUPDevice_locals_dict);

#endif // PYBRICKS_PY_IODEVICES
