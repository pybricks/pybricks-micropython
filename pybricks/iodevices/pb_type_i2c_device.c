// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors
// Copyright (c) 2026 SparkFun Electronics

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES_I2C_DEVICE

#include "py/mphal.h"
#include "py/objstr.h"


#include <pbdrv/i2c.h>
#include <pbio/port.h>

#include <pybricks/common.h>
#include <pybricks/iodevices/iodevices.h>
#include <pybricks/parameters.h>
#include <pybricks/tools/pb_type_async.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#if PYBRICKS_PY_IODEVICES_I2C_DEVICE_ALLOC_BUFFER

/**
 * Default buffer size: max 255 bytes write followed by max 255 bytes read.
 */
#define I2C_DEVICE_BUFFER_SIZE_DEFAULT (512)

/**
 * Transaction buffer for one I2C bus, allocated on first use in a program.
 *
 * All I2C device objects on the same bus reference the same buffer object, so
 * it stays alive while any of them are in use. It is also attached to the
 * driver as its opaque container, so that new device objects can find it.
 */
typedef struct {
    mp_obj_base_t base;
    /**
     * The bus this buffer is attached to, so the finalizer can detach it.
     */
    pbdrv_i2c_dev_t *i2c_dev;
    size_t size;
    uint8_t data[];
} buffer_obj_t;

static mp_obj_t buffer_close(mp_obj_t self_in) {
    buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Ends any ongoing transaction at the next byte boundary, so the driver
    // stops accessing this memory before it is freed.
    pbdrv_i2c_set_buffer(self->i2c_dev, NULL, NULL, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(buffer_close_obj, buffer_close);

static const mp_rom_map_elem_t buffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&buffer_close_obj) },
};
static MP_DEFINE_CONST_DICT(buffer_locals_dict, buffer_locals_dict_table);

// Internal type, never exposed to the user.
static MP_DEFINE_CONST_OBJ_TYPE(pb_type_i2c_device_buffer,
    MP_QSTRnull, // never exposed to user.
    MP_TYPE_FLAG_NONE,
    locals_dict, &buffer_locals_dict);

/**
 * Gets the buffer attached to the given bus, or allocates and attaches one.
 *
 * @param [in]  i2c_dev     The I2C device.
 * @param [in]  size        Requested buffer size.
 * @return                  The buffer object.
 */
static mp_obj_t get_or_attach_buffer(pbdrv_i2c_dev_t *i2c_dev, size_t size) {
    buffer_obj_t *buffer = pbdrv_i2c_get_buffer_container(i2c_dev);
    if (buffer) {
        // Resizing an attached buffer is not supported.
        if (buffer->size != size) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
        return MP_OBJ_FROM_PTR(buffer);
    }

    buffer = mp_obj_malloc_var_with_finaliser(buffer_obj_t, data, uint8_t, size, &pb_type_i2c_device_buffer);
    buffer->i2c_dev = i2c_dev;
    buffer->size = size;
    pbdrv_i2c_set_buffer(i2c_dev, buffer, buffer->data, size);
    return MP_OBJ_FROM_PTR(buffer);
}

#endif // PYBRICKS_PY_IODEVICES_I2C_DEVICE_ALLOC_BUFFER

/**
 * Object representing a pybricks.iodevices.I2CDevice instance.
 */
typedef struct {
    mp_obj_base_t base;
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
    /**
     * Number of bytes used to send the register address (1 or 2).
     */
    uint8_t reg_len;
    size_t write_len;
    size_t read_len;
    uint8_t *read_buf;
    /**
     * Determines the result of the ongoing operation: MP_OBJ_NULL for no
     * result (write), mp_const_none to return the bytes read, or a user
     * callable that maps the bytes read to a return object.
     */
    mp_obj_t map;
    #if PYBRICKS_PY_IODEVICES_I2C_DEVICE_ALLOC_BUFFER
    /**
     * Transaction buffer shared by all devices on this bus. Not used directly,
     * but referenced here to keep it alive while this device exists.
     */
    mp_obj_t buffer;
    #endif
} device_obj_t;

