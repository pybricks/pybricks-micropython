// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_CONF_H_
#define _PBIO_CONF_H_

#include <stdint.h>

#define CCIF
#define CLIF
#define AUTOSTART_ENABLE 0

typedef uint32_t clock_time_t;
#define CLOCK_CONF_SECOND 1000

// extra clock function just for tests
void clock_tick(clock_time_t ticks);

#endif /* _PBIO_CONF_H_ */
