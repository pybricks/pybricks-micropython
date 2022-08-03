// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IMU_H_
#define _INTERNAL_PBDRV_IMU_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IMU

#include <pbdrv/imu.h>

void pbdrv_imu_init(void);

#else // PBDRV_CONFIG_IMU

#define pbdrv_imu_init()

#endif // PBDRV_CONFIG_IMU

#endif // _INTERNAL_PBDRV_IMU_H_
