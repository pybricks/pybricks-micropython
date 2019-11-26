// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <string.h>

#include "py/mpconfig.h"

#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pbkwarg.h"
#include "modparameters.h"

#include "modsmbus.h"

#include <pbio/iodev.h>
#include <pbio/ev3device.h>
#include <pbio/serial.h>
#include <pberror.h>

// pybricks.customdevices.AnalogSensor class object
typedef struct _customdevices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    bool active;
    pbio_ev3iodev_t *iodev;
} customdevices_AnalogSensor_obj_t;

// pybricks.customdevices.AnalogSensor.__init__
STATIC mp_obj_t customdevices_AnalogSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_TRUE(verify_type)
    );
    customdevices_AnalogSensor_obj_t *self = m_new_obj(customdevices_AnalogSensor_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    pbio_iodev_type_id_t id = mp_obj_is_true(verify_type) ? PBIO_IODEV_TYPE_ID_NXT_ANALOG : PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG;

    pbio_error_t err;
    while ((err = ev3device_get_device(&self->iodev, id, port_num)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    // Initialize NXT sensors to passive state
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.customdevices.AnalogSensor.voltage
STATIC mp_obj_t customdevices_AnalogSensor_voltage(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_assert(ev3device_get_values_at_mode(self->iodev, mode, &voltage));
    return mp_obj_new_int(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_voltage_obj, customdevices_AnalogSensor_voltage);

// pybricks.customdevices.AnalogSensor.resistance
STATIC mp_obj_t customdevices_AnalogSensor_resistance(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_assert(ev3device_get_values_at_mode(self->iodev, mode, &voltage));

    // Open terminal/infinite resistance, return infinite resistance
    const int32_t vmax = 4972;
    if (voltage >= vmax) {
        return mp_obj_new_int(MP_SSIZE_MAX);
    }
    // Return as if a pure voltage divider between load and 10K internal resistor
    return mp_obj_new_int((10000*voltage)/(vmax-voltage));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_resistance_obj, customdevices_AnalogSensor_resistance);

// pybricks.customdevices.AnalogSensor.active
STATIC mp_obj_t customdevices_AnalogSensor_active(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &voltage));
    self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_active_obj, customdevices_AnalogSensor_active);

// pybricks.customdevices.AnalogSensor.passive
STATIC mp_obj_t customdevices_AnalogSensor_passive(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage));
    self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_passive_obj, customdevices_AnalogSensor_passive);

// dir(pybricks.customdevices.AnalogSensor)
STATIC const mp_rom_map_elem_t customdevices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&customdevices_AnalogSensor_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_resistance), MP_ROM_PTR(&customdevices_AnalogSensor_resistance_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),     MP_ROM_PTR(&customdevices_AnalogSensor_active_obj )    },
    { MP_ROM_QSTR(MP_QSTR_passive),    MP_ROM_PTR(&customdevices_AnalogSensor_passive_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(customdevices_AnalogSensor_locals_dict, customdevices_AnalogSensor_locals_dict_table);

// type(pybricks.customdevices.AnalogSensor)
STATIC const mp_obj_type_t customdevices_AnalogSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogSensor,
    .make_new = customdevices_AnalogSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&customdevices_AnalogSensor_locals_dict,
};

// pybricks.customdevices.I2CDevice class object
typedef struct _customdevices_I2CDevice_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
    smbus_t *bus;
    int8_t address;
} customdevices_I2CDevice_obj_t;

