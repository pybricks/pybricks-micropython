// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef INTERNAL_IMU_LSM6S3TR_C_STM32_H
#define INTERNAL_IMU_LSM6S3TR_C_STM32_H

#include STM32_H

/**
 * Platform-specific config for the LSM6S3TR-C/STM32 driver.
 */
typedef struct {
    /** The I2C instance the IMU is connected to. */
    I2C_TypeDef *i2c;
} pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t;

extern const pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t pbdrv_imu_lsm6s3tr_c_stm32_platform_data;


void pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_er_irq(void);
void pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_ev_irq(void);

#endif // INTERNAL_IMU_LSM6S3TR_C_STM32_H
