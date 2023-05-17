// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include <pybricks/common.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_pb/pb_error.h>

#include <py/mphal.h>

static void pup_device_wait_ready(pbio_iodev_t *iodev) {
    pbio_error_t err;
    while ((err = pbio_iodev_is_ready(iodev)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
}

void pup_device_get_data(pbio_iodev_t *iodev, uint8_t mode, uint8_t **data) {
    pb_assert(pbio_iodev_set_mode(iodev, mode));
    pup_device_wait_ready(iodev);
    pb_assert(pbio_iodev_get_data(iodev, mode, data));
}

void pup_device_set_data(pbio_iodev_t *iodev, uint8_t mode, uint8_t *data) {
    pb_assert(pbio_iodev_set_mode(iodev, mode));
    pup_device_wait_ready(iodev);
    pb_assert(pbio_iodev_set_data(iodev, mode, data));
    pup_device_wait_ready(iodev);
}

pbio_iodev_t *pup_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id) {

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

void pup_device_setup_motor(pbio_port_id_t port, bool is_servo) {
    // HACK: Built-in motors on BOOST Move hub do not have I/O ports associated
    // with them.
    #if PYBRICKS_HUB_MOVEHUB
    if (port == PBIO_PORT_ID_A || port == PBIO_PORT_ID_B) {
        return;
    }
    #endif

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

    // Choose mode based on device capabilities.
    uint8_t mode_id = PBIO_IODEV_IS_ABS_MOTOR(iodev) ?
        PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB:
        PBIO_IODEV_MODE_PUP_REL_MOTOR__POS;

    // Activate mode.
    pb_assert(pbio_iodev_set_mode(iodev, mode_id));
    pup_device_wait_ready(iodev);
}

// REVISIT: Drop pb_device abstraction layer
void pb_device_setup_motor(pbio_port_id_t port, bool is_servo) {
    pup_device_setup_motor(port, is_servo);
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
