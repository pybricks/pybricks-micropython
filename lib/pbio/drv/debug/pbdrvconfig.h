// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

#define PBDRV_CONFIG_ADC            (1)
#define PBDRV_CONFIG_BATTERY        (0)
#define PBDRV_CONFIG_BLUETOOTH      (0)
#define PBDRV_CONFIG_IOPORT         (1)
#define PBDRV_CONFIG_LIGHT          (1)
#define PBDRV_CONFIG_MOTOR          (0)
#define PBDRV_CONFIG_UART           (1)

#define PBDRV_CONFIG_HAS_PORT_1 (1)

#define PBDRV_CONFIG_FIRST_IO_PORT          PBIO_PORT_1
#define PBDRV_CONFIG_LAST_IO_PORT           PBIO_PORT_1
#define PBDRV_CONFIG_NUM_IO_PORT            (1)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 16000000

#endif // _PBDRVCONFIG_H_