// pybricks.iodevices.I2CDevice.__init__
static mp_obj_t make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address),
        PB_ARG_DEFAULT_INT(mode, PBIO_PORT_MODE_LEGO_DCM),
        PB_ARG_DEFAULT_INT(power_pin, PBIO_PORT_POWER_REQUIREMENTS_NONE),
        PB_ARG_DEFAULT_NONE(frequency),
        PB_ARG_DEFAULT_INT(reg_len, 1)
        );

    pb_module_tools_assert_blocking();

    pbio_port_mode_t mode = mp_obj_get_int(mode_in);
    if (mode != PBIO_PORT_MODE_LEGO_DCM && mode != PBIO_PORT_MODE_I2C_NXT && mode != PBIO_PORT_MODE_I2C_QWIIC) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Register addresses are sent as 1 or 2 bytes.
    mp_int_t reg_len = mp_obj_get_int(reg_len_in);
    if (reg_len != 1 && reg_len != 2) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // The frequency applies to all buses, so the most recent setting wins.
    // None (0) selects the platform default.
    pb_assert(pbdrv_i2c_set_frequency(frequency_in == mp_const_none ? 0 : pb_obj_get_positive_int(frequency_in)));

    // Get the port instance.
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_port_t *port;
    pb_assert(pbio_port_get_port(port_id, &port));

    // Set the port mode to LEGO DCM or raw I2C mode.
    pbio_error_t err = pbio_port_set_mode(port, mode);
    if (mode == PBIO_PORT_MODE_LEGO_DCM && err == PBIO_ERROR_AGAIN) {
        // If coming from a different mode, give port some time to get started.
        // This happens when the user has a custom device and decides to switch
        // back to LEGO mode. This should be rare, so we can afford to wait.
        mp_hal_delay_ms(1000);
        err = pbio_port_set_mode(port, PBIO_PORT_MODE_LEGO_DCM);
    }
    pb_assert(err);

    // In LEGO mode, wait for the port to finish scanning for known devices.
    pbdrv_i2c_dev_t *i2c_dev;
    while ((err = pbio_port_get_i2c_dev(port, &i2c_dev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    pb_assert(err);

    #if PYBRICKS_PY_IODEVICES_I2C_DEVICE_ALLOC_BUFFER
    mp_obj_t buffer = get_or_attach_buffer(i2c_dev, I2C_DEVICE_BUFFER_SIZE_DEFAULT);
    #endif

    device_obj_t *device = mp_obj_malloc(device_obj_t, &pb_type_i2c_device);
    #if PYBRICKS_PY_IODEVICES_I2C_DEVICE_ALLOC_BUFFER
    device->buffer = buffer;
    #endif
    device->i2c_dev = i2c_dev;
    device->address = mp_obj_get_int(address_in);
    device->reg_len = reg_len;
    device->iter = NULL;
    pbio_port_p1p2_set_power(port, pb_module_iodevices_get_requested_power_pin(power_pin_in));

    return MP_OBJ_FROM_PTR(device);
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
        false
        );
}

/**
 * Maps the bytes read to the result of a read operation: either the bytes
 * themselves or the result of the user provided callable.
 */
static mp_obj_t pb_type_i2c_device_return_map(mp_obj_t i2c_device_obj) {
    device_obj_t *device = MP_OBJ_TO_PTR(i2c_device_obj);

    mp_obj_t data = mp_obj_new_bytes(device->read_buf, device->read_len);

    // If user provides bound method, MicroPython takes care of providing
    // self as the first argument. We just need to pass in data arg.
    return device->map == mp_const_none ? data : mp_call_function_1(device->map, data);
}

/**
 * Starts an I2C operation.
 *
 * @param [in]  device      The I2C device.
 * @param [in]  write_data  Data to write.
 * @param [in]  write_len   Number of bytes to write.
 * @param [in]  read_len    Number of bytes to read.
 * @param [in]  map         MP_OBJ_NULL for no result, mp_const_none to return the bytes read, or a callable to map them.
 * @return                  An awaitable if the runloop is active, otherwise the result.
 */
static mp_obj_t pb_type_i2c_device_start_operation(device_obj_t *device, const uint8_t *write_data, size_t write_len, size_t read_len, mp_obj_t map) {

    // Kick off the operation. This will immediately raise if a transaction is
    // in progress.
    pbio_os_state_t state = 0;
    uint8_t *read_buf = NULL;
    pbio_error_t err = pbdrv_i2c_write_then_read(
        &state, device->i2c_dev, device->address,
        (uint8_t *)write_data, write_len,
        &read_buf, read_len, false);

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
    device->map = map;

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(device),
        .iter_once = pb_type_i2c_device_iterate_once,
        .state = state,
        .return_map = map != MP_OBJ_NULL ? pb_type_i2c_device_return_map : NULL,
    };
    // New operation always wins; ongoing sound awaitable is cancelled.
    return pb_type_async_wait_or_await(&config, &device->iter, true);
}

