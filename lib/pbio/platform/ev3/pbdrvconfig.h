// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS EV3

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_TIAM1808                 (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST              (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE         (8 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE_USER    (512)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_D                     (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT               PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT                PBIO_PORT_ID_D
