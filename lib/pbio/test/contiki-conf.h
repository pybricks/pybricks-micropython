// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_CONF_H_
#define _PBIO_CONF_H_

#include <stdint.h>

#define CCIF
#define CLIF
#define AUTOSTART_ENABLE 0

typedef uint32_t clock_time_t;
#define CLOCK_CONF_SECOND 1000

// extra clock function just for tests
void pbio_test_clock_tick(uint32_t ticks);

#define clock_time pbdrv_clock_get_ms
#define clock_usecs pbdrv_clock_get_us

#endif /* _PBIO_CONF_H_ */
