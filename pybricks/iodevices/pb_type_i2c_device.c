// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_IODEVICES_I2CDEVICE

#include "py/mphal.h"
#include "py/objstr.h"


#include <pbdrv/i2c.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/iodevices/iodevices.h>
#include <pybricks/parameters.h>
#include <pybricks/tools/pb_type_async.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

/**
 * Object representing a pybricks.iodevices.I2CDevice instance.
 *
 * Also used by sensor classes for I2C Devices.
 */
typedef struct {
    mp_obj_base_t base;
    /**
     * Object that owns this I2C device, such as an Ultrasonic Sensor instance.
     * Gets passed to all return mappings.
     *
     * In case of the standalone I2CDevice class instance, this value is instead
     * used to store an optional user callable to map bytes to a return object.
     */
    mp_obj_t sensor_obj;
    /**
     * Generic reusable awaitable operation.
     */
    pb_type_async_t *iter;
    /**
     * The following are buffered parameters for one ongoing I2C operation, See
     * ::pbdrv_i2c_write_then_read for details on each parameter. We need to
     * buffer these arguments so we can keep calling the protothread until it
     * is complete. We don't need to buffer the write data here because it is
     * immediately copied to the driver on the first call to the protothread.
     */
    pbdrv_i2c_dev_t *i2c_dev;
    uint8_t address;
    bool nxt_quirk;
    size_t write_len;
    size_t read_len;
    uint8_t *read_buf;
    /**
     * Maps bytes read to the user return object.
     */
    pb_type_i2c_device_return_map_t return_map;
} device_obj_t;

// pybricks.iodevices.I2CDevice.__init__
mp_obj_t pb_type_i2c_device_make_new(mp_obj_t sensor_obj, mp_obj_t port_in, uint8_t address, bool custom, bool powered, bool nxt_quirk) {

    pb_module_tools_assert_blocking();

    // Get the port instance.
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_port_t *port;
    pb_assert(pbio_port_get_port(port_id, &port));

    // Set the port mode to LEGO DCM or raw I2C mode.
    pbio_error_t err = pbio_port_set_mode(port, custom ? PBIO_PORT_MODE_I2C : PBIO_PORT_MODE_LEGO_DCM);
    if (err == PBIO_ERROR_AGAIN) {
        // If coming from a different mode, give port some time to get started.
        // This happens when the user has a custom device and decides to switch
        // back to LEGO mode. This should be rare, so we can afford to wait.
        mp_hal_delay_ms(1000);
        err = pbio_port_set_mode(port, PBIO_PORT_MODE_LEGO_DCM);
    }
    pb_assert(err);

    pbdrv_i2c_dev_t *i2c_dev;
    pb_assert(pbio_port_get_i2c_dev(port, &i2c_dev));

    device_obj_t *device = mp_obj_malloc(device_obj_t, &pb_type_i2c_device);
    device->i2c_dev = i2c_dev;
    device->address = address;
    device->nxt_quirk = nxt_quirk;
    device->sensor_obj = sensor_obj;
    device->iter = NULL;
    if (powered) {
        pbio_port_p1p2_set_power(port, PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS);
    }

    return MP_OBJ_FROM_PTR(device);
}

// wrapper to parse args for __init__
static mp_obj_t make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address),
        PB_ARG_DEFAULT_FALSE(custom),
        PB_ARG_DEFAULT_FALSE(powered),
        PB_ARG_DEFAULT_FALSE(nxt_quirk)
        );

    return pb_type_i2c_device_make_new(
        MP_OBJ_NULL, // Not associated with any particular sensor instance.
        port_in,
        mp_obj_get_int(address_in),
        mp_obj_is_true(custom_in),
        mp_obj_is_true(powered_in),
        mp_obj_is_true(nxt_quirk_in)
        );
}

/**
 * This keeps calling the I2C protothread with cached parameters until completion.
 */
static pbio_error_t pb_type_i2c_device_iterate_once(pbio_os_state_t *state, mp_obj_t i2c_device_obj) {

    device_obj_t *device = MP_OBJ_TO_PTR(i2c_device_obj);

    return pbdrv_i2c_write_then_read(
        state, device->i2c_dev,
        device->address,
        NULL, // Already memcpy'd on initial iteration. No need to provide here.
        device->write_len,
        &device->read_buf,
        device->read_len,
        device->nxt_quirk
        );
}

/**
 * This is the callable form required by the shared awaitable code.
 *
 * For classes that have an I2C class instance such as the Ultrasonic Sensor,
 * the I2C object is not of interest, but rather the sensor object. So this
 * wrapper essentially passes the containing object to the return map.
 */
static mp_obj_t pb_type_i2c_device_return_generic(mp_obj_t i2c_device_obj) {
    device_obj_t *device = MP_OBJ_TO_PTR(i2c_device_obj);

    if (!device->return_map) {
        return mp_const_none;
    }

    return device->return_map(device->sensor_obj, device->read_buf, device->read_len);
}

