// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Manages power off and reset for STM32 MCUs.

// Individual platforms must define pbdrv_reset_stm32_power_off() to physically
// turn off power.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_STM32

#include <contiki.h>
#include STM32_H

#include <pbdrv/led.h>
#include <pbdrv/reset.h>

#include <pbio/color.h>

#if PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER

// Bootloader reads the value at this RAM address to know if BLE firmware loader
// should run or not.
// NB: this can't be static, otherwise section attribute is ignored.
uint32_t pbdrv_reset_stm32_bootloader_selector __attribute__((section(".magic")));

// This value enables the BLE firmware loader.
#define BOOTLOADER_FIRMWARE_UPDATE_MODE 0xAAAAAAAA

#endif // PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER

void pbdrv_reset(pbdrv_reset_action_t action) {
    switch (action) {
        #if PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER
        case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
            pbdrv_reset_stm32_bootloader_selector = BOOTLOADER_FIRMWARE_UPDATE_MODE;
            // fallthrough to PBDRV_RESET_ACTION_RESET
        #endif // PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER

        // TODO: find out if SPIKE Prime can reboot into DFU mode

        case PBDRV_RESET_ACTION_RESET:
            NVIC_SystemReset(); // does not return

        default:
            break;
    }

    pbdrv_led_dev_t *led;
    pbdrv_led_get_dev(0, &led);

    // blink pattern like LEGO firmware
    // FIXME: this is not working on any hub
    for (int i = 0; i < 3; i++) {
        pbdrv_led_on(led, PBIO_COLOR_WHITE);
        clock_delay_usec(50000);
        pbdrv_led_off(led);
        clock_delay_usec(30000);
    }

    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    for (;;) {
        // defined in platform/*/platform.c
        extern void pbdrv_reset_stm32_power_off();
        pbdrv_reset_stm32_power_off();
    }
}

#endif // PBDRV_CONFIG_RESET_STM32
