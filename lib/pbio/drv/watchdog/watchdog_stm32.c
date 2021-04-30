// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Watchdog timer driver for STM32 MCUs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_WATCHDOG_STM32

#include STM32_H

void pbdrv_watchdog_init(void) {
    IWDG->KR = 0x5555; // enable register access
    IWDG->PR = IWDG_PR_PR_2; // divide by 64
    #if defined(STM32F0)
    IWDG->RLR = 1875; // 40 kHz / 64 / 1875 = 0.33... Hz => 3 second timeout
    #elif defined(STM32F4) || defined(STM32L4)
    IWDG->RLR = 1500; // 32 kHz / 64 / 1500 = 0.33... Hz => 3 second timeout
    #else
    #error "Unsupported MCU"
    #endif
    IWDG->KR = 0xaaaa; // refresh counter
    IWDG->KR = 0xcccc; // start watchdog timer
}

void pbdrv_watchdog_update(void) {
    IWDG->KR = 0xaaaa;
}

#endif // PBDRV_CONFIG_WATCHDOG_STM32
