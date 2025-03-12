// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup ioport Driver: Driver for setting modes for input/output ports.
 *                            Currently there is only the legodev mode.
 * @{
 */

#ifndef PBDRV_IOPORT_H
#define PBDRV_IOPORT_H

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbdrv/uart.h>
#include <pbdrv/motor_driver.h>

#include <stdbool.h>

/**
 * Modes of one I/O port. Governs the behavior of the P5 and P6 pins.
 */
typedef enum {
    /**
     * Both GPIOs and and their buffered counterparts may be controlled freely.
     * The buffer may be used as needed. The ADC may be read if present.
     *
     * This is used during device detection of default LEGO devices and to read
     * values on passive LEGO devices. May also be used for general purpose and
     * custom devices. No IRQs are enabled for the GPIOs.
     */
    PBDRV_IOPORT_P5P6_MODE_GPIO_ADC,
    /**
     * The buffer, pins, and IRQs are configured for quadrature encoder
     * position counting.
     */
    PBDRV_IOPORT_P5P6_MODE_QUADRATURE,
    /**
     * The buffer, pins, and IRQs are configured for UART communication.
     **/
    PBDRV_IOPORT_P5P6_MODE_UART,
    /**
     * The buffer, pins, and IRQs are are configured for I2C communication.
     **/
    PBDRV_IOPORT_P5P6_MODE_I2C,
} pbdrv_ioport_p5p6_mode_t;

/**
 * The GPIO pins of an I/O port.
 */
typedef struct {
    /**
     * GPIO to enable (set low) or mark (set high) access to the UART pins.
     */
    pbdrv_gpio_t uart_buf;
    /**
     * Pin 5 input.
     */
    pbdrv_gpio_t p5;
    /**
     * Pin 5 output or UART TX when buffer enabled.
     */
    pbdrv_gpio_t uart_tx;
    /**
     * Pin 5 alt mode index for UART usage.
     */
    uint8_t uart_tx_alt_uart;
    /**
     * Pin 6 input or output.
     */
    pbdrv_gpio_t p6;
    /**
     * Pin 6 input or UART RX when buffer enabled.
     */
    pbdrv_gpio_t uart_rx;
    /**
     * Pin 6 alt mode index for UART usage.
     */
    uint8_t uart_rx_alt_uart;
} pbdrv_ioport_pins_t;

#define PBDRV_IOPORT_INDEX_NOT_AVAILABLE (0xFF)

typedef struct {
    /** The I/O pins of the port. Can be NULL if this port has no GPIO pins */
    const pbdrv_ioport_pins_t *pins;
    /** The I/O port identifier this port is associated with. */
    pbio_port_id_t port_id;
    /** Index of associated motor driver. */
    uint8_t motor_driver_index;
    /** Index of external port */
    uint8_t external_port_index;
    /** Counter driver index */
    uint8_t counter_driver_index;
    /** Index of associated UART driver. */
    uint8_t uart_driver_index;
    /** Supported high level modes */
    uint32_t supported_modes;
} pbdrv_ioport_platform_data_t;

/**
 * Enables or disables VCC on pin 4 of all ioports.
 *
 * @param [in]  enable        Choose true to enable VCC, false to disable.
 */
void pbdrv_ioport_enable_vcc(bool enable);

/**
 * Sets the mode of the P5P6 pins on this port.
 *
 * This will disable the current mode and re-activate the new mode, even if the
 * same mode was already set. For example, setting to GPIO mode will reset all
 * pins to inputs even if GPIO mode is already active.
 *
 * @param [in]  pins          The pins to set the mode for.
 * @param [in]  uart_dev      The UART device to use for UART mode.
 * @param [in]  mode          The mode state to set.
 * @return                    ::PBIO_SUCCESS on success, otherwise
 *                            ::PBIO_ERROR_NOT_SUPPORTED if the mode is not supported.
 *                            ::PBIO_ERROR_INVALID_OP if changing the mode is not permitted in the current state.
 */
pbio_error_t pbdrv_ioport_p5p6_set_mode(const pbdrv_ioport_pins_t *pins, pbdrv_uart_dev_t *uart_dev, pbdrv_ioport_p5p6_mode_t mode);

extern const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV];

extern const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin;

#endif // PBDRV_IOPORT_H

/** @} */
