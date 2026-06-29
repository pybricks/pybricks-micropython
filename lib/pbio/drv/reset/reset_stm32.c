// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Manages power off and reset for STM32 MCUs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_STM32

#include STM32_H

#include "reset_stm32.h"

#include <pbdrv/reset.h>
#include <pbdrv/watchdog.h>

#if PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER
// Bootloader reads the value at this RAM address to know if BLE firmware loader
// should run or not.
// NB: this can't be static, otherwise section attribute is ignored.
uint32_t pbdrv_reset_stm32_bootloader_selector __attribute__((section(".magic"), used));
#elif PBDRV_CONFIG_RESET_STM32_HAS_MBOOT_BOOTLOADER
// Survives reset since .bootloader_selector is not zero-initialized by startup.
// Checked by the platform's early init to enter mboot. NB: this can't be
// static, otherwise section attribute is ignored.
uint32_t pbdrv_reset_stm32_bootloader_selector __attribute__((section(".bootloader_selector"), used));
#endif

#define BOOTLOADER_HAS_UPDATE_MODE (PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER || PBDRV_CONFIG_RESET_STM32_HAS_MBOOT_BOOTLOADER)

#define BOOTLOADER_FIRMWARE_UPDATE_MODE 0xAAAAAAAA

#if BOOTLOADER_HAS_UPDATE_MODE
/**
 * Returns true if the bootloader should be entered after reset.
 *
 * Flag is always unset after reading.
 */
bool pbdrv_reset_stm32_is_bootloader_requested(void) {
    if (pbdrv_reset_stm32_bootloader_selector == BOOTLOADER_FIRMWARE_UPDATE_MODE) {
        pbdrv_reset_stm32_bootloader_selector = 0;
        return true;
    }
    return false;
}
#endif // BOOTLOADER_HAS_UPDATE_MODE

static pbdrv_reset_reason_t reset_reason;

void pbdrv_reset_init(void) {
    uint32_t status = RCC->CSR;

    if (status & RCC_CSR_SFTRSTF) {
        reset_reason = PBDRV_RESET_REASON_SOFTWARE;
    } else if (status & RCC_CSR_IWDGRSTF) {
        reset_reason = PBDRV_RESET_REASON_WATCHDOG;
    } else {
        reset_reason = PBDRV_RESET_REASON_NONE;
    }

    // clear flags for next reset
    RCC->CSR |= RCC_CSR_RMVF;
}

void pbdrv_reset(pbdrv_reset_action_t action) {
    switch (action) {
        // Some platforms can't reboot in update mode. In those cases it will
        // just shutdown instead so that update mode can be manually activated.
        #if BOOTLOADER_HAS_UPDATE_MODE
        case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
            pbdrv_reset_stm32_bootloader_selector = BOOTLOADER_FIRMWARE_UPDATE_MODE;
            // fallthrough to PBDRV_RESET_ACTION_RESET
            __attribute__((fallthrough));
        #endif // BOOTLOADER_HAS_UPDATE_MODE

        case PBDRV_RESET_ACTION_RESET:
            NVIC_SystemReset(); // does not return

        default:
            break;
    }

    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    for (;;) {
        // defined in platform/*/platform.c
        pbdrv_reset_power_off();
        pbdrv_watchdog_update();
    }
}

// NB: Individual platforms must override pbdrv_reset_power_off() function in
// platform.c to physically turn off power.
__WEAK void pbdrv_reset_power_off(void) {
}

pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {
    return reset_reason;
}

#endif // PBDRV_CONFIG_RESET_STM32
