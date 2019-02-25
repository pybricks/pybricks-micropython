// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBIO_CONF_H_
#define _PBIO_CONF_H_

#include <stdint.h>

#define CCIF
#define CLIF

// TODO: could use 64-bit time struct
typedef uint32_t clock_time_t;
#define CLOCK_CONF_SECOND 1000

#endif /* _PBIO_CONF_H_ */
