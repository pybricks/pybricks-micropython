// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/runtime.h"

#include <lsm6ds3tr_c_reg.h>

#include <pybricks/util_pb/pb_imu.h>

#include STM32_HAL_H

#if PYBRICKS_HUB_TECHNICHUB
#include <stm32l4xx_ll_i2c.h>
#endif

struct _pb_imu_dev_t {
    stmdev_ctx_t ctx;
    float_t gyro_scale; // m/s^2 per device count
    float_t accel_scale; // deg/s per device count
};

STATIC pb_imu_dev_t _imu_dev;
STATIC I2C_HandleTypeDef hi2c;

void mod_experimental_IMU_handle_i2c_er_irq(void) {
    HAL_I2C_ER_IRQHandler(&hi2c);
}

void mod_experimental_IMU_handle_i2c_ev_irq(void) {
    HAL_I2C_EV_IRQHandler(&hi2c);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    _imu_dev.ctx.read_write_done = true;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    _imu_dev.ctx.read_write_done = true;
}

// REVISIT: if there is ever an error the PT threads will stall since we aren't
// handling the error callbacks.

STATIC void mod_experimental_IMU_write_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_I2C_Mem_Write_IT(&hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len);
}

STATIC void mod_experimental_IMU_read_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_I2C_Mem_Read_IT(&hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len);
}

STATIC PT_THREAD(pb_imu_configure(struct pt *pt, pb_imu_dev_t *imu_dev)) {
    static struct pt child;
    static uint8_t id;
    static uint8_t rst;
    static stmdev_ctx_t *ctx;

    ctx = &imu_dev->ctx;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_device_id_get(&child, ctx, &id));

    if (id != LSM6DS3TR_C_ID) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("incorrect device id"));
    }

    // Init based on data polling example

    /*
     *  Restore default configuration
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_reset_set(&child, ctx, PROPERTY_ENABLE));
    do {
        PT_SPAWN(pt, &child, lsm6ds3tr_c_reset_get(&child, ctx, &rst));
    } while (rst);
    /*
     *  Enable Block Data Update
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_block_data_update_set(&child, ctx, PROPERTY_ENABLE));
    /*
     * Set Output Data Rate
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_data_rate_set(&child, ctx, LSM6DS3TR_C_XL_ODR_833Hz));
    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_data_rate_set(&child, ctx, LSM6DS3TR_C_GY_ODR_833Hz));
    /*
     * Set scale
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_full_scale_set(&child, ctx, LSM6DS3TR_C_2g));
    imu_dev->accel_scale = lsm6ds3tr_c_from_fs2g_to_mg(1) * 0.00981f;

    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_full_scale_set(&child, ctx, LSM6DS3TR_C_250dps));
    imu_dev->gyro_scale = lsm6ds3tr_c_from_fs250dps_to_mdps(1) / 1000.0f;

    /*
     * Configure filtering chain(No aux interface)
     */
    /* Accelerometer - analog filter */
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_filter_analog_set(&child, ctx, LSM6DS3TR_C_XL_ANA_BW_400Hz));

    /* Accelerometer - LPF1 path ( LPF2 not used )*/
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_lp1_bandwidth_set(&child, ctx, LSM6DS3TR_C_XL_LP1_ODR_DIV_4));

    /* Accelerometer - LPF1 + LPF2 path */
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_lp2_bandwidth_set(&child, ctx, LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100));

    /* Accelerometer - High Pass / Slope path */
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_reference_mode_set(&child, ctx, PROPERTY_DISABLE));
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_hp_bandwidth_set(&child, ctx, LSM6DS3TR_C_XL_HP_ODR_DIV_100));

    /* Gyroscope - filtering chain */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_band_pass_set(&child, ctx, LSM6DS3TR_C_HP_16mHz_LP1_LIGHT));

    PT_END(pt);
}

void pb_imu_get_imu(pb_imu_dev_t **imu_dev) {
    *imu_dev = &_imu_dev;
}

void pb_imu_init(pb_imu_dev_t *imu_dev) {

    struct pt pt;

    imu_dev->ctx.write_reg = mod_experimental_IMU_write_reg;
    imu_dev->ctx.read_reg = mod_experimental_IMU_read_reg;

    if (hi2c.Instance == NULL) {
        #if PYBRICKS_HUB_TECHNICHUB
        hi2c.Instance = I2C1;
        // Clock is 5MHz, so these timing come out to 1 usec. When combined with
        // internal delays, this is slightly slower than 400kHz
        hi2c.Init.Timing = __LL_I2C_CONVERT_TIMINGS(0, 0, 0, 4, 4);
        #else
        // On PrimeHub (STM32F4), we set clock speed directly
        hi2c.Instance = I2C2;
        hi2c.Init.ClockSpeed = 400000;
        #endif // PYBRICKS_HUB_TECHNICHUB
        hi2c.Init.OwnAddress1 = 0;
        hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        HAL_StatusTypeDef ret = HAL_I2C_Init(&hi2c);
        if (ret != HAL_OK) {
            hi2c.Instance = NULL;
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to init I2C"));
        }
    }

    PT_INIT(&pt);
    while (PT_SCHEDULE(pb_imu_configure(&pt, imu_dev))) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            MICROPY_EVENT_POLL_HOOK
            nlr_pop();
        } else {
            HAL_I2C_Master_Abort_IT(&hi2c, LSM6DS3TR_C_I2C_ADD_L);
            nlr_jump(nlr.ret_val);
        }
    }
}

void pb_imu_accel_read(pb_imu_dev_t *imu_dev, float_t *values) {
    struct pt pt;
    int16_t data[3];

    PT_INIT(&pt);
    while (PT_SCHEDULE(lsm6ds3tr_c_acceleration_raw_get(&pt, &imu_dev->ctx, (uint8_t *)data))) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            MICROPY_EVENT_POLL_HOOK
            nlr_pop();
        } else {
            HAL_I2C_Master_Abort_IT(&hi2c, LSM6DS3TR_C_I2C_ADD_L);
            nlr_jump(nlr.ret_val);
        }
    }
    values[0] = data[0] * imu_dev->accel_scale;
    values[1] = data[1] * imu_dev->accel_scale;
    values[2] = data[2] * imu_dev->accel_scale;
}

void pb_imu_gyro_read(pb_imu_dev_t *imu_dev, float_t *values) {
    struct pt pt;
    int16_t data[3];

    PT_INIT(&pt);
    while (PT_SCHEDULE(lsm6ds3tr_c_angular_rate_raw_get(&pt, &imu_dev->ctx, (uint8_t *)data))) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            MICROPY_EVENT_POLL_HOOK
            nlr_pop();
        } else {
            HAL_I2C_Master_Abort_IT(&hi2c, LSM6DS3TR_C_I2C_ADD_L);
            nlr_jump(nlr.ret_val);
        }
    }
    values[0] = data[0] * imu_dev->gyro_scale;
    values[1] = data[1] * imu_dev->gyro_scale;
    values[2] = data[2] * imu_dev->gyro_scale;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
