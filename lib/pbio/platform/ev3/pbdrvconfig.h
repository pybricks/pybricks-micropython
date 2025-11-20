// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS EV3

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_EV3                        (1)
#define PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS       (16)

#define PBDRV_CONFIG_CACHE                          (1)
#define PBDRV_CONFIG_CACHE_EV3                      (1)
// The EV3 MMU is configured to additionally map
// VA 0xDxxxxxxx onto PA 0xCxxxxxxx, covering the DDR RAM region.
// The 0xD alias is mapped as non-cacheable and non-bufferable
// and can be used when data needs to be shared with bus-mastering
// hardware peripherals. The 0xD region is unused in the physical
// address map and lies at a convenient offset from the 0xC region.
// The 0xC region, which is the region typically used to access RAM,
// is cacheable and bufferable.
#define PBDRV_CONFIG_CACHE_UNCACHED_OFFSET          (0x10000000)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_TIAM1808                 (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_EV3                    (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_EV3                    (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE          (1 * 1024 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_EV3               (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE          (1 * 1024 * 1024 * 3 / 4)
#define PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE_USER     (512)
#define PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS (10 * 1024 * 1024)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_HAS_ADC                 (1)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (8)

#define PBDRV_CONFIG_I2C                            (1)
#define PBDRV_CONFIG_I2C_EV3                        (1)

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE         515
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK              (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC      (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3          (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_CC2560X      (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND     LWP3_HUB_KIND_TECHNIC_LARGE
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_VARIANT_ADDR 0x08007d80

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (6)
#define PBDRV_CONFIG_BUTTON_INSTANT_RESET           (0)

#define PBDRV_CONFIG_DISPLAY                        (1)
#define PBDRV_CONFIG_DISPLAY_EV3                    (1)
#define PBDRV_CONFIG_DISPLAY_NUM_COLS               (178)
#define PBDRV_CONFIG_DISPLAY_NUM_ROWS               (128)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_EV3                       (1)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_D                     (1)
#define PBDRV_CONFIG_HAS_PORT_1                     (1)
#define PBDRV_CONFIG_HAS_PORT_2                     (1)
#define PBDRV_CONFIG_HAS_PORT_3                     (1)
#define PBDRV_CONFIG_HAS_PORT_4                     (1)

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (3)
#define PBDRV_CONFIG_LED_DUAL                       (1)
#define PBDRV_CONFIG_LED_DUAL_NUM_DEV               (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (2)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_EV3               (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (8)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (2)
#define PBDRV_CONFIG_PWM_TIAM1808                   (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_EV3                      (1)

#define PBDRV_CONFIG_RPROC                          (1)
#define PBDRV_CONFIG_RPROC_EV3                      (1)

#define PBDRV_CONFIG_SOUND                          (1)
#define PBDRV_CONFIG_SOUND_EV3                      (1)
#define PBDRV_CONFIG_SOUND_DEFAULT_VOLUME           75

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_DEBUG_FIRST_PORT          (0)
#define PBDRV_CONFIG_UART_EV3                       (1)
#define PBDRV_CONFIG_UART_EV3_PRU                   (1)
#define PBDRV_CONFIG_UART_EV3_NUM_UART              (4)

#define PBDRV_CONFIG_USB                            (1)
#define PBDRV_CONFIG_USB_MAX_PACKET_SIZE            (512)
#define PBDRV_CONFIG_USB_EV3                        (1)
#define PBDRV_CONFIG_USB_VID                        LEGO_USB_VID
#define PBDRV_CONFIG_USB_PID                        LEGO_USB_PID_EV3
#define PBDRV_CONFIG_USB_MFG_STR                    LEGO_USB_MFG_STR
#define PBDRV_CONFIG_USB_PROD_STR                   LEGO_USB_PROD_STR_EV3 " + Pybricks"

#define PBDRV_CONFIG_STACK                          (1)
#define PBDRV_CONFIG_STACK_EMBEDDED                 (1)

#define PBDRV_CONFIG_WATCHDOG                       (1)
#define PBDRV_CONFIG_WATCHDOG_EV3                   (1)
