// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// Manages power off and reset for EV3 with TIAM1808.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_EV3

#include <pbdrv/reset.h>
#include <pbdrv/gpio.h>

#include "../drv/gpio/gpio_ev3.h"

#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/timer.h>

#define BOOTLOADER_UPDATE_MODE_VALUE    0x5555AAAA

typedef struct {
    // 0xffff1ff0
    uint32_t _dummy0;
    // 0xffff1ff4
    // Pybricks uses this flag to determine software reset vs other resets
    uint32_t reset_reason_flag;
    // 0xffff1ff8
    // Have not fully investigated this, but the bootloader seems to store
    // the result of (DDR) memory testing at 0xffff1ffa and 0xffff1ffb
    uint32_t _bootloader_unk_ram_test;
    // 0xffff1ffc
    uint32_t bootloader_update_flag;
} persistent_data_t;
// This is defined as an extern variable so that its address can be specified
// in the platform.ld linker script. This means that the linker script
// contains information about *all* fixed memory locations.
//
// This lives at the very end of the ARM local RAM.
extern volatile persistent_data_t ev3_persistent_data;

static const pbdrv_gpio_t poweroff_pin = PBDRV_GPIO_EV3_PIN(13, 19, 16, 6, 11);

void pbdrv_reset_init(void) {
    pbdrv_gpio_out_high(&poweroff_pin);
}

void pbdrv_reset(pbdrv_reset_action_t action) {
    for (;;) {
        switch (action) {
            case PBDRV_RESET_ACTION_POWER_OFF:
                pbdrv_reset_power_off();
                break;
            case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
                ev3_persistent_data.bootloader_update_flag = BOOTLOADER_UPDATE_MODE_VALUE;
                __attribute__((fallthrough));
            default:
                // PBDRV_RESET_ACTION_RESET

                // Poke the watchdog timer with a bad value to immediately trigger it
                HWREG(SOC_TMR_1_REGS + TMR_WDTCR) = 0;
                break;
        }
    }
}

pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {
    return PBDRV_RESET_REASON_NONE;
}

void pbdrv_reset_power_off(void) {
    pbdrv_gpio_out_low(&poweroff_pin);
}

#endif // PBDRV_CONFIG_RESET_EV3
