// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TIAM1808

#include <stdio.h>

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
#include "../../drv/pwm/pwm_ev3.h"
#include "../../drv/gpio/gpio_ev3.h"

// This is the second 64K of the on-chip RAM
#define PRU1_SHARED_RAM_ADDR    0x80010000

// This needs to match the interface expected by the PRU firmware
typedef struct shared_ram {
    uint8_t pwms[4];
} shared_ram;
static volatile shared_ram *const pru1_shared_ram = (volatile shared_ram *)PRU1_SHARED_RAM_ADDR;

static pbio_error_t pbdrv_pwm_tiam1808_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    // TODO: Reimplement this function to use the PRU
    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_tiam1808_funcs = {
    .set_duty = pbdrv_pwm_tiam1808_set_duty,
};

extern char _pru1_start;
extern char _pru1_end;

void pbdrv_pwm_tiam1808_init(pbdrv_pwm_dev_t *devs) {
    // Enable Timer0 "34" half to count up to 256 * 256
    // This is used by the PRU to time the PWM
    TimerPreScalarCount34Set(SOC_TMR_0_REGS, 0);
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER34, 256 * 256 - 1);
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONT);

    // Set GPIO alt modes for the PRU
    pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data[0].gpio_red, SYSCFG_PINMUX13_PINMUX13_11_8_PRU1_R30_12);
    pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data[0].gpio_green, SYSCFG_PINMUX14_PINMUX14_3_0_PRU1_R30_10);
    pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data[1].gpio_red, SYSCFG_PINMUX13_PINMUX13_15_12_PRU1_R30_11);
    pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data[1].gpio_green, SYSCFG_PINMUX13_PINMUX13_7_4_PRU1_R30_13);

    // TODO: Remove this test code
    pru1_shared_ram->pwms[0] = 0x20;    // R
    pru1_shared_ram->pwms[1] = 0xc0;    // G
    pru1_shared_ram->pwms[2] = 0x10;    // R
    pru1_shared_ram->pwms[3] = 0xf0;    // G

    // Enable PRU1 and load its firmware
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_PRU, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    PRUSSDRVPruDisable(1);
    PRUSSDRVPruReset(1);
    unsigned int *fw_start = (unsigned int *)&_pru1_start;
    uint32_t fw_sz = &_pru1_end - &_pru1_start;
    PRUSSDRVPruWriteMemory(PRUSS0_PRU1_IRAM, 0, fw_start, fw_sz);
    // Set constant table C30 to point to 0x80010000
    PRUSSDRVPruSetCTable(1, 30, (PRU1_SHARED_RAM_ADDR >> 8) & 0xffff);
    PRUSSDRVPruEnable(1);

    for (int i = 0; i < PBDRV_CONFIG_PWM_TIAM1808_NUM_DEV; i++) {
        devs[i].funcs = &pbdrv_pwm_tiam1808_funcs;
        devs[i].priv = (pbdrv_pwm_tiam1808_platform_data_t *)&pbdrv_pwm_tiam1808_platform_data[i];
    }
}

#endif // PBDRV_CONFIG_PWM_TIAM1808
