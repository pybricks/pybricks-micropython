// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <pbdrv/counter.h>
#include <pbdrv/i2c.h>
#include <pbdrv/ioport.h>
#include <pbdrv/uart.h>
#include <pbdrv/motor_driver.h>

#include <pbio/angle.h>
#include <pbio/battery.h>
#include <pbio/control.h>
#include <pbio/config.h>
#include <pbio/drivebase.h>
#include <pbio/servo.h>
#include <pbio/util.h>

#include <pbio/os.h>
#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>
#include <pbio/port_lump.h>


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

#if PBIO_CONFIG_PORT

/**
 * Port instance.
 */
struct _pbio_port_t {
    /**
     * The IO port platform data.
     */
    const pbdrv_ioport_platform_data_t *pdata;
    /**
     * High level dc motor motor driver.
     */
    pbio_dcmotor_t *dcmotor;
    /**
     * High level servo motor driver, including access tacho.
     */
    pbio_servo_t *servo;
    /**
     * Position counter device driver.
     */
    pbdrv_counter_dev_t *counter;
    /**
     * UART device driver.
     */
    pbdrv_uart_dev_t *uart_dev;
    /**
     * I2C device driver.
     */
    pbdrv_i2c_dev_t *i2c_dev;
    /**
     * The currently active port mode.
     */
    pbio_port_mode_t mode;
    /**
     * Process for this port.
     */
    pbio_os_process_t process;
    /**
     * Timer for this port process.
     */
    pbio_os_timer_t timer;
    /**
     * Parallel child protothreads used for spawning async functions such as
     * the device connection manager and the uart device process. */
    pbio_os_state_t child1;
    pbio_os_state_t child2;
    /**
     * Device connection manager that detects passive devices in LEGO mode.
     */
    pbio_port_dcm_t *connection_manager;
    /**
     * LEGO UART Messaging Protocol device instance.
     */
    pbio_port_lump_dev_t *lump_dev;
};

static pbio_port_t ports[PBIO_CONFIG_PORT_NUM_DEV];

