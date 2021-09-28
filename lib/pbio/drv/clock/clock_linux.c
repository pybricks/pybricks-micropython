// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_LINUX

#include <stdint.h>
#include <time.h>
#include <unistd.h>

void pbdrv_clock_init(void) {
}

uint32_t pbdrv_clock_get_ms(void) {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000 + time_val.tv_nsec / 1000000;
}

uint32_t pbdrv_clock_get_us(void) {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000000 + time_val.tv_nsec / 1000;
}

#endif // PBDRV_CONFIG_CLOCK_LINUX
