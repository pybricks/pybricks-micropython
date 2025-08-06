// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_
#define _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_

#include <stdint.h>

// This file is shared between the PRU1 source code
// and the Pybricks source code. It defines the ABI
// between the two codebases and must be kept in sync.

#define PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS   4

typedef struct {
    union {
        // u32 used by the PRU codebase to get more-efficient codegen
        uint32_t pwms;
        // PWM duty cycle for each channel, range [0-255]
        uint8_t pwm_duty_cycle[PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS];
    };
    // Because the PRU needs to manipulate the GPIO direction
    // for doing I2C, and because these registers do *not*
    // support atomic bit access, we give the PRU full ownership
    // of them and route ARM accesses through the PRU.
    uint32_t gpio_bank_01_dir_set;
    uint32_t gpio_bank_01_dir_clr;
} pbdrv_rproc_ev3_pru1_shared_ram_t;

#endif // _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_