// pybricks.customdevices.I2CDevice.__init__
STATIC mp_obj_t customdevices_I2CDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address)
    );
    customdevices_I2CDevice_obj_t *self = m_new_obj(customdevices_I2CDevice_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    // Get port number
    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    // Get selected I2C Address
    mp_int_t addr = mp_obj_get_int(address);
    if (addr < 0 || addr > 255) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    self->address = addr;

    // Init I2C port
    pbio_error_t err;
    while ((err = ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_CUSTOM_I2C, port_num)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }

    // Get the smbus, which on ev3dev is zero based sensor port number + 3.
    pb_assert(smbus_get(&self->bus, port_num - PBIO_PORT_1 + 3));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.customdevices.I2CDevice.read
STATIC mp_obj_t customdevices_I2CDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(reg),
        PB_ARG_REQUIRED(length)
    );

    customdevices_I2CDevice_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // Get requested data length
    mp_int_t len = mp_obj_get_int(length);
    if (len < 0 || len > I2C_MAX_LEN) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // First, deal with the case where no register is given
    if (reg == mp_const_none) {
        uint8_t data;
        switch (len) {
            case 0:
                // No register, no data, so quick-read:
                pb_assert(smbus_read_quick(self->bus, self->address));
                return mp_const_none;
            case 1:
                // No register, read 1 byte:
                pb_assert(smbus_read_no_reg(self->bus, self->address, &data));
                return mp_obj_new_int(data);
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
    uint8_t buf[I2C_MAX_LEN];
    mp_obj_t ret[I2C_MAX_LEN];

    pb_assert(smbus_read_bytes(self->bus, self->address, regist, len, buf));

    // Wrap the data in a tuple and return it
    for (uint8_t i = 0; i < len; i++) {
        ret[i] = mp_obj_new_int(buf[i]);
    }

    return mp_obj_new_tuple(len, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(customdevices_I2CDevice_read_obj, 0, customdevices_I2CDevice_read);

// pybricks.customdevices.I2CDevice.write
STATIC mp_obj_t customdevices_I2CDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(reg),
        PB_ARG_REQUIRED(data)
    );

    customdevices_I2CDevice_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // First, deal with the case where no data is given
    if (data == mp_const_none) {
        // In this case, register must also be None
        if (reg != mp_const_none) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
        // Do a quick write and we're done
        pb_assert(smbus_write_quick(self->bus, self->address));
        return mp_const_none;
    }

    // Get register if given
    mp_int_t regist = 0;
    if (reg != mp_const_none) {
        regist = mp_obj_get_int(reg);
        if (regist < 0 || regist > 255) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
    }

    // Unpack user argument to bytes
    uint8_t *bytes;
    size_t len;
    bool clean = unpack_byte_arg(data, &bytes, &len);

    pbio_error_t err;

    // First, deal with the case where no register is given
    if (reg == mp_const_none) {
        // There must be only one byte
        if (len != 1) {
            err = PBIO_ERROR_INVALID_ARG;
        }
        else {
            err = smbus_write_no_reg(self->bus, self->address, bytes[0]);
        }
    }
    else {
        // There is data and a register, so send all data
        err = smbus_write_bytes(self->bus, self->address, regist, len, bytes);
    }

    // Clean up bytes buffer if needed
    if (clean) {
        m_free(bytes, len);
    }

    // Assert error if any
    pb_assert(err);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(customdevices_I2CDevice_write_obj, 0, customdevices_I2CDevice_write);


// dir(pybricks.customdevices.I2CDevice)
STATIC const mp_rom_map_elem_t customdevices_I2CDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),    MP_ROM_PTR(&customdevices_I2CDevice_read_obj)    },
    { MP_ROM_QSTR(MP_QSTR_write),   MP_ROM_PTR(&customdevices_I2CDevice_write_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(customdevices_I2CDevice_locals_dict, customdevices_I2CDevice_locals_dict_table);

// type(pybricks.customdevices.I2CDevice)
STATIC const mp_obj_type_t customdevices_I2CDevice_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2CDevice,
    .make_new = customdevices_I2CDevice_make_new,
    .locals_dict = (mp_obj_dict_t*)&customdevices_I2CDevice_locals_dict,
};

// pybricks.customdevices.UARTDevice class object
typedef struct _customdevices_UARTDevice_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
    pbio_serial_t *serial;
} customdevices_UARTDevice_obj_t;

// pybricks.customdevices.UARTDevice.__init__
STATIC mp_obj_t customdevices_UARTDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(baudrate),
        PB_ARG_DEFAULT_NONE(timeout)
    );
    customdevices_UARTDevice_obj_t *self = m_new_obj(customdevices_UARTDevice_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    // Get port number
    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    // Init UART port
    pbio_error_t err;
    while ((err = ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_CUSTOM_UART, port_num)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }

    // Get and init serial
    pb_assert(pbio_serial_get(
        &self->serial,
        self->iodev->port,
        pb_obj_get_int(baudrate),
        timeout == mp_const_none ? -1 : pb_obj_get_int(timeout)
    ));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.customdevices.UARTDevice.write
STATIC mp_obj_t customdevices_UARTDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(data)
    );

    customdevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    uint8_t *bytes;
    size_t len;
    bool clean = unpack_byte_arg(data, &bytes, &len);

    pbio_error_t err = pbio_serial_write(self->serial, bytes, len);

    if (clean) {
        m_free(bytes, len);
    }

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(customdevices_UARTDevice_write_obj, 0, customdevices_UARTDevice_write);

// pybricks.customdevices.UARTDevice.waiting
STATIC mp_obj_t customdevices_UARTDevice_waiting(mp_obj_t self_in) {
    customdevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t waiting;
    pb_assert(pbio_serial_in_waiting(self->serial, &waiting));
    return mp_obj_new_int(waiting);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_UARTDevice_waiting_obj, customdevices_UARTDevice_waiting);

