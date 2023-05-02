// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

#define PBIO_CONFIG_BATTERY                 (1)
#define PBIO_CONFIG_DCMOTOR                 (1)
#define PBIO_CONFIG_DCMOTOR_NUM_DEV         (4)
#define PBIO_CONFIG_DRIVEBASE_SPIKE         (0)
#define PBIO_CONFIG_EV3_INPUT_DEVICE        (1)
#define PBIO_CONFIG_IMU                     (0)
#define PBIO_CONFIG_LIGHT                   (1)
#define PBIO_CONFIG_LOGGER                  (1)
#define PBIO_CONFIG_SERIAL                  (1)
#define PBIO_CONFIG_MOTOR_PROCESS           (1)
#define PBIO_CONFIG_SERVO                   (1)
#define PBIO_CONFIG_SERVO_NUM_DEV           (4)
#define PBIO_CONFIG_SERVO_EV3_NXT           (1)
#define PBIO_CONFIG_SERVO_PUP               (0)
#define PBIO_CONFIG_SERVO_PUP_MOVE_HUB      (0)
#define PBIO_CONFIG_TACHO                   (1)

// On ev3dev, we can't keep up with a 5 ms loop.
#define PBIO_CONFIG_CONTROL_LOOP_TIME_MS    (10)
