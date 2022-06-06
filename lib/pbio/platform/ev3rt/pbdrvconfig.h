// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS EV3 running ev3rt

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_EV3RT                    (1)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_D                     (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT               PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT                PBIO_PORT_ID_D
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER           (0)
