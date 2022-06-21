// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBDRV_CONFIG_H_
#define _PBDRV_CONFIG_H_

// This file is defined per-platform. It should override the config values below as needed.
#include "pbdrvconfig.h"

// set to (1) if there is a port labeled "A" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_A
#define PBDRV_CONFIG_HAS_PORT_A (0)
#endif

// set to (1) if there is a port labeled "B" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_B
#define PBDRV_CONFIG_HAS_PORT_B (0)
#endif

// set to (1) if there is a port labeled "C" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_C
#define PBDRV_CONFIG_HAS_PORT_C (0)
#endif

// set to (1) if there is a port labeled "D" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_D
#define PBDRV_CONFIG_HAS_PORT_D (0)
#endif

// set to (1) if there is a port labeled "E" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_E
#define PBDRV_CONFIG_HAS_PORT_E (0)
#endif

// set to (1) if there is a port labeled "F" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_F
#define PBDRV_CONFIG_HAS_PORT_F (0)
#endif

// set to (1) if there is a port labeled "1" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_1
#define PBDRV_CONFIG_HAS_PORT_1 (0)
#endif

// set to (1) if there is a port labeled "2" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_2
#define PBDRV_CONFIG_HAS_PORT_2 (0)
#endif

// set to (1) if there is a port labeled "3" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_3
#define PBDRV_CONFIG_HAS_PORT_3 (0)
#endif

// set to (1) if there is a port labeled "4" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_4
#define PBDRV_CONFIG_HAS_PORT_4 (0)
#endif

// the number of built-in motor controllers in the programmable brick
#ifndef PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER (0)
#elif PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// the pbio_port_id_t enum value of the first motor port
#ifndef PBDRV_CONFIG_FIRST_MOTOR_PORT
#error PBDRV_CONFIG_NUM_MOTOR_CONTROLLER requires that PBDRV_CONFIG_FIRST_MOTOR_PORT is defined
#endif

// the pbio_port_id_t enum value of the last motor port
#ifndef PBDRV_CONFIG_LAST_MOTOR_PORT
#error PBDRV_CONFIG_NUM_MOTOR_CONTROLLER requires that PBDRV_CONFIG_LAST_MOTOR_PORT is defined
#endif
#endif

#endif // _PBDRV_CONFIG_H_
