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
#include <pbdrv/uart.h>
#include <pbdrv/i2c.h>

#include <pbio/error.h>

#include <pbio/angle.h>
#include <pbio/dcmotor.h>
#include <pbio/servo.h>

#include <pbio/port_lump.h>
#include <pbio/port_dcm.h>


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
     * The port is in LEGO mode, auto-detecting official active and passive
     * components. Runs LEGO UART Messaging protocol when a LUMP device is
     * detected. Runs background process for analog light sensors, etc.
     */
    PBIO_PORT_MODE_LEGO_DCM = 1 << 0,
    /**
     * The port acts as a quadrature encoder, counting position changes. May
     * also provide connected device type information on some platforms.
     */
    PBIO_PORT_MODE_QUADRATURE = 1 << 1,
    /**
     * The port is in direct access UART mode. No port process is running. User
     * code may access the UART device directly.
     */
    PBIO_PORT_MODE_UART = 1 << 2,
    /**
     * The port is in I2C mode. No port process is running. User code may access
     * the I2C device directly.
     */
    PBIO_PORT_MODE_I2C = 1 << 3,
    /**
     * The port is in GPIO mode and ADC. No port process is running. User code
     * may access GPIOs on P5 and P6 and read the ADC on P1 and P6.
     */
    PBIO_PORT_MODE_GPIO_ADC = 1 << 4,
} pbio_port_mode_t;

#if PBIO_CONFIG_PORT

void pbio_port_init(void);

void pbio_port_power_off(void);

void pbio_port_stop_user_actions(bool reset);

pbio_error_t pbio_port_get_port(pbio_port_id_t id, pbio_port_t **port);

pbio_port_t *pbio_port_by_index(uint8_t index);

pbio_error_t pbio_port_get_dcmotor(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_dcmotor_t **dcmotor);

pbio_error_t pbio_port_get_servo(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_servo_t **servo);

pbio_error_t pbio_port_get_lump_device(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_port_lump_dev_t **lump_dev);

pbio_error_t pbio_port_get_angle(pbio_port_t *port, pbio_angle_t *angle);

pbio_error_t pbio_port_get_abs_angle(pbio_port_t *port, pbio_angle_t *angle);

pbio_error_t pbio_port_get_analog_value(pbio_port_t *port, lego_device_type_id_t type_id, bool active, uint32_t *value);

pbio_error_t pbio_port_get_analog_rgba(pbio_port_t *port, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t **rgba);

pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement);

pbio_error_t pbio_port_set_mode(pbio_port_t *port, pbio_port_mode_t mode);

pbio_error_t pbio_port_set_type(pbio_port_t *port, lego_device_type_id_t type_id);

pbio_error_t pbio_port_get_uart_dev(pbio_port_t *port, pbdrv_uart_dev_t **uart_dev);

pbio_error_t pbio_port_get_i2c_dev(pbio_port_t *port, pbdrv_i2c_dev_t **i2c_dev);

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

static inline pbio_port_t *pbio_port_by_index(uint8_t index) {
    return NULL;
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

static inline pbio_error_t pbio_port_get_analog_value(pbio_port_t *port, lego_device_type_id_t type_id, bool active, uint32_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_get_analog_rgba(pbio_port_t *port, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t **rgba) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_set_mode(pbio_port_t *port, pbio_port_mode_t mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_set_type(pbio_port_t *port, lego_device_type_id_t type_id) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_get_uart_dev(pbio_port_t *port, pbdrv_uart_dev_t **uart_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_get_i2c_dev(pbio_port_t *port, pbdrv_i2c_dev_t **i2c_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}


#endif // PBIO_CONFIG_PORT

#endif // _PBIO_PORT_INTERFACE_H_

/** @} */
