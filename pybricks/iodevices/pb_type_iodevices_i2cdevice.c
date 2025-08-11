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

// Object representing a pybricks.iodevices.I2CDevice instance.
typedef struct {
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
    size_t write_len;
    /**
     * The read buffer is allocated as a bytes object when the operation
     * begins. It is returned to the user on completion.
     */
    mp_obj_str_t *read_result;
} device_obj_t;

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

    device_obj_t *self = mp_obj_malloc(device_obj_t, type);
    self->i2c_dev = i2c_dev;
    self->address = mp_obj_get_int(address_in);
    self->nxt_quirk = mp_obj_is_true(nxt_quirk_in);
    if (mp_obj_is_true(powered_in)) {
        pbio_port_p1p2_set_power(port, PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS);
    }

    return MP_OBJ_FROM_PTR(self);
}

// Object representing the iterable that is returned when calling an I2C
// method. This object can then be awaited (iterated). It has a reference to
// the device from which it was created. Only one operation can be active at
// one time.
typedef struct {
    mp_obj_base_t base;
    mp_obj_t device_obj;
} operation_obj_t;

static mp_obj_t operation_close(mp_obj_t self_in) {
    // Close is not implemented but needs to exist.
    operation_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;
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
    mp_obj_t result = MP_OBJ_FROM_PTR(device->read_result);
    // The device should not hold up garbage collection of the result.
    device->read_result = MP_OBJ_NULL;
    return result;
}

static mp_obj_t operation_iternext(mp_obj_t self_in) {
    operation_obj_t *self = MP_OBJ_TO_PTR(self_in);
    device_obj_t *device = MP_OBJ_TO_PTR(self->device_obj);

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

// pybricks.iodevices.I2CDevice.write_then_read
static mp_obj_t write_then_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        device_obj_t, self,
        PB_ARG_DEFAULT_NONE(write_data),
        PB_ARG_DEFAULT_INT(read_size, 0)
        );

    uint8_t *write_data = (uint8_t *)mp_obj_str_get_data(write_data_in, &self->write_len);

    self->state = 0;

    // Pre-allocate the return value so we have something to write the result to.
    self->read_result = mp_obj_malloc(mp_obj_str_t, &mp_type_bytes);
    self->read_result->len = mp_obj_get_int(read_size_in);
    self->read_result->hash = 0;
    self->read_result->data = m_new(byte, self->read_result->len);

    // Kick off the operation. This will immediately raise if a transaction is
    // in progress.
    pbio_error_t err = pbdrv_i2c_write_then_read(
        &self->state, self->i2c_dev, self->address,
        write_data, self->write_len,
        (uint8_t *)self->read_result->data, self->read_result->len, self->nxt_quirk);

    // Expect yield after the initial call.
    if (err == PBIO_SUCCESS) {
        pb_assert(PBIO_ERROR_FAILED);
    } else if (err != PBIO_ERROR_AGAIN) {
        pb_assert(err);
    }

    // If runloop active, return an awaitable object.
    if (pb_module_tools_run_loop_is_active()) {
        operation_obj_t *operation = mp_obj_malloc(operation_obj_t, &operation_type);
        operation->device_obj = MP_OBJ_FROM_PTR(self);
        return MP_OBJ_FROM_PTR(operation);
    }

    // Otherwise block and wait for the result here.
    while ((err = operation_iterate_once(self)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK;
    }
    pb_assert(err);

    return operation_get_result_obj(self);
}
// See also experimental_globals_table below. This function object is added there to make it importable.
static MP_DEFINE_CONST_FUN_OBJ_KW(write_then_read_obj, 0, write_then_read);

// dir(pybricks.iodevices.I2CDevice)
static const mp_rom_map_elem_t locals_dict_table[] = {
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
