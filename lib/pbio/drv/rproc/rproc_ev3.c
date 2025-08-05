// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Manages EV3 PRU coprocessor initialization

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC_EV3

#include <stdbool.h>
#include <string.h>

#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/pruss.h>
#include <tiam1808/psc.h>
#include <tiam1808/timer.h>

#include "rproc_ev3.h"

volatile pbdrv_rproc_ev3_pru1_shared_ram_t pbdrv_rproc_ev3_pru1_shared_ram __attribute__((section(".shared1")));

static bool pbdrv_ev3_rproc_is_ready;

extern char _pru1_start;
extern char _pru1_end;

void pbdrv_rproc_init(void) {
    // Enable PRU subsystem
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_PRU, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // PRU1 initialization

    // Enable Timer0 "34" half to count up to 256 * 256
    // This is used by the PRU to time the PWM
    TimerPreScalarCount34Set(SOC_TMR_0_REGS, 0);
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER34, 256 * 256 - 1);
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONT);

    // Clear shared command memory
    memset((void *)&pbdrv_rproc_ev3_pru1_shared_ram, 0, sizeof(pbdrv_rproc_ev3_pru1_shared_ram));

    // Enable PRU1 and load its firmware
    PRUSSDRVPruDisable(1);
    PRUSSDRVPruReset(1);
    unsigned int *fw_start = (unsigned int *)&_pru1_start;
    uint32_t fw_sz = &_pru1_end - &_pru1_start;
    PRUSSDRVPruWriteMemory(PRUSS0_PRU1_IRAM, 0, fw_start, fw_sz);
    // Set constant table C30 to point to shared memory
    PRUSSDRVPruSetCTable(1, 30, (((uint32_t)&pbdrv_rproc_ev3_pru1_shared_ram) >> 8) & 0xffff);
    PRUSSDRVPruEnable(1);

    // All good
    pbdrv_ev3_rproc_is_ready = true;
}

bool pbdrv_rproc_is_ready(void) {
    return pbdrv_ev3_rproc_is_ready;
}

#endif // PBDRV_CONFIG_RPROC_EV3
