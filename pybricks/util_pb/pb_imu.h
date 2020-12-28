// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PB_IMU_H_
#define _PB_IMU_H_

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/obj.h"

typedef struct _pb_imu_dev_t pb_imu_dev_t;

void pb_imu_get_imu(pb_imu_dev_t **imu_dev);

void pb_imu_init(pb_imu_dev_t *imu_dev);

void pb_imu_accel_read(pb_imu_dev_t *imu_dev, int16_t *data);

void pb_imu_gyro_read(pb_imu_dev_t *imu_dev, int16_t *data);

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#endif // _PB_IMU_H_
