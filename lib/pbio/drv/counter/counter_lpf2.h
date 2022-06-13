// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PVDRV_COUNTER_LPF2_H_
#define _INTERNAL_PVDRV_COUNTER_LPF2_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_LPF2

#if !PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV
#endif

#include <stdint.h>

#include <pbdrv/counter.h>
#include <pbio/port.h>

typedef struct {
    /** The ID of the counter device provided by this instance. */
    uint8_t counter_id;
    /** The I/O port identifier this counter is associated with. */
    pbio_port_id_t port_id;
} pbdrv_counter_lpf2_platform_data_t;

// defined in platform/*/platform.c
extern const pbdrv_counter_lpf2_platform_data_t
    pbdrv_counter_lpf2_platform_data[PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV];

void pbdrv_counter_lpf2_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_LPF2

#define pbdrv_counter_lpf2_init(devs)

#endif // PBDRV_CONFIG_COUNTER_LPF2

#endif // _INTERNAL_PVDRV_COUNTER_LPF2_H_
