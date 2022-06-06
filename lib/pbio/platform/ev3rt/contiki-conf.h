// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_CONF_H_
#define _PBIO_CONF_H_

#include <stdint.h>

#define CCIF
#define CLIF
#define AUTOSTART_ENABLE 1

typedef uint32_t clock_time_t;
#define CLOCK_CONF_SECOND 1000

#define clock_time pbdrv_clock_get_ms
#define clock_usecs pbdrv_clock_get_us

#define PROCESS_CONF_NO_PROCESS_NAMES 1

#endif /* _PBIO_CONF_H_ */
