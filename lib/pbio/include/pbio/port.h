// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup Port pbio/port: I/O port interface
 * @{
 */

#ifndef _PBIO_PORT_H_
#define _PBIO_PORT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lego/device.h>

#include <pbdrv/config.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include <pbsys/telemetry.h>

typedef struct _pbio_port_t pbio_port_t;

// Handle types owned by other modules, only used as pointers here. Forward
// declared to avoid a circular header dependency.
typedef struct _pbio_dcmotor_t pbio_dcmotor_t;
typedef struct _pbio_servo_t pbio_servo_t;
typedef struct _pbio_port_lump_dev_t pbio_port_lump_dev_t;
typedef struct _pbio_port_dcm_analog_rgba_t pbio_port_dcm_analog_rgba_t;
typedef struct _pbdrv_uart_dev_t pbdrv_uart_dev_t;
typedef struct _pbdrv_i2c_dev_t pbdrv_i2c_dev_t;

/**
 * I/O port identifier. The meaning and availability of a port is device-specific.
 */
typedef enum {
    #if PBDRV_CONFIG_HAS_PORT_A
    PBIO_PORT_ID_A = 'A', /**< I/O port labeled as "A" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_B
    PBIO_PORT_ID_B = 'B', /**< I/O port labeled as "B" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_C
    PBIO_PORT_ID_C = 'C', /**< I/O port labeled as "C" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_D
    PBIO_PORT_ID_D = 'D', /**< I/O port labeled as "D" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_E
    PBIO_PORT_ID_E = 'E', /**< I/O port labeled as "E" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_F
    PBIO_PORT_ID_F = 'F', /**< I/O port labeled as "F" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_1
    PBIO_PORT_ID_1 = '1', /**< I/O port labeled as "1" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_2
    PBIO_PORT_ID_2 = '2', /**< I/O port labeled as "2" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_3
    PBIO_PORT_ID_3 = '3', /**< I/O port labeled as "3" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_4
    PBIO_PORT_ID_4 = '4', /**< I/O port labeled as "4" */
    #endif
} pbio_port_id_t;

/**
 * Power state across the P1P2 pins when full on/off is required. Used for some
 * sensors that cannot run on the voltage provided by P4 alone.
 *
 * On some platforms, one pin may supply a voltage while the other is ground.
 * On some platforms, the two wires are driven by a motor driver. The effect on
 * a powered sensor is the same.
 */
typedef enum {
    /* The power is off. Both pins are either floating or both are ground */
    PBIO_PORT_POWER_REQUIREMENTS_NONE,
    /* The battery voltage is applied across the pins, with p1 the positive side. */
    PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS,
    /* The battery voltage is applied across the pins, with p2 the positive side. */
    PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P2_POS,
} pbio_port_power_requirements_t;


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

pbio_error_t pbio_port_get_analog_rgba(pbio_port_t *port, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t *rgba);

pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement);

pbio_error_t pbio_port_set_mode(pbio_port_t *port, pbio_port_mode_t mode);

pbio_error_t pbio_port_set_type(pbio_port_t *port, lego_device_type_id_t type_id);

pbio_error_t pbio_port_get_uart_dev(pbio_port_t *port, pbdrv_uart_dev_t **uart_dev);

pbio_error_t pbio_port_get_i2c_dev(pbio_port_t *port, pbdrv_i2c_dev_t **i2c_dev);

pbsys_telemetry_error_t pbio_port_get_telemetry(uint8_t index, pbsys_telemetry_packet_t *tel, uint32_t *size);

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

static inline pbio_error_t pbio_port_get_analog_rgba(pbio_port_t *port, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t *rgba) {
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

static inline pbsys_telemetry_error_t pbio_port_get_telemetry(uint8_t index, pbsys_telemetry_packet_t *tel, uint32_t *size) {
    *size = 0;
    return PBSYS_TELEMETRY_ERROR_NO_ROOM;
}


#endif // PBIO_CONFIG_PORT

#endif // _PBIO_PORT_H_

/** @} */
