// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <string.h>

#include "py/mpconfig.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objstr.h"

#include "pbdevice.h"
#include "pbobj.h"
#include "pbkwarg.h"
#include "modmotor.h"
#include "modparameters.h"

#include <pbio/iodev.h>
#include <pbio/serial.h>
#include <pberror.h>

#if PYBRICKS_PY_EV3DEVICES

#include "pbsmbus.h"

#define UART_MAX_LEN (32*1024)

// pybricks.iodevices.AnalogSensor class object
typedef struct _iodevices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    bool active;
    pbdevice_t *pbdev;
} iodevices_AnalogSensor_obj_t;

// pybricks.iodevices.AnalogSensor.__init__
STATIC mp_obj_t iodevices_AnalogSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_TRUE(verify_type)
    );
    iodevices_AnalogSensor_obj_t *self = m_new_obj(iodevices_AnalogSensor_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    pbio_iodev_type_id_t id = mp_obj_is_true(verify_type) ? PBIO_IODEV_TYPE_ID_NXT_ANALOG : PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG;

    self->pbdev = pbdevice_get_device(port_num, id);

    // Initialize NXT sensors to passive state
    int32_t voltage;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.AnalogSensor.voltage
STATIC mp_obj_t iodevices_AnalogSensor_voltage(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pbdevice_get_values(self->pbdev, mode, &voltage);
    return mp_obj_new_int(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_voltage_obj, iodevices_AnalogSensor_voltage);

// pybricks.iodevices.AnalogSensor.resistance
STATIC mp_obj_t iodevices_AnalogSensor_resistance(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pbdevice_get_values(self->pbdev, mode, &voltage);

    // Open terminal/infinite resistance, return infinite resistance
    const int32_t vmax = 4972;
    if (voltage >= vmax) {
        return mp_obj_new_int(MP_SSIZE_MAX);
    }
    // Return as if a pure voltage divider between load and 10K internal resistor
    return mp_obj_new_int((10000*voltage)/(vmax-voltage));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_resistance_obj, iodevices_AnalogSensor_resistance);

// pybricks.iodevices.AnalogSensor.active
STATIC mp_obj_t iodevices_AnalogSensor_active(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &voltage);
    self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_active_obj, iodevices_AnalogSensor_active);

// pybricks.iodevices.AnalogSensor.passive
STATIC mp_obj_t iodevices_AnalogSensor_passive(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage);
    self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_passive_obj, iodevices_AnalogSensor_passive);

// dir(pybricks.iodevices.AnalogSensor)
STATIC const mp_rom_map_elem_t iodevices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&iodevices_AnalogSensor_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_resistance), MP_ROM_PTR(&iodevices_AnalogSensor_resistance_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),     MP_ROM_PTR(&iodevices_AnalogSensor_active_obj )    },
    { MP_ROM_QSTR(MP_QSTR_passive),    MP_ROM_PTR(&iodevices_AnalogSensor_passive_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_AnalogSensor_locals_dict, iodevices_AnalogSensor_locals_dict_table);

// type(pybricks.iodevices.AnalogSensor)
STATIC const mp_obj_type_t iodevices_AnalogSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogSensor,
    .make_new = iodevices_AnalogSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&iodevices_AnalogSensor_locals_dict,
};

// pybricks.iodevices.I2CDevice class object
typedef struct _iodevices_I2CDevice_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
    smbus_t *bus;
    int8_t address;
} iodevices_I2CDevice_obj_t;

// pybricks.iodevices.I2CDevice.__init__
STATIC mp_obj_t iodevices_I2CDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address)
    );
    iodevices_I2CDevice_obj_t *self = m_new_obj(iodevices_I2CDevice_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    // Get port number
    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get selected I2C Address
    mp_int_t addr = mp_obj_get_int(address);
    if (addr < 0 || addr > 255) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    self->address = addr;

    // Init I2C port
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_CUSTOM_I2C);

    // Get the smbus, which on ev3dev is zero based sensor port number + 3.
    pb_assert(pb_smbus_get(&self->bus, port_num - PBIO_PORT_1 + 3));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.I2CDevice.read
STATIC mp_obj_t iodevices_I2CDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_I2CDevice_obj_t, self,
        PB_ARG_REQUIRED(reg),
        PB_ARG_REQUIRED(length)
    );

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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_I2CDevice_read_obj, 0, iodevices_I2CDevice_read);

// pybricks.iodevices.I2CDevice.write
STATIC mp_obj_t iodevices_I2CDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_I2CDevice_obj_t, self,
        PB_ARG_REQUIRED(reg),
        PB_ARG_REQUIRED(data)
    );

    // Assert that data argument are bytes
    if (!mp_obj_is_str_or_bytes(data)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get data and length
    GET_STR_DATA_LEN(data, bytes, len);

    // Len 0 with no register given
    if (len == 0 && reg == mp_const_none) {
        pb_assert(pb_smbus_write_quick(self->bus, self->address));
        return mp_const_none;
    }

    // Len 0 is not allowed in any other case
    if (len == 0) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
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

    pb_assert(pb_smbus_write_bytes(self->bus, self->address, regist, len, bytes));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_I2CDevice_write_obj, 0, iodevices_I2CDevice_write);


// dir(pybricks.iodevices.I2CDevice)
STATIC const mp_rom_map_elem_t iodevices_I2CDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),    MP_ROM_PTR(&iodevices_I2CDevice_read_obj)    },
    { MP_ROM_QSTR(MP_QSTR_write),   MP_ROM_PTR(&iodevices_I2CDevice_write_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_I2CDevice_locals_dict, iodevices_I2CDevice_locals_dict_table);

// type(pybricks.iodevices.I2CDevice)
STATIC const mp_obj_type_t iodevices_I2CDevice_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2CDevice,
    .make_new = iodevices_I2CDevice_make_new,
    .locals_dict = (mp_obj_dict_t*)&iodevices_I2CDevice_locals_dict,
};

