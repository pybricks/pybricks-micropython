// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_PUP

#include <contiki.h>

#include "ioport_pup.h"
#include "../core.h"

static void init_ports(void) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_IOPORT_NUM_DEV; i++) {
        const pbdrv_ioport_pup_pins_t *pins = &pbdrv_ioport_pup_platform_data.ports[i].pins;

        pbdrv_gpio_input(&pins->gpio1);
        pbdrv_gpio_input(&pins->gpio2);
        pbdrv_gpio_input(&pins->uart_buf);
        pbdrv_gpio_input(&pins->uart_tx);
        pbdrv_gpio_input(&pins->uart_rx);

        // These should be set by default already, but it seems that the
        // bootloader on the Technic hub changes these and causes wrong
        // detection if we don't make sure pull is disabled.
        pbdrv_gpio_set_pull(&pins->gpio1, PBDRV_GPIO_PULL_NONE);
        pbdrv_gpio_set_pull(&pins->gpio2, PBDRV_GPIO_PULL_NONE);
        pbdrv_gpio_set_pull(&pins->uart_buf, PBDRV_GPIO_PULL_NONE);
        pbdrv_gpio_set_pull(&pins->uart_tx, PBDRV_GPIO_PULL_NONE);
        pbdrv_gpio_set_pull(&pins->uart_rx, PBDRV_GPIO_PULL_NONE);
    }
}

void pbdrv_ioport_enable_vcc(bool enable) {
    if (enable) {
        pbdrv_gpio_out_high(&pbdrv_ioport_pup_platform_data.port_vcc);
    } else {
        pbdrv_gpio_out_low(&pbdrv_ioport_pup_platform_data.port_vcc);
    }
}

#if PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE
// This is generic ioport process. It is currently only used for the power
// cycle quirk, so it is protected by guards to save space elsewhere.
PROCESS(pbdrv_ioport_pup_process, "ioport_pup");
#endif

void pbdrv_ioport_init(void) {
    init_ports();

    #if PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE
    pbdrv_init_busy_up();
    process_start(&pbdrv_ioport_pup_process);
    #endif
}

void pbdrv_ioport_power_off(void) {
    init_ports();

    // Turn off power on pin 4 on all ports. This is set to input instead of
    // low to avoid city/move hubs turning back on when button released.
    // as soon as the user releases the power button
    pbdrv_gpio_input(&pbdrv_ioport_pup_platform_data.port_vcc);
}

#if PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE
PROCESS_THREAD(pbdrv_ioport_pup_process, ev, data) {

    static struct etimer timer;

    PROCESS_BEGIN();

    // Some hubs turn on power to the I/O ports in the bootloader. This causes
    // UART sync delays after boot. This process turns them off to make sure
    // power can be enabled at the right time by the legodev driver instead.
    pbdrv_ioport_enable_vcc(false);

    etimer_set(&timer, 500);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

    pbdrv_init_busy_down();

    PROCESS_END();
}

#endif // PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE

#endif // PBDRV_CONFIG_IOPORT_PUP
