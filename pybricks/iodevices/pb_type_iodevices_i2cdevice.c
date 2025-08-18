// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_IODEVICES_I2CDEVICE

#include "py/mphal.h"
#include "py/objstr.h"


#include <pbdrv/i2c.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/util_pb/pb_error.h>

typedef struct _device_obj_t device_obj_t;

/**
 * Given a completed I2C operation, maps the resulting read buffer to an object
 * of a desired form. For example, it could map two bytes to a single floating
 * point value representing temperature.
 *
 * @param [in]  device   The device object.
 */
typedef mp_obj_t (*return_map_t)(device_obj_t *device);

// Object representing a pybricks.iodevices.I2CDevice instance.
struct _device_obj_t {
    mp_obj_base_t base;
    /**
     * The following are buffered parameters for one ongoing I2C operation, See
     * ::pbdrv_i2c_write_then_read for details on each parameter. We need to
     * buffer these arguments so we can keep calling the protothread until it
     * is complete. We don't need to buffer the write buffer here because it is
     * immediately copied to the driver on the first call to the protothread.
     */
    pbdrv_i2c_dev_t *i2c_dev;
    pbio_os_state_t state;
    uint8_t address;
    bool nxt_quirk;
    return_map_t return_map;
    size_t write_len;
    /**
     * The read buffer is allocated as a bytes object when the operation
     * begins. It is returned to the user on completion.
     */
    mp_obj_str_t *read_result;
};

// pybricks.iodevices.I2CDevice.__init__
static mp_obj_t make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(address),
        PB_ARG_DEFAULT_FALSE(custom),
        PB_ARG_DEFAULT_FALSE(powered),
        PB_ARG_DEFAULT_FALSE(nxt_quirk)
        );

    pb_module_tools_assert_blocking();

    // Get the port instance.
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_port_t *port;
    pb_assert(pbio_port_get_port(port_id, &port));

    // Set the port mode to LEGO DCM or raw I2C mode.
    pbio_error_t err = pbio_port_set_mode(port, mp_obj_is_true(custom_in) ? PBIO_PORT_MODE_I2C : PBIO_PORT_MODE_LEGO_DCM);
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

    device_obj_t *device = mp_obj_malloc(device_obj_t, type);
    device->i2c_dev = i2c_dev;
    device->address = mp_obj_get_int(address_in);
    device->nxt_quirk = mp_obj_is_true(nxt_quirk_in);
    if (mp_obj_is_true(powered_in)) {
        pbio_port_p1p2_set_power(port, PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS);
    }

    return MP_OBJ_FROM_PTR(device);
}

// Object representing the iterable that is returned when calling an I2C
// method. This object can then be awaited (iterated). It has a reference to
// the device from which it was created. Only one operation can be active at
// one time.
typedef struct {
    mp_obj_base_t base;
    mp_obj_t device_obj;
} operation_obj_t;

