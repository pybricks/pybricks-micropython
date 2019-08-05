// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO Powered Up Smart Hub

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         11
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         10

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)

#define PBDRV_CONFIG_BLUETOOTH                      (0)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (2)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F0                   (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_LPF2                    (1)
#define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS          (2)
#define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT         PBIO_PORT_A
#define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT          PBIO_PORT_B

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32F0                   (1)
#define PBDRV_CONFIG_UART_STM32F0_NUM_UART          (2)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)

#define PBDRV_CONFIG_FIRST_IO_PORT          PBIO_PORT_A
#define PBDRV_CONFIG_LAST_IO_PORT           PBIO_PORT_B
#define PBDRV_CONFIG_NUM_IO_PORT            (2)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_B
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (2)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 48000000

#endif // _PBDRVCONFIG_H_
