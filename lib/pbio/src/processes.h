// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#ifndef _PBIO_PROCESSES_H_
#define _PBIO_PROCESSES_H_

#include <pbdrv/config.h>

// All of the contiki processes

#if PBDRV_CONFIG_IOPORT_LPF2
PROCESS_NAME(pbdrv_ioport_lpf2_process);
#endif

#endif // _PBIO_PROCESSES_H_
