// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <stdbool.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>
#include <pbdrv/uart.h>

#if PBDRV_CONFIG_IOPORT

/**
 * Resets pins to the default state, which is input with the buffer disabled.
 */
static pbio_error_t pbdrv_ioport_p5p6_pin_reset(const pbdrv_ioport_pins_t *pins) {

    if (!pins) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_gpio_input(&pins->p5);
    pbdrv_gpio_input(&pins->p6);
    pbdrv_gpio_input(&pins->uart_tx);
    pbdrv_gpio_input(&pins->uart_rx);
    pbdrv_gpio_out_high(&pins->uart_buf);

    // These should be set by default already, but it seems that the
    // bootloader on the Technic hub changes these and causes wrong
    // detection if we don't make sure pull is disabled.
    pbdrv_gpio_set_pull(&pins->p5, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->p6, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_buf, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_tx, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_rx, PBDRV_GPIO_PULL_NONE);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_ioport_p5p6_set_mode(const pbdrv_ioport_pins_t *pins, pbdrv_ioport_p5p6_mode_t mode) {

    pbio_error_t err;

    if (mode == PBDRV_IOPORT_P5P6_MODE_GPIO_ADC) {
        // This is the same as the default mode.
        return pbdrv_ioport_p5p6_pin_reset(pins);
    } else if (mode == PBDRV_IOPORT_P5P6_MODE_UART) {
        err = pbdrv_ioport_p5p6_pin_reset(pins);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Sets up alt mode and enables the buffer for UART use.
        pbdrv_gpio_alt(&pins->uart_rx, pins->uart_rx_alt_uart);
        pbdrv_gpio_alt(&pins->uart_tx, pins->uart_tx_alt_uart);
        pbdrv_gpio_out_low(&pins->uart_buf);
        return PBIO_SUCCESS;
    } else if (mode == PBDRV_IOPORT_P5P6_MODE_I2C) {
        err = pbdrv_ioport_p5p6_pin_reset(pins);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Required for EV3 I2C implementation.
        pbdrv_gpio_out_low(&pins->p5);
        pbdrv_gpio_input(&pins->p5);
        pbdrv_gpio_out_low(&pins->p6);
        pbdrv_gpio_input(&pins->p6);
        return PBIO_SUCCESS;
    } else if (mode == PBDRV_IOPORT_P5P6_MODE_QUADRATURE) {
        // Ports with this mode support only this mode and nothing else. The
        // counter drivers are automatically started on boot. This mode is only
        // set at port init when default ports are set, for which the return
        // value is not checked. This mode should never change at runtime
        // either, so always just return an error.
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_IOPORT

#if PBDRV_CONFIG_HAS_PORT_VCC_CONTROL
void pbdrv_ioport_enable_vcc(bool enable) {
    if (enable) {
        pbdrv_gpio_out_high(&pbdrv_ioport_platform_data_vcc_pin);
    } else {
        pbdrv_gpio_out_low(&pbdrv_ioport_platform_data_vcc_pin);
    }
}
#endif // PBDRV_CONFIG_HAS_PORT_VCC_CONTROL
