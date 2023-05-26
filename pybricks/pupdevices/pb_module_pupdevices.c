// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/uartdev.h>

#include <pybricks/common.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_pb/pb_error.h>

#include <py/runtime.h>
#include <py/mphal.h>

/**
 * Non-blocking version of powered up data getter. Will raise exception if
 * sensor is not already in the right mode.
 *
 * @param [in]  sensor      The powered up device.
 * @param [in]  mode        Desired mode.
 * @return                  Void pointer to data.
 */
void *pb_pupdevices_get_data(mp_obj_t self_in, uint8_t mode) {
    pb_pupdevices_obj_base_t *sensor = MP_OBJ_TO_PTR(self_in);
    void *data;
    pb_assert(pbio_uartdev_get_data(sensor->iodev, mode, &data));
    return data;
}

/**
 * Always-blocking version of powered up data getter. Can be used during sensor
 * or motor initialization.
 *
 * @param [in]  sensor      The powered up device.
 * @param [in]  mode        Desired mode.
 * @return                  Void pointer to data.
 */
void *pb_pupdevices_get_data_blocking(pb_pupdevices_obj_base_t *sensor, uint8_t mode) {
    pb_assert(pbio_uartdev_set_mode(sensor->iodev, mode));
    pbio_error_t err;
    while ((err = pbio_uartdev_is_ready(sensor->iodev)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
    void *data;
    pb_assert(pbio_uartdev_get_data(sensor->iodev, mode, &data));
    return data;
}

STATIC bool pb_pup_device_test_completion(mp_obj_t self_in, uint32_t end_time) {
    pb_pupdevices_obj_base_t *sensor = MP_OBJ_TO_PTR(self_in);
    pbio_error_t err = pbio_uartdev_is_ready(sensor->iodev);
    if (err == PBIO_ERROR_AGAIN) {
        return false;
    }
    pb_assert(err);
    return true;
}

mp_obj_t pb_pupdevices_method_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    assert(mp_obj_is_type(self_in, &pb_type_pupdevices_method));
    pb_obj_pupdevices_method_t *method = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_t sensor_in = args[0];
    pb_pupdevices_obj_base_t *sensor = MP_OBJ_TO_PTR(sensor_in);
    pb_assert(pbio_uartdev_set_mode(sensor->iodev, method->mode));

    return pb_type_awaitable_await_or_wait(
        sensor_in,
        sensor->awaitables,
        pb_type_awaitable_end_time_none,
        pb_pup_device_test_completion,
        method->get_values,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_NONE);
}

MP_DEFINE_CONST_OBJ_TYPE(
    pb_type_pupdevices_method, MP_QSTR_function, MP_TYPE_FLAG_BINDS_SELF | MP_TYPE_FLAG_BUILTIN_FUN,
    call, pb_pupdevices_method_call,
    unary_op, mp_generic_unary_op
    );

mp_obj_t pb_pupdevices_set_data(pb_pupdevices_obj_base_t *sensor, uint8_t mode, const void *data) {
    pb_assert(pbio_uartdev_set_mode_with_data(sensor->iodev, mode, data));
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(sensor),
        sensor->awaitables,
        pb_type_awaitable_end_time_none,
        pb_pup_device_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_RAISE_ON_BUSY);
}

void pb_pupdevices_init_class(pb_pupdevices_obj_base_t *self, mp_obj_t port_in, pbio_iodev_type_id_t valid_id) {

    pb_module_tools_assert_blocking();

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    pbio_error_t err;
    while ((err = pbdrv_ioport_get_iodev(port, &self->iodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);

    // Verify the ID or always allow generic LUMP device
    if (self->iodev->info->type_id != valid_id && valid_id != PBIO_IODEV_TYPE_ID_LUMP_UART) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    self->awaitables = mp_obj_new_list(0, NULL);
}

// Delete me: refactor getter only still used for passive external light.
pbio_iodev_t *pb_pup_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id) {

    pbio_iodev_t *iodev;
    pbio_error_t err;
    while ((err = pbdrv_ioport_get_iodev(port, &iodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);

    // Verify the ID or always allow generic LUMP device
    if (iodev->info->type_id != valid_id && valid_id != PBIO_IODEV_TYPE_ID_LUMP_UART) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    return iodev;
}

// Revisit: get rid of this abstraction
void pb_pup_device_setup_motor(pbio_port_id_t port, bool is_servo) {
    // HACK: Built-in motors on BOOST Move hub do not have I/O ports associated
    // with them.
    #if PYBRICKS_HUB_MOVEHUB
    if (port == PBIO_PORT_ID_A || port == PBIO_PORT_ID_B) {
        return;
    }
    #endif

    pb_module_tools_assert_blocking();

    // Get the iodevice
    pbio_iodev_t *iodev;
    pbio_error_t err;

    // Set up device
    while ((err = pbdrv_ioport_get_iodev(port, &iodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);

    // Only motors are allowed.
    if (!PBIO_IODEV_IS_DC_OUTPUT(iodev)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // If it's a DC motor, no further setup is needed.
    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(iodev)) {
        return;
    }

    // FIXME: there is no guarantee that the iodev is a uartdev
    #if !PYBRICKS_HUB_VIRTUALHUB

    // Choose mode based on device capabilities.
    uint8_t mode_id = PBIO_IODEV_IS_ABS_MOTOR(iodev) ?
        PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB:
        PBIO_IODEV_MODE_PUP_REL_MOTOR__POS;

    // Activate mode.
    pb_pupdevices_obj_base_t device = {
        .iodev = iodev,
    };
    pb_pupdevices_get_data_blocking(&device, mode_id);

    #endif // !PYBRICKS_HUB_VIRTUALHUB
}

// REVISIT: Drop pb_device abstraction layer
void pb_device_setup_motor(pbio_port_id_t port, bool is_servo) {
    pb_pup_device_setup_motor(port, is_servo);
}

STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pupdevices)                    },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&pb_type_Motor)                         },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),             MP_ROM_PTR(&pb_type_DCMotor)                       },
    #endif
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pb_type_pupdevices_ColorDistanceSensor)},
    { MP_ROM_QSTR(MP_QSTR_ColorLightMatrix),    MP_ROM_PTR(&pb_type_pupdevices_ColorLightMatrix)   },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),         MP_ROM_PTR(&pb_type_pupdevices_ColorSensor)        },
    { MP_ROM_QSTR(MP_QSTR_ForceSensor),         MP_ROM_PTR(&pb_type_pupdevices_ForceSensor)        },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),      MP_ROM_PTR(&pb_type_pupdevices_InfraredSensor)     },
    { MP_ROM_QSTR(MP_QSTR_Light),               MP_ROM_PTR(&pb_type_pupdevices_Light)              },
    { MP_ROM_QSTR(MP_QSTR_PFMotor),             MP_ROM_PTR(&pb_type_pupdevices_PFMotor)            },
    { MP_ROM_QSTR(MP_QSTR_Remote),              MP_ROM_PTR(&pb_type_pupdevices_Remote)             },
    { MP_ROM_QSTR(MP_QSTR_TiltSensor),          MP_ROM_PTR(&pb_type_pupdevices_TiltSensor)         },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),    MP_ROM_PTR(&pb_type_pupdevices_UltrasonicSensor)   },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_pupdevices_globals, pupdevices_globals_table);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_pupdevices_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_pupdevices, pb_module_pupdevices);

#endif // PYBRICKS_PY_PUPDEVICES
