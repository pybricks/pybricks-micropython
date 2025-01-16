// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PBIO_PROCESSES_H_
#define _PBIO_PROCESSES_H_

#include <contiki.h>

#include <pbdrv/config.h>
#include <pbio/config.h>

// DO NOT ADD NEW PROCESSES HERE!
// we are trying to get rid of this file.

#if PBDRV_CONFIG_ADC
PROCESS_NAME(pbdrv_adc_process);
#endif

#endif // _PBIO_PROCESSES_H_