pbio_error_t pbio_port_process_pup_thread(pbio_os_state_t *state, void *context) {

    pbio_port_t *port = context;

    // NB: This same process thread definition is shared for all ports, so we
    // cannot use static variables across yields as is normally done in these
    // processes. Use the port state variables instead.

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // NB: Currently only implements LEGO mode and assumes that all PUP
    // peripherals are available and initialized.

    for (;;) {

        // Run passive device connection manager until UART device is detected.
        pbdrv_ioport_p5p6_set_mode(port->pdata->pins, port->uart_dev, PBDRV_IOPORT_P5P6_MODE_GPIO_ADC);
        PBIO_OS_AWAIT(state, &port->child1, err = pbio_port_dcm_thread(&port->child1, &port->timer, port->connection_manager, port->pdata->pins));

        // Synchronize with LUMP data stream from sensor and parse device info.
        pbdrv_ioport_p5p6_set_mode(port->pdata->pins, port->uart_dev, PBDRV_IOPORT_P5P6_MODE_UART);
        PBIO_OS_AWAIT(state, &port->child1, err = pbio_port_lump_sync_thread(&port->child1, port->lump_dev, port->uart_dev, &port->timer));
        if (err != PBIO_SUCCESS) {
            // Synchronization failed. Retry.
            continue;
        }

        // Exchange sensor data with the LUMP device until it is disconnected.
        // The send thread detects this when the keep alive messages time out.
        pbio_port_p1p2_set_power(port, pbio_port_lump_get_power_requirements(port->lump_dev));
        PBIO_OS_AWAIT_RACE(state, &port->child1, &port->child2,
            pbio_port_lump_data_recv_thread(&port->child1, port->lump_dev, port->uart_dev),
            pbio_port_lump_data_send_thread(&port->child2, port->lump_dev, port->uart_dev, &port->timer)
            );

        pbio_port_p1p2_set_power(port, PBIO_PORT_POWER_REQUIREMENTS_NONE);
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

/**
 * Gets the reported angle set by an attached device, if any.
 *
 * @param [in]  ioport    The port instance.
 * @param [out] angle     The angle.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device that reports
 *                        angles is attached.
 */
pbio_error_t pbio_port_get_angle(pbio_port_t *port, pbio_angle_t *angle) {
    if (port->lump_dev) {
        return pbio_port_lump_get_angle(port->lump_dev, angle, false);
    }

    if (port->counter) {
        return pbdrv_counter_get_angle(port->counter, &angle->rotations, &angle->millidegrees);
    }
    return PBIO_ERROR_NO_DEV;
}

/**
 * Gets the reported absolute angle set by an attached device, if any.
 *
 * @param [in]  ioport    The port instance.
 * @param [out] angle     The absolute angle.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device that reports
 *                          angles is attached.
 *                        ::PBIO_ERROR_NOT_SUPPORTED if the device does not
 *                          support absolute angles.
 */
pbio_error_t pbio_port_get_abs_angle(pbio_port_t *port, pbio_angle_t *angle) {
    if (port->lump_dev) {
        return pbio_port_lump_get_angle(port->lump_dev, angle, false);
    }

    if (port->counter) {
        return pbdrv_counter_get_abs_angle(port->counter, &angle->millidegrees);
    }
    return PBIO_ERROR_NO_DEV;
}

/**
 * Gets the UART interface of the port.
 *
 * @param [in]  port        The port instance.
 * @param [out] uart_dev    The UART device.
 * @return                  ::PBIO_SUCCESS on success, otherwise
 *                          ::PBIO_ERROR_NOT_SUPPORTED if this port does not support UART.
 */
pbio_error_t pbio_port_get_uart_dev(pbio_port_t *port, pbdrv_uart_dev_t **uart_dev) {
    // User access to UART device is only allowed in direct UART mode.
    if (port->mode != PBIO_PORT_MODE_UART) {
        return PBIO_ERROR_INVALID_OP;
    }
    *uart_dev = port->uart_dev;
    return PBIO_SUCCESS;
}

/**
 * Gets the I2C interface of the port.
 *
 * @param [in]  port        The port instance.
 * @param [out] i2c_dev     The I2C device.
 * @return                  ::PBIO_SUCCESS on success, otherwise
 *                          ::PBIO_ERROR_NOT_SUPPORTED if this port does not support I2C.
 */
pbio_error_t pbio_port_get_i2c_dev(pbio_port_t *port, pbdrv_i2c_dev_t **i2c_dev) {
    // User access to I2C device is only allowed in direct I2C mode.
    if (port->mode != PBIO_PORT_MODE_I2C) {
        return PBIO_ERROR_INVALID_OP;
    }
    *i2c_dev = port->i2c_dev;
    return PBIO_SUCCESS;
}

/**
 * Gets the LUMP device connected to this port, if any.
 *
 * @param [in]    port        The port instance.
 * @param [inout] type_id     The expected type identifier of the LUMP device.
 * @param [out]   lump_dev    The LUMP device.
 * @return                    ::PBIO_SUCCESS on success, otherwise
 *                            ::PBIO_ERROR_NO_DEV if no LUMP device is connected or not of the expected type.
 *                            ::PBIO_ERROR_INVALID_OP if the port is not in LEGO mode.
 *                            ::PBIO_ERROR_NOT_SUPPORTED if this port does not support UART.
 */
pbio_error_t pbio_port_get_lump_device(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_port_lump_dev_t **lump_dev) {

    if (port->mode != PBIO_PORT_MODE_LEGO_DCM) {
        return PBIO_ERROR_INVALID_OP;
    }

    pbio_error_t err = pbio_port_lump_assert_type_id(port->lump_dev, expected_type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *lump_dev = port->lump_dev;
    return PBIO_SUCCESS;
}

/**
 * Gets the DC motor or light device connected to this port, if any.
 *
 * @param [in]  port                The port instance.
 * @param [inout] expected_type_id  The expected type identifier or range. Returns the actual type.
 * @param [out] dcmotor             The dcmotor device.
 * @return                          ::PBIO_SUCCESS on success, otherwise
 *                                  ::PBIO_ERROR_NO_DEV if no DC motor or light device of expected type is connected.
 */
pbio_error_t pbio_port_get_dcmotor(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_dcmotor_t **dcmotor) {

    // Not all ports have a motor.
    if (!port->dcmotor) {
        return PBIO_ERROR_NO_DEV;
    }

    // Return requested device.
    *dcmotor = port->dcmotor;

    // In LEGO mode, we require that something valid is indeed attached.
    if (port->mode == PBIO_PORT_MODE_LEGO_DCM && port->connection_manager) {
        return pbio_port_dcm_assert_type_id(port->connection_manager, expected_type_id);
    }
    return PBIO_SUCCESS;
}

/**
 * Gets the servo connected to this port, if any.
 *
 * @param [in]    port              The port instance.
 * @param [inout] expected_type_id  The expected type identifier or range. Returns the actual type.
 * @param [out]   dcmotor           The servo device.
 * @return                          ::PBIO_SUCCESS on success, otherwise
 *                                  ::PBIO_ERROR_NO_DEV if no DC motor or light device of expected type is connected.
 */
pbio_error_t pbio_port_get_servo(pbio_port_t *port, lego_device_type_id_t *expected_type_id, pbio_servo_t **servo) {

    // Not all ports have a motor.
    if (!port->servo) {
        return PBIO_ERROR_NO_DEV;
    }

    // Return requested device.
    *servo = port->servo;

    if (port->mode == PBIO_PORT_MODE_LEGO_DCM && port->lump_dev) {
        return pbio_port_lump_assert_type_id(port->lump_dev, expected_type_id);
    }

    if (port->counter) {
        return pbdrv_counter_assert_type(port->counter, expected_type_id);
    }

    return PBIO_ERROR_NO_DEV;
}

pbio_error_t pbio_port_p1p2_set_power(pbio_port_t *port, pbio_port_power_requirements_t power_requirement) {
    pbdrv_motor_driver_dev_t *motor_driver;
    pbio_error_t err = pbdrv_motor_driver_get_dev(port->pdata->motor_driver_index, &motor_driver);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    if (power_requirement == PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS) {
        return pbdrv_motor_driver_set_duty_cycle(motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY);
    }
    if (power_requirement == PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P2_POS) {
        return pbdrv_motor_driver_set_duty_cycle(motor_driver, PBDRV_MOTOR_DRIVER_MAX_DUTY);
    }
    return pbdrv_motor_driver_coast(motor_driver);
}

/**
 * Gets the analog value of the LEGO device.
 *
 * @param [in]  port        The port instance.
 * @param [in]  type_id     The expected type identifier.
 * @param [in]  active      Whether to get activate active mode.
 * @param [out] value       The analog value.
 * @return                  ::PBIO_SUCCESS on success, ::PBIO_ERROR_NO_DEV expected device is not connected.
 */
pbio_error_t pbio_port_get_analog_value(pbio_port_t *port, lego_device_type_id_t type_id, bool active, uint32_t *value) {
    if (!port->connection_manager || port->mode != PBIO_PORT_MODE_LEGO_DCM) {
        return PBIO_ERROR_INVALID_OP;
    }
    lego_device_type_id_t expected_type_id = type_id;
    pbio_error_t err = pbio_port_dcm_assert_type_id(port->connection_manager, &expected_type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *value = pbio_port_dcm_get_analog_value(port->connection_manager, port->pdata->pins, active);
    return PBIO_SUCCESS;
}

/**
 * Gets the analog color values of the LEGO device.
 *
 * @param [in]  port        The port instance.
 * @param [in]  type_id     The expected type identifier.
 * @param [out] rgba        The analog color values.
 * @return                  ::PBIO_SUCCESS on success, ::PBIO_ERROR_NO_DEV expected device is not connected.
 */
pbio_error_t pbio_port_get_analog_rgba(pbio_port_t *port, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t **rgba) {

    *rgba = pbio_port_dcm_get_analog_rgba(port->connection_manager);
    if (!port->connection_manager || port->mode != PBIO_PORT_MODE_LEGO_DCM || !*rgba) {
        return PBIO_ERROR_INVALID_OP;
    }

    lego_device_type_id_t expected_type_id = type_id;
    return pbio_port_dcm_assert_type_id(port->connection_manager, &expected_type_id);
}

/**
 * Initializes a port instance.
 *
 * NB: Errors from this init functions are not checked. This should only use
 * static config information and not rely on runtime capability checks.
 *
 * @param [in]  port  The port instance.
 */
static void pbio_port_init_one_port(pbio_port_t *port) {

    // Initialize all ports with the none process.
    pbio_os_process_start(&port->process, pbio_port_process_none_thread, port);

    // Configure motor instances if this port has them. This assumes that
    // all motor ports have a way to get angle information. We can add
    // a dedicated DC mode if we support hubs where this is not the case.
    pbdrv_motor_driver_dev_t *motor_driver;
    pbio_error_t err = pbdrv_motor_driver_get_dev(port->pdata->motor_driver_index, &motor_driver);
    if (err == PBIO_SUCCESS) {
        // Get the motor driver for this port in order to initialize the
        // high level DC motor instance and servo. Since the index getter
        // passed and other lookups below use constant platform data, skip
        // checking for errors.
        port->dcmotor = pbio_dcmotor_init_instance(port->pdata->motor_driver_index, motor_driver);
        port->servo = pbio_servo_init_instance(port->pdata->motor_driver_index, port, port->dcmotor);
    }

    // Optionally used by some ports to get angle information.
    pbdrv_counter_get_dev(port->pdata->counter_driver_index, &port->counter);

    // Configure basic quadrature-only ports such as BOOST A&B or NXT A&B&C
    // without device kind and type id detection.
    if (port->pdata->supported_modes == PBIO_PORT_MODE_QUADRATURE) {
        pbio_port_set_mode(port, PBIO_PORT_MODE_QUADRATURE);
        return;
    }

    // If uart and gpio available, initialize device manager and uart devices.
    pbdrv_uart_get_instance(port->pdata->uart_driver_index, &port->uart_dev);
    pbdrv_i2c_get_instance(port->pdata->i2c_driver_index, &port->i2c_dev);

    if (port->uart_dev && port->pdata->supported_modes & PBIO_PORT_MODE_LEGO_DCM) {
        // Initialize passive device connection manager and LEGO UART device.
        port->connection_manager = pbio_port_dcm_init_instance(port->pdata->external_port_index);
        port->lump_dev = pbio_port_lump_init_instance(port->pdata->external_port_index);
        pbio_port_set_mode(port, PBIO_PORT_MODE_LEGO_DCM);
        return;
    }

    // If uart is available but LEGO mode was not selected, initialize UART
    // mode. In practice, this initialization step is only used in debug mode
    // where the LEGO mode is disabled on one port.
    if (port->uart_dev && (port->pdata->supported_modes & PBIO_PORT_MODE_UART)) {
        pbio_port_set_mode(port, PBIO_PORT_MODE_UART);
        return;
    }
}

void pbio_port_init(void) {

    for (uint8_t i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {
        pbio_port_t *port = &ports[i];
        port->pdata = &pbdrv_ioport_platform_data[i];
        pbio_port_init_one_port(port);
    }
}

pbio_error_t pbio_port_get_port(pbio_port_id_t id, pbio_port_t **port) {
    for (uint8_t i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {
        pbio_port_t *p = &ports[i];
        if (p->pdata->port_id == id) {
            *port = p;
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_NO_DEV;
}

/**
 * Stops all user-level background processes related to ports
 *
 * @param [in]  reset  Whether to reset all user-level processes to a clean
 *                     state (true), or whether to only stop active outputs
 *                     like motors (false). The latter is useful
 *                     to preserve the state for debugging, without
 *                     movement getting in the way, or out of control.
 */
void pbio_port_stop_user_actions(bool reset) {
    for (uint8_t i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {
        pbio_port_t *port = &ports[i];

        // Don't reset devices that always need power like powered sensors.
        if (port->lump_dev && pbio_port_lump_get_power_requirements(port->lump_dev) != PBIO_PORT_POWER_REQUIREMENTS_NONE) {
            continue;
        }

        // Stops and resets motors. Also stops higher level controls like servo.
        pbio_dcmotor_reset(port->dcmotor, reset);
    }
}

/**
 * Prepares the system for power off by turning off all sensors and motors.
 *
 * @return  Whether the system is ready to power off.
 */

void pbio_port_power_off(void) {
    // Stops motors if active, ignoring sensors that need permanent power.
    pbio_port_stop_user_actions(true);

    // We also want to turn off sensors powered through the motor terminals
    // and stop their processes from triggering any further actions.
    for (uint8_t i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {
        pbio_port_t *port = &ports[i];
        pbio_port_set_mode(port, PBIO_PORT_MODE_NONE);
    }

    // This turns off any sensor lights that run on VCC.
    pbdrv_ioport_enable_vcc(false);
}

/**
 * Sets the mode of the port.
 *
 * Should not be called before port process is started.
 *
 * The return value is not checked for the initial mode, which is assumed to
 * be allowed and properly configured in constant platform data.
 *
 * The return value can be used instead when switching mode at runtime.
 *
 * @param [in]  port    The port instance.
 * @param [in]  mode    The mode to set.
 * @return              ::PBIO_SUCCESS on success.
 *                      ::PBIO_ERROR_INVALID_OP if the operation is not permitted in the current state.
 */
pbio_error_t pbio_port_set_mode(pbio_port_t *port, pbio_port_mode_t mode) {

    // Nothing to do.
    if (port->mode == mode) {
        return PBIO_SUCCESS;
    }

    // Check if the mode is supported by the port.
    if (!(port->pdata->supported_modes & mode)) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Disable thread activity by attaching a thread that does nothing.
    pbio_os_process_init(&port->process, pbio_port_process_none_thread);
    port->mode = mode;

    switch (mode) {
        case PBIO_PORT_MODE_NONE:
            pbdrv_ioport_p5p6_set_mode(port->pdata->pins, port->uart_dev, PBDRV_IOPORT_P5P6_MODE_GPIO_ADC);
            pbio_port_p1p2_set_power(port, PBIO_PORT_POWER_REQUIREMENTS_NONE);
            return PBIO_SUCCESS;
        case PBIO_PORT_MODE_LEGO_DCM:
            // Physical modes for this mode will be set by the process so this
            // is all we need to do here.
            pbio_os_process_init(&port->process, pbio_port_process_pup_thread);
            // Returning e-again allows user module to wait for the port to be
            // ready after first entering LEGO mode, avoiding NODEV errors when
            // switching from direct access modes back to LEGO mode.
            return PBIO_ERROR_AGAIN;
        case PBIO_PORT_MODE_UART:
            // Enable UART on the port. No process needed here. User can
            // access UART from their own event loop.
            pbdrv_ioport_p5p6_set_mode(port->pdata->pins, port->uart_dev, PBDRV_IOPORT_P5P6_MODE_UART);
            return PBIO_SUCCESS;
        case PBIO_PORT_MODE_QUADRATURE:
            return PBIO_SUCCESS;
        default:
            return PBIO_ERROR_NOT_SUPPORTED;
    }
}

#endif // PBIO_CONFIG_PORT