// pybricks.iodevices.UARTDevice class object
typedef struct _iodevices_UARTDevice_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
    pbio_serial_t *serial;
} iodevices_UARTDevice_obj_t;

// pybricks.iodevices.UARTDevice.__init__
STATIC mp_obj_t iodevices_UARTDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(baudrate),
        PB_ARG_DEFAULT_NONE(timeout)
    );
    iodevices_UARTDevice_obj_t *self = m_new_obj(iodevices_UARTDevice_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    // Get port number
    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Init UART port
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_CUSTOM_UART);

    // Get and init serial
    pb_assert(pbio_serial_get(
        &self->serial,
        port_num,
        pb_obj_get_int(baudrate),
        timeout == mp_const_none ? -1 : pb_obj_get_int(timeout)
    ));
    pb_assert(pbio_serial_clear(self->serial));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.UARTDevice.write
STATIC mp_obj_t iodevices_UARTDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_UARTDevice_obj_t, self,
        PB_ARG_REQUIRED(data)
    );

    // Assert that data argument are bytes
    if (!mp_obj_is_str_or_bytes(data)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get data and length
    GET_STR_DATA_LEN(data, bytes, len);

    // Write data to serial
    pb_assert(pbio_serial_write(self->serial, bytes, len));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_write_obj, 0, iodevices_UARTDevice_write);

// pybricks.iodevices.UARTDevice.waiting
STATIC mp_obj_t iodevices_UARTDevice_waiting(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t waiting;
    pb_assert(pbio_serial_in_waiting(self->serial, &waiting));
    return mp_obj_new_int(waiting);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_waiting_obj, iodevices_UARTDevice_waiting);

// pybricks.iodevices.UARTDevice._read_internal
STATIC mp_obj_t iodevices_UARTDevice_read_internal(iodevices_UARTDevice_obj_t *self, size_t len) {

    if (len > UART_MAX_LEN) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // If we don't need to read anything, return empty bytearray
    if (len < 1) {
        uint8_t none = 0;
        return mp_obj_new_bytes(&none, 0);
    }

    // Read data into buffer
    uint8_t *buf = m_malloc(len);
    if (buf == NULL) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Read up to the timeout
    pbio_error_t err;
    while ((err = pbio_serial_read(self->serial, buf, len)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    // Raise io/timeout error if needed.
    pb_assert(err);

    // Convert to bytes
    mp_obj_t ret = mp_obj_new_bytes(buf, len);

    // Free internal buffer and return bytes
    m_free(buf, len);
    return ret;
}

// pybricks.iodevices.UARTDevice.read
STATIC mp_obj_t iodevices_UARTDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_UARTDevice_obj_t, self,
        PB_ARG_DEFAULT_INT(length, 1)
    );

    size_t len = mp_obj_get_int(length);
    return iodevices_UARTDevice_read_internal(self, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_read_obj, 0, iodevices_UARTDevice_read);

// pybricks.iodevices.UARTDevice.read_all
STATIC mp_obj_t iodevices_UARTDevice_read_all(mp_obj_t self_in) {

    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t len;
    pb_assert(pbio_serial_in_waiting(self->serial, &len));

    return iodevices_UARTDevice_read_internal(self, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_read_all_obj, iodevices_UARTDevice_read_all);

// pybricks.iodevices.UARTDevice.clear
STATIC mp_obj_t iodevices_UARTDevice_clear(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_serial_clear(self->serial));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_clear_obj, iodevices_UARTDevice_clear);

// dir(pybricks.iodevices.UARTDevice)
STATIC const mp_rom_map_elem_t iodevices_UARTDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),  MP_ROM_PTR(&iodevices_UARTDevice_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_all),  MP_ROM_PTR(&iodevices_UARTDevice_read_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),  MP_ROM_PTR(&iodevices_UARTDevice_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_waiting),MP_ROM_PTR(&iodevices_UARTDevice_waiting_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear),MP_ROM_PTR(&iodevices_UARTDevice_clear_obj) },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_UARTDevice_locals_dict, iodevices_UARTDevice_locals_dict_table);

// type(pybricks.iodevices.UARTDevice)
STATIC const mp_obj_type_t iodevices_UARTDevice_type = {
    { &mp_type_type },
    .name = MP_QSTR_UARTDevice,
    .make_new = iodevices_UARTDevice_make_new,
    .locals_dict = (mp_obj_dict_t*)&iodevices_UARTDevice_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES 

// dir(pybricks.iodevices)
STATIC const mp_rom_map_elem_t iodevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_iodevices)              },
#if PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&iodevices_AnalogSensor_type)    },
    { MP_ROM_QSTR(MP_QSTR_I2CDevice),        MP_ROM_PTR(&iodevices_I2CDevice_type   )    },
    { MP_ROM_QSTR(MP_QSTR_UARTDevice),       MP_ROM_PTR(&iodevices_UARTDevice_type  )    },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),          MP_ROM_PTR(&motor_DCMotor_type)             },
#endif // PYBRICKS_PY_EV3DEVICES 
};
STATIC MP_DEFINE_CONST_DICT(pb_module_iodevices_globals, iodevices_globals_table);

const mp_obj_module_t pb_module_iodevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_iodevices_globals,
};
