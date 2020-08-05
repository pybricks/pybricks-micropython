// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include <string.h>

#include <pbio/iodev.h>
#include <pbio/serial.h>

#include "py/mpconfig.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objstr.h"

#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>


#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include "pbsmbus.h"

// pybricks.iodevices.I2CDevice class object
typedef struct _iodevices_I2CDevice_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
    smbus_t *bus;
    int8_t address;
} iodevices_I2CDevice_obj_t;

// pybricks.iodevices.I2CDevice.__init__
STATIC mp_obj_t iodevices_I2CDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address));
    iodevices_I2CDevice_obj_t *self = m_new_obj(iodevices_I2CDevice_obj_t);
    self->base.type = (mp_obj_type_t *)otype;

    // Get port number
    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get selected I2C Address
    mp_int_t addr = mp_obj_get_int(address);
    if (addr < 0 || addr > 255) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    self->address = addr;

    // Init I2C port
    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_CUSTOM_I2C);

    // Get the smbus, which on ev3dev is zero based sensor port number + 3.
    pb_assert(pb_smbus_get(&self->bus, port_num - PBIO_PORT_1 + 3));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.I2CDevice.read
STATIC mp_obj_t iodevices_I2CDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_I2CDevice_obj_t, self,
        PB_ARG_REQUIRED(reg),
        PB_ARG_DEFAULT_INT(length, 1));

    // Get requested data length
    mp_int_t len = mp_obj_get_int(length);
    if (len < 0 || len > PB_SMBUS_BLOCK_MAX) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // First, deal with the case where no register is given
    if (reg == mp_const_none) {
        uint8_t data;
        switch (len) {
            case 0:
                // No register, no data, so quick-read:
                pb_assert(pb_smbus_read_quick(self->bus, self->address));
                return mp_const_none;
            case 1:
                // No register, read 1 byte:
                pb_assert(pb_smbus_read_no_reg(self->bus, self->address, &data));
                return mp_obj_new_bytes(&data, 1);
            default:
                pb_assert(PBIO_ERROR_INVALID_ARG);
                return mp_const_none;
        }
    }

    // There was a register, so get it
    mp_int_t regist = mp_obj_get_int(reg);
    if (regist < 0 || regist > 255) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Read the given amount of bytes
    uint8_t buf[PB_SMBUS_BLOCK_MAX];

    pb_assert(pb_smbus_read_bytes(self->bus, self->address, regist, len, buf));

    return mp_obj_new_bytes(buf, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_I2CDevice_read_obj, 1, iodevices_I2CDevice_read);

// pybricks.iodevices.I2CDevice.write
STATIC mp_obj_t iodevices_I2CDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_I2CDevice_obj_t, self,
        PB_ARG_REQUIRED(reg),
        PB_ARG_DEFAULT_NONE(data));

    // No data means an empty byte array
    if (data == mp_const_none) {
        data = mp_obj_new_bytes(NULL, 0);
    }

    // Assert that data argument are bytes
    if (!(mp_obj_is_str_or_bytes(data) || mp_obj_is_type(data, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get data and length
    GET_STR_DATA_LEN(data, bytes, len);

    // Len 0 with no register given
    if (len == 0 && reg == mp_const_none) {
        pb_assert(pb_smbus_write_quick(self->bus, self->address));
        return mp_const_none;
    }

    // Len 1 with no register given
    if (len == 1 && reg == mp_const_none) {
        pb_smbus_write_no_reg(self->bus, self->address, bytes[0]);
        return mp_const_none;
    }

    // Get register
    mp_int_t regist = mp_obj_get_int(reg);
    if (regist < 0 || regist > 255) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Len 0 and register given just means that register is the data
    if (len == 0) {
        pb_smbus_write_no_reg(self->bus, self->address, regist);
        return mp_const_none;
    }

    // Otherwise send a block of data
    pb_assert(pb_smbus_write_bytes(self->bus, self->address, regist, len, bytes));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_I2CDevice_write_obj, 1, iodevices_I2CDevice_write);


// dir(pybricks.iodevices.I2CDevice)
STATIC const mp_rom_map_elem_t iodevices_I2CDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),    MP_ROM_PTR(&iodevices_I2CDevice_read_obj)    },
    { MP_ROM_QSTR(MP_QSTR_write),   MP_ROM_PTR(&iodevices_I2CDevice_write_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_I2CDevice_locals_dict, iodevices_I2CDevice_locals_dict_table);

// type(pybricks.iodevices.I2CDevice)
const mp_obj_type_t pb_type_iodevices_I2CDevice = {
    { &mp_type_type },
    .name = MP_QSTR_I2CDevice,
    .make_new = iodevices_I2CDevice_make_new,
    .locals_dict = (mp_obj_dict_t *)&iodevices_I2CDevice_locals_dict,
};

#endif // PYBRICKS_PY_IODEVICES
