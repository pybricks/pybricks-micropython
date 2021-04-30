// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Manages power off and reset for STM32 MCUs.

// Individual platforms must define pbdrv_reset_stm32_platform_power_off() to
// physically turn off power.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_STM32

#include <contiki.h>
#include STM32_H

#include <pbdrv/reset.h>
#include <pbdrv/watchdog.h>

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
        // Some platforms can't reboot in update mode. In those cases it will
        // just shutdown instead so that update mode can be manually activated.
        #if PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER
        case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
            pbdrv_reset_stm32_bootloader_selector = BOOTLOADER_FIRMWARE_UPDATE_MODE;
            // fallthrough to PBDRV_RESET_ACTION_RESET
        #endif // PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER

        case PBDRV_RESET_ACTION_RESET:
            NVIC_SystemReset(); // does not return

        default:
            break;
    }

    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    for (;;) {
        // defined in platform/*/platform.c
        extern void pbdrv_reset_stm32_platform_power_off(void);
        pbdrv_reset_stm32_platform_power_off();
        pbdrv_watchdog_update();
    }
}

pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {
    uint32_t status = RCC->CSR;

    if (status & RCC_CSR_SFTRSTF) {
        return PBDRV_RESET_REASON_SOFTWARE;
    }

    if (status & RCC_CSR_IWDGRSTF) {
        return PBDRV_RESET_REASON_WATCHDOG;
    }

    return PBDRV_RESET_REASON_NONE;
}

#endif // PBDRV_CONFIG_RESET_STM32