static mp_obj_t operation_close(mp_obj_t op_in) {
    // Close is not implemented but needs to exist.
    operation_obj_t *op = MP_OBJ_TO_PTR(op_in);
    (void)op;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(operation_close_obj, operation_close);

static pbio_error_t operation_iterate_once(device_obj_t *device) {
    return pbdrv_i2c_write_then_read(
        &device->state, device->i2c_dev,
        device->address,
        NULL, // Already memcpy'd on initial iteration. No need to provide here.
        device->write_len,
        (uint8_t *)device->read_result->data,
        device->read_result->len,
        device->nxt_quirk
        );
}

static mp_obj_t operation_get_result_obj(device_obj_t *device) {
    mp_obj_t result = device->return_map ? device->return_map(device) : mp_const_none;
    // The device should not hold up garbage collection of the result.
    device->read_result = MP_OBJ_NULL;
    return result;
}

static mp_obj_t operation_iternext(mp_obj_t op_in) {
    operation_obj_t *op = MP_OBJ_TO_PTR(op_in);
    device_obj_t *device = MP_OBJ_TO_PTR(op->device_obj);

    pbio_error_t err = operation_iterate_once(device);

    // Yielded, keep going.
    if (err == PBIO_ERROR_AGAIN) {
        return mp_const_none;
    }

    // Raises on Timeout and other I/O errors. Proceeds on success.
    pb_assert(err);

    // Set return value via stop iteration.
    return mp_make_stop_iteration(operation_get_result_obj(device));
}

static const mp_rom_map_elem_t operation_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&operation_close_obj) },
};
MP_DEFINE_CONST_DICT(operation_locals_dict, operation_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(operation_type,
    MP_QSTR_I2COperation,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, operation_iternext,
    locals_dict, &operation_locals_dict);

static mp_obj_t start_operation(device_obj_t *device, const uint8_t *write_data, size_t write_len, size_t read_len, return_map_t return_map) {

    device->write_len = write_len;

    mp_obj_str_t *read_result;
    if (read_len) {
        read_result = mp_obj_new_bytes(NULL, read_len);
        read_result->hash = 0;
        read_result->data = m_new(byte, read_result->len);
    } else {
        read_result = (mp_obj_str_t *)&mp_const_empty_bytes_obj;
    }

    // Kick off the operation. This will immediately raise if a transaction is
    // in progress.
    pbio_os_state_t state = 0;
    pbio_error_t err = pbdrv_i2c_write_then_read(
        &state, device->i2c_dev, device->address,
        (uint8_t *)write_data, device->write_len,
        (uint8_t *)read_result->data, read_result->len, device->nxt_quirk);

    // Expect yield after the initial call.
    if (err == PBIO_SUCCESS) {
        pb_assert(PBIO_ERROR_FAILED);
    } else if (err != PBIO_ERROR_AGAIN) {
        pb_assert(err);
    }

    // The initial operation above can fail if an I2C transaction is already in
    // progress. If so, we don't want to reset it state or allow the return
    // result to be garbage collected. Now that the first iteration succeeded,
    // save the state and assign the new result buffer.
    device->state = state;
    device->read_result = read_result;
    device->return_map = return_map;

    // If runloop active, return an awaitable object.
    if (pb_module_tools_run_loop_is_active()) {
        operation_obj_t *operation = mp_obj_malloc(operation_obj_t, &operation_type);
        operation->device_obj = MP_OBJ_FROM_PTR(device);
        return MP_OBJ_FROM_PTR(operation);
    }

    // Otherwise block and wait for the result here.
    while ((err = operation_iterate_once(device)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK;
    }
    pb_assert(err);

    return operation_get_result_obj(device);
}

static mp_obj_t return_map_bytes(device_obj_t *device) {
    return MP_OBJ_FROM_PTR(device->read_result);
}

// pybricks.iodevices.I2CDevice.write_then_read
static mp_obj_t write_then_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_REQUIRED(write_data),
        PB_ARG_REQUIRED(read_length)
        );

    size_t write_len;
    return start_operation(device, (const uint8_t *)mp_obj_str_get_data(write_data_in, &write_len), write_len, pb_obj_get_positive_int(read_length_in), return_map_bytes);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(write_then_read_obj, 0, write_then_read);

// pybricks.iodevices.I2CDevice.read
static mp_obj_t read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_DEFAULT_NONE(reg),
        PB_ARG_DEFAULT_INT(length, 1)
        );

    // Write payload is one byte representing the register we want to read,
    // or no write for reg=None.
    const mp_obj_str_t *write_data = reg_in == mp_const_none ?
        &mp_const_empty_bytes_obj :
        &(mp_obj_str_t) {
        .base = {
            .type = &mp_type_bytes,
        },
        .hash = 0,
        .len = 1,
        .data = &(const byte) {
            mp_obj_get_int(reg_in),
        },
    };

    // Call write_then_read with parsed arguments.
    const mp_obj_t write_then_read_args[] = {
        MP_OBJ_FROM_PTR(device),
        MP_OBJ_FROM_PTR(write_data),
        length_in,
    };
    return write_then_read(MP_ARRAY_SIZE(write_then_read_args), write_then_read_args, (mp_map_t *)&mp_const_empty_map);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(read_obj, 0, read);

// pybricks.iodevices.I2CDevice.write
static mp_obj_t write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, device,
        PB_ARG_DEFAULT_NONE(reg),
        PB_ARG_DEFAULT_NONE(data)
        );

    // Treat none as empty bytes, needed for common operation.
    if (data_in == mp_const_none) {
        data_in = mp_const_empty_bytes;
    }

    // If a register is provided, we need to concatenate it with write data.
    if (reg_in != mp_const_none) {
        size_t original_len;
        const char *original_data = mp_obj_str_get_data(data_in, &original_len);
        mp_obj_str_t *reg_and_data = mp_obj_new_bytes(NULL, original_len + 1);
        byte *bytes = m_new(byte, reg_and_data->len);
        bytes[0] = pb_obj_get_positive_int(reg_in);
        memcpy(&reg_and_data[1], original_data, original_len);
        data_in = MP_OBJ_FROM_PTR(reg_and_data);
    }

    // Call write_then_read with parsed arguments.
    const mp_obj_t write_then_read_args[] = {
        MP_OBJ_FROM_PTR(device),
        data_in,
        MP_OBJ_NEW_SMALL_INT(0),
    };
    return write_then_read(MP_ARRAY_SIZE(write_then_read_args), write_then_read_args, (mp_map_t *)&mp_const_empty_map);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(write_obj, 0, write);

// dir(pybricks.iodevices.I2CDevice)
static const mp_rom_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_then_read), MP_ROM_PTR(&write_then_read_obj) },
};
static MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

// type(pybricks.iodevices.I2CDevice)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_I2CDevice,
    MP_QSTR_I2CDevice,
    MP_TYPE_FLAG_NONE,
    make_new, make_new,
    locals_dict, &locals_dict);

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_IODEVICES_I2CDEVICE
