// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup MotorProcess pbio/motor_process: Motor control background process.
 *
 * API for Motor control background process.
 * @{
 */

#ifndef _PBIO_MOTOR_PROCESS_H_
#define _PBIO_MOTOR_PROCESS_H_

#include <pbio/config.h>

#if PBIO_CONFIG_MOTOR_PROCESS

void pbio_motor_process_start(void);

#else

static inline void pbio_motor_process_start(void) {
}

#endif // PBIO_CONFIG_MOTOR_PROCESS

#endif // _PBIO_MOTOR_PROCESS_H_

/** @} */