mp_obj_t pb_type_i2c_device_start_operation(mp_obj_t i2c_device_obj, const uint8_t *write_data, size_t write_len, size_t read_len, pb_type_i2c_device_return_map_t return_map) {

    pb_assert_type(i2c_device_obj, &pb_type_i2c_device);
    device_obj_t *device = MP_OBJ_TO_PTR(i2c_device_obj);

    // Kick off the operation. This will immediately raise if a transaction is
    // in progress.
    pbio_os_state_t state = 0;
    uint8_t *read_buf = NULL;
    pbio_error_t err = pbdrv_i2c_write_then_read(
        &state, device->i2c_dev, device->address,
        (uint8_t *)write_data, write_len,
        &read_buf, read_len, device->nxt_quirk);

    // Expect yield after the initial call.
    if (err == PBIO_SUCCESS) {
        pb_assert(PBIO_ERROR_FAILED);
    } else if (err != PBIO_ERROR_AGAIN) {
        pb_assert(err);
    }

    // The initial operation above can fail if an I2C transaction is already in
    // progress. If so, we don't want to reset its state or allow the return
    // result to be garbage collected. Now that the first iteration succeeded,
    // save the state.
    device->read_len = read_len;
    device->write_len = write_len;
    device->read_buf = NULL;
    device->return_map = return_map;

    pb_type_async_t config = {
        .parent_obj = i2c_device_obj,
        .iter_once = pb_type_i2c_device_iterate_once,
        .state = state,
        .return_map = return_map ? pb_type_i2c_device_return_generic : NULL,
    };
    // New operation always wins; ongoing sound awaitable is cancelled.
    return pb_type_async_wait_or_await(&config, &device->iter, true);
}

/**
 * Helper utility to verify that expected device is attached by asserting an
 * expected manufacturer or id string. Raises ::PBIO_ERROR_NO_DEV if expected
 * string is not found.
 */
void pb_type_i2c_device_assert_string_at_register(mp_obj_t i2c_device_obj, uint8_t reg, const char *string) {

    device_obj_t *device = MP_OBJ_TO_PTR(i2c_device_obj);

    pb_module_tools_assert_blocking();

    size_t read_len = strlen(string);
    const uint8_t write_data[] = { reg };
    pb_type_i2c_device_start_operation(i2c_device_obj, write_data, MP_ARRAY_SIZE(write_data), read_len, NULL);

    if (memcmp(string, device->read_buf, read_len)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
}

/**
 * I2C result mapping that just returns a bytes object.
 */
static mp_obj_t pb_type_i2c_device_return_bytes(mp_obj_t self_in, const uint8_t *data, size_t len) {
    return mp_obj_new_bytes(data, len);
}

/**
 * I2C result mapping that calls user provided callback with self and bytes as argument.
 */
static mp_obj_t pb_type_i2c_device_return_user_map(mp_obj_t callable_obj, const uint8_t *data, size_t len) {
    // If user provides bound method, MicroPython takes care of providing
    // self as the first argument. We just need to pass in data arg.
    return mp_call_function_1(callable_obj, mp_obj_new_bytes(data, len));
}

// pybricks.iodevices.I2CDevice.read
static mp_obj_t read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_DEFAULT_NONE(reg),
        PB_ARG_DEFAULT_INT(length, 1),
        PB_ARG_DEFAULT_NONE(map)
        );

    // Write payload is one byte representing the register we want to read,
    // or no write for reg=None.
    uint8_t *write_data = reg_in == mp_const_none ?
        NULL :
        &(uint8_t) { mp_obj_get_int(reg_in) };
    size_t write_len = reg_in == mp_const_none ? 0 : 1;

    // Optional user provided callback method of the form def my_method(self, data)
    // We can use sensor_obj for this since it isn't used by I2CDevice instances,
    // and we are already passing this to the mapping anyway, so we can conviently
    // use it to pass the callable object in this case.
    device->sensor_obj = mp_obj_is_callable(map_in) ? map_in : MP_OBJ_NULL;

    return pb_type_i2c_device_start_operation(
        MP_OBJ_FROM_PTR(device),
        write_data,
        write_len,
        pb_obj_get_positive_int(length_in),
        mp_obj_is_callable(map_in) ? pb_type_i2c_device_return_user_map : pb_type_i2c_device_return_bytes
        );
}
static MP_DEFINE_CONST_FUN_OBJ_KW(read_obj, 0, read);

// pybricks.iodevices.I2CDevice.write
static mp_obj_t write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_DEFAULT_NONE(reg),
        PB_ARG_DEFAULT_NONE(data)
        );

    // Treat none data as empty bytes.
    if (data_in == mp_const_none) {
        data_in = mp_const_empty_bytes;
    }

    // Assert data is bytes.
    size_t user_len;
    const char *user_data = mp_obj_str_get_data(data_in, &user_len);

    // No register given, write data as is.
    if (reg_in == mp_const_none) {
        return pb_type_i2c_device_start_operation(MP_OBJ_FROM_PTR(device), (const uint8_t *)user_data, user_len, 0, NULL);
    }

    // Otherwise need to prefix write data with given register. We're limiting
    // write data to 32 bytes in this case. To send more data, the user can
    // use reg=None and prefix the address to the data themselves.
    uint8_t write_data[33];
    if (user_len > MP_ARRAY_SIZE(write_data) - 1) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    write_data[0] = pb_obj_get_positive_int(reg_in);
    memcpy(&write_data[1], user_data, user_len);

    return pb_type_i2c_device_start_operation(MP_OBJ_FROM_PTR(device), write_data, user_len + 1, 0, NULL);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(write_obj, 0, write);

// dir(pybricks.iodevices.I2CDevice)
static const mp_rom_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_obj) },
};
static MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

// type(pybricks.iodevices.I2CDevice)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_i2c_device,
    MP_QSTR_I2CDevice,
    MP_TYPE_FLAG_NONE,
    make_new, make_new,
    locals_dict, &locals_dict);

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_IODEVICES_I2CDEVICE
