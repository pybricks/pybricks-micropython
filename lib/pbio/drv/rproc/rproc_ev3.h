// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_EV3_H_
#define _INTERNAL_PBDRV_RPROC_EV3_H_

#include <stdint.h>

// EV3 PRU interfaces and ABI
// These need to match the interface expected by the PRU firmware

// PRU1 interface

#define PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS   4

typedef struct {
    uint8_t pwms[PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS];
} pbdrv_rproc_ev3_pru1_shared_ram_t;

extern volatile pbdrv_rproc_ev3_pru1_shared_ram_t pbdrv_rproc_ev3_pru1_shared_ram;

#endif // _INTERNAL_PBDRV_RPROC_EV3_H_
