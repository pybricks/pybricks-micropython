// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// platform-specific configuration for Raspberry Pi Build HAT

#define PBDRV_CONFIG_ADC                            (0)
// TODO: Implement ADC driver for Build HAT

#define PBDRV_CONFIG_BATTERY                        (0)
#define PBDRV_CONFIG_BATTERY_ADC                    (0)
// TODO: Implement battery driver for Build HAT

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_PICO                     (1)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_PICO                      (1)

#define PBDRV_CONFIG_IOPORT                         (0)
// TODO: Implement IOPORT driver for Build HAT

#define PBDRV_CONFIG_LED                            (0)
// TODO: Implement LED driver for Build HAT (should be similar to EV3)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (0)
// TODO: Implement motor driver for Build HAT

#define PBDRV_CONFIG_PWM                            (0)
// TODO: Implement PWM driver for Build HAT

#define PBDRV_CONFIG_RANDOM                         (0)
// TODO: Implement RANDOM driver for Build HAT

#define PBDRV_CONFIG_RESET                          (0)
// TODO: Implement RESET driver for Build HAT (if applicable)

#define PBDRV_CONFIG_UART                           (0)
// TODO: Implement UART driver for Build HAT

#define PBDRV_CONFIG_STACK                          (1)
#define PBDRV_CONFIG_STACK_EMBEDDED                 (1)

#define PBDRV_CONFIG_WATCHDOG                       (0)
// TODO: Implement WATCHDOG driver for Build HAT

// TODO: replace this with a serial port implementation - there is no USB hardware
#define PBDRV_CONFIG_USB                                    (1)
#define PBDRV_CONFIG_USB_MAX_PACKET_SIZE                    (64)
#define PBDRV_CONFIG_USB_SIMULATION_PICO                    (1)
#define PBDRV_CONFIG_USB_MFG_STR                            u"Raspberry Pi"
#define PBDRV_CONFIG_USB_PROD_STR                           u"Build HAT"

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_D                     (1)
#define PBDRV_CONFIG_HAS_PORT_VCC_CONTROL           (1)
