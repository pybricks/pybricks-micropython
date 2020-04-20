// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PBIO_PROCESSES_H_
#define _PBIO_PROCESSES_H_

#include <contiki.h>

#include <pbdrv/config.h>

// All of the contiki processes

#if PBDRV_CONFIG_ADC
PROCESS_NAME(pbdrv_adc_process);
#endif

#if PBDRV_CONFIG_BATTERY
PROCESS_NAME(pbdrv_battery_process);
#endif

#if PBDRV_CONFIG_BLUETOOTH
PROCESS_NAME(pbdrv_bluetooth_hci_process);
PROCESS_NAME(pbdrv_bluetooth_spi_process);
#endif

#if PBDRV_CONFIG_COUNTER
PROCESS_NAME(pbdrv_counter_process);
#endif

#if PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH
PROCESS_NAME(pbdrv_ioport_ev3dev_stretch_process);
#endif

#if PBDRV_CONFIG_IOPORT_LPF2
PROCESS_NAME(pbdrv_ioport_lpf2_process);
#endif

#if PBDRV_CONFIG_UART
PROCESS_NAME(pbdrv_uart_process);
#endif

#if PBIO_CONFIG_UARTDEV
PROCESS_NAME(pbio_uartdev_process);
#endif

#if PBIO_CONFIG_ENABLE_SYS
PROCESS_NAME(pbsys_process);
#endif

#endif // _PBIO_PROCESSES_H_