// pybricks.customdevices.UARTDevice.read
STATIC mp_obj_t customdevices_UARTDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(length, 1)
    );

    customdevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // Get requested data length
    size_t len = mp_obj_get_int(length);
    size_t remaining = len;
    if (len > UART_MAX_LEN) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Read data into buffer
    uint8_t *buf = m_malloc(len);
    if (buf == NULL) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Start time
    int32_t time_start = mp_hal_ticks_ms();

    // Read up to the timeout
    pbio_error_t err;
    while ((err = pbio_serial_read(self->serial, buf, len, &remaining, time_start, mp_hal_ticks_ms())) == PBIO_ERROR_AGAIN) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(customdevices_UARTDevice_read_obj, 0, customdevices_UARTDevice_read);

// dir(pybricks.customdevices.UARTDevice)
STATIC const mp_rom_map_elem_t customdevices_UARTDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),  MP_ROM_PTR(&customdevices_UARTDevice_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),  MP_ROM_PTR(&customdevices_UARTDevice_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_waiting),MP_ROM_PTR(&customdevices_UARTDevice_waiting_obj) },
};
STATIC MP_DEFINE_CONST_DICT(customdevices_UARTDevice_locals_dict, customdevices_UARTDevice_locals_dict_table);

// type(pybricks.customdevices.UARTDevice)
STATIC const mp_obj_type_t customdevices_UARTDevice_type = {
    { &mp_type_type },
    .name = MP_QSTR_UARTDevice,
    .make_new = customdevices_UARTDevice_make_new,
    .locals_dict = (mp_obj_dict_t*)&customdevices_UARTDevice_locals_dict,
};

// pybricks.customdevices.Ev3devSensor class object
typedef struct _customdevices_Ev3devSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} customdevices_Ev3devSensor_obj_t;

// pybricks.customdevices.Ev3devSensor.__init__
STATIC mp_obj_t customdevices_Ev3devSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );
    customdevices_Ev3devSensor_obj_t *self = m_new_obj(customdevices_Ev3devSensor_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    // Get port number
    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    // Get the device
    pbio_error_t err;
    while ((err = ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3DEV_LEGO_SENSOR, port_num)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.customdevices.Ev3devSensor.mode
STATIC mp_obj_t customdevices_Ev3devSensor_mode(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(mode)
    );

    customdevices_Ev3devSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    uint8_t bin[PBIO_IODEV_MAX_DATA_SIZE];

    pb_assert(ev3device_get_values_at_mode(self->iodev, mp_obj_get_int(mode), bin));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(customdevices_Ev3devSensor_mode_obj, 0, customdevices_Ev3devSensor_mode);

// pybricks.customdevices.Ev3devSensor.bin
STATIC mp_obj_t customdevices_Ev3devSensor_bin(mp_obj_t self_in) {

    customdevices_Ev3devSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t bin[PBIO_IODEV_MAX_DATA_SIZE];
    memset(bin, 0, sizeof(bin));

    pb_assert(ev3device_get_values_at_mode(self->iodev, self->iodev->mode, bin));

    return mp_obj_new_bytes(bin, PBIO_IODEV_MAX_DATA_SIZE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_Ev3devSensor_bin_obj, customdevices_Ev3devSensor_bin);

// dir(pybricks.customdevices.Ev3devSensor)
STATIC const mp_rom_map_elem_t customdevices_Ev3devSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mode),   MP_ROM_PTR(&customdevices_Ev3devSensor_mode_obj)   },
    { MP_ROM_QSTR(MP_QSTR_bin),    MP_ROM_PTR(&customdevices_Ev3devSensor_bin_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(customdevices_Ev3devSensor_locals_dict, customdevices_Ev3devSensor_locals_dict_table);

// type(pybricks.customdevices.Ev3devSensor)
STATIC const mp_obj_type_t customdevices_Ev3devSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_Ev3devSensor,
    .make_new = customdevices_Ev3devSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&customdevices_Ev3devSensor_locals_dict,
};

// dir(pybricks.customdevices)
STATIC const mp_rom_map_elem_t customdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_customdevices)              },
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&customdevices_AnalogSensor_type)    },
    { MP_ROM_QSTR(MP_QSTR_Ev3devSensor),     MP_ROM_PTR(&customdevices_Ev3devSensor_type)    },
    { MP_ROM_QSTR(MP_QSTR_I2CDevice),        MP_ROM_PTR(&customdevices_I2CDevice_type   )    },
    { MP_ROM_QSTR(MP_QSTR_UARTDevice),       MP_ROM_PTR(&customdevices_UARTDevice_type  )    },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_customdevices_globals, customdevices_globals_table);
const mp_obj_module_t pb_module_customdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_customdevices_globals,
};
