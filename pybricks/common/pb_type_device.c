// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_DEVICES

#include <pbdrv/legodev.h>
#include <pbdrv/legodev.h>

#include <pybricks/common.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_pb/pb_error.h>

#include <py/runtime.h>
#include <py/mphal.h>

/**
 * Non-blocking version of powered up data getter. Will raise exception if
 * sensor is not already in the right mode.
 *
 * Object @p self_in must be of pb_type_device_obj_base_t type or equivalent.
 *
 * @param [in]  sensor      The powered up device.
 * @param [in]  mode        Desired mode.
 * @return                  Void pointer to data.
 */
void *pb_type_device_get_data(mp_obj_t self_in, uint8_t mode) {
    pb_type_device_obj_base_t *sensor = MP_OBJ_TO_PTR(self_in);
    void *data;
    pb_assert(pbdrv_legodev_get_data(sensor->legodev, mode, &data));
    return data;
}

/**
 * Always-blocking version of powered up data getter. Can be used during sensor
 * or motor initialization.
 *
 * Object @p self_in must be of pb_type_device_obj_base_t type or equivalent.
 *
 * @param [in]  sensor      The powered up device.
 * @param [in]  mode        Desired mode.
 * @return                  Void pointer to data.
 */
void *pb_type_device_get_data_blocking(mp_obj_t self_in, uint8_t mode) {
    pb_type_device_obj_base_t *sensor = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbdrv_legodev_set_mode(sensor->legodev, mode));
    pbio_error_t err;
    while ((err = pbdrv_legodev_is_ready(sensor->legodev)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
    void *data;
    pb_assert(pbdrv_legodev_get_data(sensor->legodev, mode, &data));
    return data;
}

/**
 * Tests that a Powered Up device read or write operation has completed.
 * For reading, this means that the mode has been set and the first data for
 * the mode is ready. For writing, this means that the mode has been set and
 * data has been written to the device, including the neccessary delays for
 * discarding stale data or the time needed to externally process written data.
 *
 * @param [in]  self_in     The sensor object instance.
 * @param [in]  end_time    Not used.
 * @return                  True if operation is complete (device ready),
 *                          false otherwise.
 */
static bool pb_pup_device_test_completion(mp_obj_t self_in, uint32_t end_time) {
    pb_type_device_obj_base_t *sensor = MP_OBJ_TO_PTR(self_in);
    pbio_error_t err = pbdrv_legodev_is_ready(sensor->legodev);
    if (err == PBIO_ERROR_AGAIN) {
        return false;
    }
    pb_assert(err);
    return true;
}

/**
 * Implements calling of async sensor methods. This is called when a (constant)
 * entry of pb_type_device_method type in a sensor class is called. It is
 * responsible for setting the sensor mode and returning an awaitable object.
 *
 * This is also called in a few places where a simple constant sensor method
 * is not sufficient, where additional wrapping code is used to dynamically
 * set the mode or return mapping, such as in the multi-purpose PUPDevice, or
 * the ColorSensor class variants, where different modes or mappings are needed
 * for a single method depending on a keyword argument.
 */
mp_obj_t pb_type_device_method_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    assert(mp_obj_is_type(self_in, &pb_type_device_method));
    pb_type_device_method_obj_t *method = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_t sensor_in = args[0];
    pb_type_device_obj_base_t *sensor = MP_OBJ_TO_PTR(sensor_in);
    pb_assert(pbdrv_legodev_set_mode(sensor->legodev, method->mode));

    return pb_type_awaitable_await_or_wait(
        sensor_in,
        sensor->awaitables,
        pb_type_awaitable_end_time_none,
        pb_pup_device_test_completion,
        method->get_values,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_NONE);
}

/**
 * Function-like callable type for async sensor methods. This type is used for
 * constant pb_type_device_method_obj_t instances, which store a sensor mode to
 * set and a mapping function to create a return object when that data is ready.
 */
MP_DEFINE_CONST_OBJ_TYPE(
    pb_type_device_method, MP_QSTR_function, MP_TYPE_FLAG_BINDS_SELF | MP_TYPE_FLAG_BUILTIN_FUN,
    call, pb_type_device_method_call,
    unary_op, mp_generic_unary_op
    );

/**
 * Set data for a Powered Up device, such as the brightness of multiple external
 * lights on a sensor. Automatically sets the mode if not already set. Returns
 * an awaitable object that can be used to wait for the operation to complete.
 *
 * @param [in]  sensor      The powered up device.
 * @param [in]  mode        Desired mode.
 * @param [in]  data        Data to set.
 * @param [in]  size        Size of data.
 * @return                  Awaitable object.
 */
mp_obj_t pb_type_device_set_data(pb_type_device_obj_base_t *sensor, uint8_t mode, const void *data, uint8_t size) {
    pb_assert(pbdrv_legodev_set_mode_with_data(sensor->legodev, mode, data, size));
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(sensor),
        sensor->awaitables,
        pb_type_awaitable_end_time_none,
        pb_pup_device_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_RAISE_ON_BUSY);
}

pbdrv_legodev_type_id_t pb_type_device_init_class(pb_type_device_obj_base_t *self, mp_obj_t port_in, pbdrv_legodev_type_id_t valid_id) {

    pb_module_tools_assert_blocking();

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_error_t err;
    pbdrv_legodev_type_id_t actual_id = valid_id;
    while ((err = pbdrv_legodev_get_device(port, &actual_id, &self->legodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);
    self->awaitables = mp_obj_new_list(0, NULL);
    return actual_id;
}

#endif // PYBRICKS_PY_PUPDEVICES
