// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup Port pbio/port: I/O port interface
 * @{
 */

#ifndef _PBIO_PORT_INTERFACE_H_
#define _PBIO_PORT_INTERFACE_H_

// REVISIT: Merge to pbio/port, but this is currently a circular dependency.

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/error.h>

#include <pbio/angle.h>
#include <pbio/dcmotor.h>
#include <pbio/servo.h>

#include <pbio/port_lump.h>


/**
 * Port modes. Use a single value for setting a mode. Use as flags to indicate
 * capabilities for a port.
 */
typedef enum {
    /**
     * No mode specified.
     */
    PBIO_PORT_MODE_NONE = 0,
    /**
     * The port is in LEGO Powered Up mode, auto-detecting official active and
     * passive components. Runs LEGO UART Messaging protocol when a LUMP device
     * is detected.
     */
    PBIO_PORT_MODE_LEGO_PUP = 1 << 0,
    /**
     * The port acts as a quadrature encoder, counting position changes without
     * any device presence or type detection. Provides access to the counter
     * driver of the device that is expected to be always running for this port.
     */
    PBIO_PORT_MODE_QUADRATURE_PASSIVE = 1 << 1,
    /**
     * The port is in direct access UART mode. No port process is running. User
     * code may access the UART device directly.
     */
    PBIO_PORT_MODE_UART = 1 << 2,
} pbio_port_mode_t;

#if PBIO_CONFIG_PORT

void pbio_port_init(void);

void pbio_port_power_off(void);

void pbio_port_stop_user_actions(bool reset);

pbio_error_t pbio_port_get_port(pbio_port_id_t id, pbio_port_t **port);

pbio_error_t pbio_port_get_dcmotor(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_dcmotor_t **dcmotor);

pbio_error_t pbio_port_get_servo(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_servo_t **servo);

pbio_error_t pbio_port_get_lump_device(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_port_lump_dev_t **lump_dev);

pbio_error_t pbio_port_get_angle(pbio_port_t *port, pbio_angle_t *angle);

pbio_error_t pbio_port_get_abs_angle(pbio_port_t *port, pbio_angle_t *angle);

pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement);

void pbio_port_process_poll(void *port);

pbio_error_t pbio_port_set_mode(pbio_port_t *port, pbio_port_mode_t mode);

#else // PBIO_CONFIG_PORT

static inline void pbio_port_init(void) {
}

static inline void pbio_port_power_off(void) {
}

static inline void pbio_port_stop_user_actions(bool reset) {
}

static inline pbio_error_t pbio_port_get_port(pbio_port_id_t id, pbio_port_t **port) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_get_dcmotor(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_dcmotor_t **dcmotor) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_get_servo(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_servo_t **servo) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_get_lump_device(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_port_lump_dev_t **lump_dev) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_get_angle(pbio_port_t *port, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_get_abs_angle(pbio_port_t *port, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_port_process_poll(void *port) {
}

#endif // PBIO_CONFIG_PORT

#endif // _PBIO_PORT_INTERFACE_H_

/** @} */
