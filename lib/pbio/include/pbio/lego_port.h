// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup LegoPort pbio/lego_port: I/O port control for LEGO 6-wire ports.
 * @{
 */

#ifndef _PBIO_LEGO_PORT_H_
#define _PBIO_LEGO_PORT_H_

#include <stdint.h>
#include <pbio/iodev.h> // drop this, now used to get device_id_t, which could be moved here.
#include <pbdrv/motor_driver.h>

/**
 * Modes that control the behavior of pins P1 and P2.
 *
 * These modes are not accessible by users, but are set by the system to create
 * the desired behavior specified by the user-given pbio_lego_port_mode_t.
 *
 * Switching between modes is an instantaneous operation. Waiting on processes
 * to close or finish should be done by the higher level pbio_lego_port_mode_t.
 */
typedef enum {
    /**
     * The port is off, both pins are floating.
     */
    LEGO_PORT_P1P2_MODE_OFF,
    /**
     * Allows P1 to be set high (connected to battery voltage). P2 is always
     * low (connected to ground). Can be used to power devices that need a
     * constant supply of +VBAT.
     */
    LEGO_PORT_P1P2_MODE_BATTERY_POWER,
    /**
     * Pins act as PWM output, where V1-V2 equals +VBAT for 100% PWM, and
     * equals -VBAT for -100% PWM. Can be used to power devices that need a
     * constant supply of +VBAT or -VBAT, or a variable voltage for dc motors
     * and lights. Use ::LEGO_PORT_P1P2_MODE_OFF to make both pins float.
     */
    LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM,
    /**
     * The outputs on P1 and P2 are floating, and the voltage across
     * the passive impedance across P1 and P2 is measured with an ADC.
     */
    LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE,
    /**
     * The outputs on P1 and P2 are floating for 0.1ms, and the voltage across
     * the passive impedance across P1 and P2 is measured with an ADC. Then P1
     * is high and P2 is low for 3ms to provide power to the device.
     * Physically, this a combination of ::LEGO_PORT_P1P2_MODE_BATTERY_POWER
     * and ::LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE, but it has its own mode
     * because the timing is configured through hardware peripherals.
     */
    LEGO_PORT_P1P2_MODE_ANALOG_POWERED,
} pbio_lego_port_p1p2_mode_t;


/**
 * Modes that control the behavior of pins P5 and P6.
 *
 * These modes are not accessible by users, but are set by the system to create
 * the desired behavior specified by the user-given pbio_lego_port_mode_t.
 *
 * Switching between modes is an instantaneous operation. Waiting on processes
 * to close or finish should be done by the higher level pbio_lego_port_mode_t.
 */
typedef enum {
    /**
     * P5 and P6 are floating.
     */
    LEGO_PORT_P5P6_MODE_OFF,
    /**
     * P5 is I2C SCL, P6 is I2C SDA.
     */
    LEGO_PORT_P5P6_MODE_I2C,
    /**
     * P5 is UART TX (hub side), P6 is UART RX (hub side).
     */
    LEGO_PORT_P5P6_MODE_UART,
    /**
     * P5 and P6 are general purpose I/O.
     */
    LEGO_PORT_P5P6_MODE_GPIO,
    /**
     * Measures the voltage across the impedance from P6 to GND.
     */
    LEGO_PORT_P5P6_MODE_ANALOG,
    /**
     * P5 and P6 are connected to a quadrature encoder to measure rotation.
     *
     * This mode may also be used to obtain a motor ID on some platforms.
     */
    LEGO_PORT_P5P6_MODE_QUADRATURE,
} pbio_lego_port_p5p6_mode_t;

/**
 * Modes that govern the combined behavior of pins P1, P2, P5, and P6. This
 * overall port mode *never* changes automatically. It is set to a
 * platform-specific default on boot. It can be changed implicitly by the user
 * by initializing a device class that maps to one of these modes.
 */
typedef enum {
    /**
     * This is used for all devices that adhere to the LEGO Powered Up (PUP)
     * standard, also known as LEGO Power Functions 2.0 (LPF2).
     *
     * P5 and P6 start in ::LEGO_PORT_P5P6_MODE_GPIO to scan for 8 possible
     * device types or no device attached. If a UART-type device is detected,
     * P5 and P6 switch to ::LEGO_PORT_P5P6_MODE_UART to get more information
     * about the device capabilities and to poll sensor data.
     *
     * P1 and P2 start in ::LEGO_PORT_P1P2_MODE_OFF and switch to
     * ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM if the detected device requires
     * power. It switches back to ::LEGO_PORT_P1P2_MODE_OFF if the device is
     * disconnected.
     */
    LEGO_PORT_MODE_PUP_DEVICE = (1 << 0),
    /**
     * This is used for motors built into hubs. These are not externally wired
     * with a 6-wire cable, but work according to the same specification.
     *
     * P5 and P6 start in ::LEGO_PORT_P5P6_MODE_QUADRATURE.
     *
     * P1 and P2 are in ::LEGO_PORT_P1P2_MODE_OFF to coast or in
     * ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM to provide power to the motor.
     */
    LEGO_PORT_MODE_PUP_MOTOR_INTERNAL = (1 << 1),
    /**
     * This is used for all official LEGO EV3 sensors and input devices that
     * adhere to the same specification, including auto-detectable NXT sensors.
     *
     * TODO: describe basic auto-id process
     *
     * If a UART-type device is detected, P5 and P6 switch to
     * ::LEGO_PORT_P5P6_MODE_UART to get more information about the device
     * capabilities and to poll sensor data. If the device needs power,
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_BATTERY_POWER.
     *
     * If an EV3-type-analog device is detected, P5 and P6 are
     * in ::LEGO_PORT_P5P6_MODE_ANALOG.
     *
     * If a NXT-type-analog device is detected, P1 and P2 switch
     * to ::LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE.
     * P5 and P6 are in ::LEGO_PORT_P5P6_MODE_GPIO in order to toggle the
     * measurement type by setting P5 high or low.
     *
     * If a NXT-type-i2c device is detected, P5 and P6 switch
     * to ::LEGO_PORT_P5P6_MODE_I2C. If the device needs power,
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_BATTERY_POWER.
     *
     * If a NXT Color sensor is detected, P5 and P6 switch
     * to ::LEGO_PORT_P5P6_MODE_GPIO.
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE.
     */
    LEGO_PORT_MODE_EV3_SENSOR = (1 << 2),
    /**
     * This is used on the EV3 Brick for all official LEGO EV3 motors or
     * devices that adhere to the same specification, including the NXT motor.
     *
     * P1 and P2 are in ::LEGO_PORT_P1P2_MODE_OFF to coast or in
     * ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM to provide power to the motor.
     *
     * P5 and P6 are in :: LEGO_PORT_P5P6_MODE_QUADRATURE.
     */
    LEGO_PORT_MODE_EV3_MOTOR = (1 << 3),
    /**
     * This is used for all motors that cannot be auto-detected on a platform,
     * such as any motor on the NXT brick, or no-ID DC motors on all platforms.
     *
     * P1 and P2 are in ::LEGO_PORT_P1P2_MODE_OFF to coast or in
     * ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM to provide power to the motor.
     *
     * P5 and P6 are in :: LEGO_PORT_P5P6_MODE_QUADRATURE if supported on that
     * platform, else ::LEGO_PORT_P5P6_MODE_OFF.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_MOTOR = (1 << 4),
    /**
     * This is used for analog devices that cannot be auto-detected on a platform.
     * such as any motor on the NXT brick, or non-ID DC motors on all platforms.
     *
     * P1 and P2 are in ::LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE.
     * P5 and P6 are in ::LEGO_PORT_P5P6_MODE_GPIO, starting both as inputs but
     * may be switched to outputs to provide additional functionality.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_ANALOG = (1 << 5),
    /**
     * This is used only on the NXT Brick with the NXT color sensor class.
     *
     * P5 and P6 switch to ::LEGO_PORT_P5P6_MODE_GPIO.
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_ANALOG_PASSIVE.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_NXT_COLOR = (1 << 6),
    /**
     * This is used for powered RCX sensors like the light and rotation sensor.
     *
     * P1 and P2 are in ::LEGO_PORT_P1P2_MODE_ANALOG_POWERED.
     * P5 and P6 are in ::LEGO_PORT_P5P6_MODE_OFF.
     */
    LEGO_PORT_MODE_RCX_SENSOR_ACTIVE = (1 << 7),
    /**
     * This is used with custom or unknown UART devices.
     *
     * P5 and P6 switch to ::LEGO_PORT_P5P6_MODE_UART.
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM if available
     * on that platform, else LEGO_PORT_P1P2_MODE_BATTERY_POWER.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_UART = (1 << 8),
    /**
     * This is used with custom or unknown I2C devices.
     *
     * This is also used for all I2C devices on the NXT brick.
     *
     * P5 and P6 switch to ::LEGO_PORT_P5P6_MODE_I2C.
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM if available
     * on that platform, else LEGO_PORT_P1P2_MODE_BATTERY_POWER.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_I2C = (1 << 9),
    /**
     * This is used with custom or unknown GPIO devices.
     *
     * P5 and P6 switch to ::LEGO_PORT_P5P6_MODE_GPIO.
     * P1 and P2 switch to ::LEGO_PORT_P1P2_MODE_BATTERY_POWER_PWM if available
     * on that platform, else LEGO_PORT_P1P2_MODE_BATTERY_POWER.
     */
    LEGO_PORT_MODE_UNIDENTIFIED_GPIO = (1 << 10),
} pbio_lego_port_mode_t;

/**
 * Description of ports and capabilities.
 */
typedef struct {
    /** Port identifier marked on the hub casing. */
    pbio_port_id_t port_id;
    /** Default device ID before anything is detected. */
    pbio_iodev_type_id_t default_id;
    /** Default mode on boot. */
    pbio_lego_port_mode_t default_mode;
    /** Modes that this port can operate in. */
    pbio_lego_port_mode_t available_modes;
} pbio_lego_port_platform_data_t;

/**
 * The port object that provides access to all i/o interfaces.
 */
typedef struct {
    /** Port information and capabilities. */
    const pbio_lego_port_platform_data_t *data;
    /** The PWM output on P1, P2, or NULL if unavailable.*/
    pbdrv_motor_driver_dev_t *motor_driver;
    /** Current port mode. */
    pbio_lego_port_mode_t current_mode;
    /**
     * True if port is ready to switch modes, false if the current mode has
     * ongoing processes to finish.
     */
    bool can_switch_mode;
} pbio_lego_port_t;

/**
 * Get a LEGO port object by its port identifier.
 *
 * NB: This will replace all by-port-id getters we have now; those will operate
 *     with getters on the port object instead.
 *
 * @param [out] port       Pointer to the LEGO port object.
 * @param [in]  port_id    Port identifier.
 * @return                 ::PBIO_SUCCESS if successful,
 *                         ::PBIO_ERROR_NO_DEV if port does not exist.
 */
pbio_error_t pbio_lego_port_get_port(pbio_lego_port_t **port, pbio_port_id_t port_id);

/**
 * Sets the mode of a LEGO port.
 *
 * @param [in] port        Pointer to the LEGO port object.
 * @param [in] mode        The mode to set.
 * @return                 ::PBIO_SUCCESS if successful,
 *                         ::PBIO_ERROR_BUSY if port is not ready to switch modes,
 *                         ::PBIO_ERROR_NOT_SUPPORTED if requested mode is not supported.
 */
pbio_error_t pbio_lego_port_set_mode(pbio_lego_port_t *port, pbio_lego_port_mode_t mode);

/**
 * Starts closing any ongoing processes of the current mode.
 *
 * @param [in] port        Pointer to the LEGO port object.
 *
 * @return ::PBIO_SUCCESS if successful, ::PBIO_ERROR_NO_DEV if port does not exist.
 */
pbio_error_t pbio_lego_port_unprepare_mode(pbio_lego_port_t *port);

#endif // _PBIO_LEGO_PORT_H_

/** @} */