/**
 * Packs the register address into a buffer, most significant byte first.
 *
 * @param [in]  device      The I2C device.
 * @param [in]  reg_in      The register address, or None for no register.
 * @param [out] buf         Buffer of at least 2 bytes.
 * @return                  Number of bytes packed: 0 for None, else reg_len.
 */
static size_t pb_type_i2c_device_pack_reg(device_obj_t *device, mp_obj_t reg_in, uint8_t *buf) {
    if (reg_in == mp_const_none) {
        return 0;
    }
    mp_int_t reg = pb_obj_get_positive_int(reg_in);
    if (reg >> (8 * device->reg_len)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    if (device->reg_len == 2) {
        buf[0] = reg >> 8;
        buf[1] = reg;
        return 2;
    }
    buf[0] = reg;
    return 1;
}

// pybricks.iodevices.I2CDevice.read
static mp_obj_t read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_DEFAULT_NONE(reg),
        PB_ARG_DEFAULT_INT(length, 1),
        PB_ARG_DEFAULT_NONE(map)
        );

    // Write payload is the register we want to read, or no write for reg=None.
    uint8_t write_data[2];
    size_t write_len = pb_type_i2c_device_pack_reg(device, reg_in, write_data);

    // Optional user provided callback method of the form def my_method(self, data)
    // or None to return the bytes as is.
    return pb_type_i2c_device_start_operation(
        device,
        write_data,
        write_len,
        pb_obj_get_positive_int(length_in),
        mp_obj_is_callable(map_in) ? map_in : mp_const_none
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
        return pb_type_i2c_device_start_operation(device, (const uint8_t *)user_data, user_len, 0, MP_OBJ_NULL);
    }

    // Otherwise need to prefix write data with given register. The buffer is
    // sized for the maximum write length supported by the drivers. Drivers
    // with a lower limit will raise when the operation starts.
    uint8_t write_data[UINT8_MAX];
    size_t reg_len = pb_type_i2c_device_pack_reg(device, reg_in, write_data);
    if (user_len > MP_ARRAY_SIZE(write_data) - reg_len) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    memcpy(&write_data[reg_len], user_data, user_len);

    return pb_type_i2c_device_start_operation(device, write_data, reg_len + user_len, 0, MP_OBJ_NULL);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(write_obj, 0, write);

// dir(pybricks.iodevices.I2CDevice)
static const mp_rom_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_obj) },
    { MP_ROM_QSTR(MP_QSTR_AUTOMATIC), MP_ROM_INT(PBIO_PORT_MODE_LEGO_DCM)},
    { MP_ROM_QSTR(MP_QSTR_CUSTOM_NXT), MP_ROM_INT(PBIO_PORT_MODE_I2C_NXT)},
    { MP_ROM_QSTR(MP_QSTR_CUSTOM_QWIIC), MP_ROM_INT(PBIO_PORT_MODE_I2C_QWIIC)},
};
static MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

// type(pybricks.iodevices.I2CDevice)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_i2c_device,
    MP_QSTR_I2CDevice,
    MP_TYPE_FLAG_NONE,
    make_new, make_new,
    locals_dict, &locals_dict);

#endif // PYBRICKS_PY_IODEVICES_I2C_DEVICE
