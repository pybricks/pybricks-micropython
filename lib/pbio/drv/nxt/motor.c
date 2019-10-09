// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR

#include <stdbool.h>

#include <pbdrv/motor.h>
#include <pbio/config.h>

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)

inline void _pbdrv_motor_init(void) { }

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
}
#endif

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    *id = PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_MOTOR
