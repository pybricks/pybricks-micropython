// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_PUP

#include <pbdrv/ioport.h>

#include "ioport_pup.h"

static void init_one_port(const pbdrv_ioport_pup_pins_t *pins) {
    // Normally should be set already, but could have been changed by bootloader.
    pbdrv_gpio_set_pull(&pins->gpio1, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->gpio2, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_buf, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_tx, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pins->uart_rx, PBDRV_GPIO_PULL_NONE);
}

void pbdrv_ioport_enable_vcc(bool enable) {
    if (enable) {
        pbdrv_gpio_out_high(&pbdrv_ioport_pup_platform_data.port_vcc);
    } else {
        pbdrv_gpio_out_low(&pbdrv_ioport_pup_platform_data.port_vcc);
    }
}

void pbdrv_ioport_init(void) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_IOPORT_NUM_DEV; i++) {
        init_one_port(&pbdrv_ioport_pup_platform_data.ports[i].pins);
    }
    pbdrv_ioport_enable_vcc(true);
}

void pbdrv_ioport_deinit(void) {
    pbdrv_ioport_init();

    // Turn off power on pin 4 on all ports. This is set to input instead of
    // low to avoid city/move hubs turning back on when button released.
    // as soon as the user releases the power button
    pbdrv_gpio_input(&pbdrv_ioport_pup_platform_data.port_vcc);
}

#endif // PBDRV_CONFIG_IOPORT_PUP
