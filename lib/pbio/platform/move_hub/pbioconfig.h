// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#define PBIO_CONFIG_BATTERY                 (1)
#define PBIO_CONFIG_DCMOTOR                 (1)
#define PBIO_CONFIG_DRIVEBASE_SPIKE         (0)
#define PBIO_CONFIG_LIGHT                   (1)
#define PBIO_CONFIG_LOGGER                  (0)
#define PBIO_CONFIG_SERVO                   (1)
#define PBIO_CONFIG_SERVO_EV3_NXT           (0)
#define PBIO_CONFIG_SERVO_PUP               (1)
#define PBIO_CONFIG_SERVO_PUP_MOVE_HUB      (1)
#define PBIO_CONFIG_TACHO                   (1)
#define PBIO_CONFIG_CONTROL_MINIMAL         (1)

#define PBIO_CONFIG_UARTDEV                 (1)
#define PBIO_CONFIG_UARTDEV_NUM_DEV         (2)
#define PBIO_CONFIG_UARTDEV_FIRST_PORT      PBIO_PORT_ID_C

#define PBIO_CONFIG_ENABLE_SYS              (1)

// Use shorter window size than default to reduce
// size of static motor position buffer.
#define PBIO_CONFIG_DIFFERENTIATOR_WINDOW_MS (50)
