// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TIAM1808

#include <stdio.h>
#include <string.h>

#include <pbdrv/pwm.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>

#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/pruss.h>
#include <tiam1808/psc.h>
#include <tiam1808/timer.h>

#include "../drv/pwm/pwm.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_ev3.h"
#include "../../drv/gpio/gpio_ev3.h"

// This needs to match the interface expected by the PRU firmware
typedef struct shared_ram {
    uint8_t pwms[PBDRV_PWM_EV3_NUM_CHANNELS];
} shared_ram;
static volatile shared_ram pru1_shared_ram __attribute__((section(".shared1")));

static pbio_error_t pbdrv_pwm_tiam1808_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    // Blue not available.
    if (ch == PBDRV_LED_PWM_CHANNEL_INVALID) {
        return PBIO_SUCCESS;
    }

    pru1_shared_ram.pwms[ch] = value;
    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_tiam1808_funcs = {
    .set_duty = pbdrv_pwm_tiam1808_set_duty,
};

extern char _pru1_start;
extern char _pru1_end;

#define PINMUX_ALT_PRU1     4

void pbdrv_pwm_tiam1808_init(pbdrv_pwm_dev_t *devs) {
    // Enable Timer0 "34" half to count up to 256 * 256
    // This is used by the PRU to time the PWM
    TimerPreScalarCount34Set(SOC_TMR_0_REGS, 0);
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER34, 256 * 256 - 1);
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONT);

    // Clear shared command memory
    memset((void *)&pru1_shared_ram, 0, sizeof(shared_ram));

    // Enable PRU1 and load its firmware
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_PRU, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    PRUSSDRVPruDisable(1);
    PRUSSDRVPruReset(1);
    unsigned int *fw_start = (unsigned int *)&_pru1_start;
    uint32_t fw_sz = &_pru1_end - &_pru1_start;
    PRUSSDRVPruWriteMemory(PRUSS0_PRU1_IRAM, 0, fw_start, fw_sz);
    // Set constant table C30 to point to shared memory
    PRUSSDRVPruSetCTable(1, 30, (((uint32_t)&pru1_shared_ram) >> 8) & 0xffff);
    PRUSSDRVPruEnable(1);

    devs[0].funcs = &pbdrv_pwm_tiam1808_funcs;

    // Set GPIO alt modes for the PRU
    for (int j = 0; j < PBDRV_PWM_EV3_NUM_CHANNELS; j++) {
        pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data.gpios[j], PINMUX_ALT_PRU1);
    }
}

#endif // PBDRV_CONFIG_PWM_TIAM1808
