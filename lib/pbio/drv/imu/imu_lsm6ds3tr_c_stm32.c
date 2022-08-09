// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

// IMU driver for STMicroelectronics LSM6DS3TR-C accel/gyro connected to STM32 MCU.


#include <pbdrv/config.h>

#if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32

#include <stdint.h>
#include <string.h>

#include <pbdrv/imu.h>

#include <contiki.h>
#include <lsm6ds3tr_c_reg.h>

#include STM32_HAL_H

#if defined(STM32L4)
#include <stm32l4xx_ll_i2c.h>
#endif

#include "../core.h"
#include "./imu_lsm6ds3tr_c_stm32.h"

typedef enum {
    /** Initialization is not complete yet. */
    IMU_INIT_STATE_BUSY,
    /** Initialization failed at some point. */
    IMU_INIT_STATE_FAILED,
    /** Initialization was successful. */
    IMU_INIT_STATE_COMPLETE,
} imu_init_state_t;

struct _pbdrv_imu_dev_t {
    /** Driver context for external library. */
    stmdev_ctx_t ctx;
    /** STM32 HAL I2C context. */
    I2C_HandleTypeDef hi2c;
    /** Scale factor to convert raw data to degrees per second. */
    float gyro_scale;
    /** Scale factor to convert raw data to m/s^2. */
    float accel_scale;
    /** Raw data. */
    int16_t data[7];
    /** Initialization state. */
    imu_init_state_t init_state;
};

static pbdrv_imu_dev_t global_imu_dev;
PROCESS(pbdrv_imu_lsm6ds3tr_c_stm32_process, "LSM6DS3TR-C");

// REVISIT: For now, this driver takes complete ownership of the STM32 I2C
// subsystem. A shared I2C driver would be needed

void pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_er_irq(void) {
    HAL_I2C_ER_IRQHandler(&global_imu_dev.hi2c);
}

void pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_ev_irq(void) {
    HAL_I2C_EV_IRQHandler(&global_imu_dev.hi2c);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

/**
 * Reset the I2C peripheral.
 *
 * Occasionally, I2C transactions will fail. The BUSY flag is stuck on and
 * HAL error flag is set to HAL_I2C_ERROR_AF. To recover, we just reset and
 * reinitialize the I2C peripheral.
 */
static void pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(I2C_HandleTypeDef *hi2c) {
    I2C_TypeDef *I2C = hi2c->Instance;

    I2C->CR1 |= I2C_CR1_SWRST;
    I2C->CR1 &= ~I2C_CR1_SWRST;

    HAL_I2C_Init(hi2c);
}

static void pbdrv_imu_lsm6ds3tr_c_stm32_write_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_IT(&global_imu_dev.hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len);

    if (ret != HAL_OK) {
        // If there was an error, the interrupt will never come so we have to set the flag here.
        global_imu_dev.ctx.read_write_done = true;
    }
}

static void pbdrv_imu_lsm6ds3tr_c_stm32_read_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_IT(&global_imu_dev.hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len);

    if (ret != HAL_OK) {
        // If there was an error, the interrupt will never come so we have to set the flag here.
        global_imu_dev.ctx.read_write_done = true;
    }
}

static PT_THREAD(pbdrv_imu_lsm6ds3tr_c_stm32_init(struct pt *pt)) {
    const pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t *pdata = &pbdrv_imu_lsm6s3tr_c_stm32_platform_data;
    pbdrv_imu_dev_t *imu_dev = &global_imu_dev;
    I2C_HandleTypeDef *hi2c = &imu_dev->hi2c;
    stmdev_ctx_t *ctx = &imu_dev->ctx;

    static struct pt child;
    static uint8_t id;
    static uint8_t rst;

    PT_BEGIN(pt);

    ctx->write_reg = pbdrv_imu_lsm6ds3tr_c_stm32_write_reg;
    ctx->read_reg = pbdrv_imu_lsm6ds3tr_c_stm32_read_reg;

    hi2c->Instance = pdata->i2c;
    #if defined(STM32L4)
    // HACK: This is hard-coded for Technic hub.
    // Clock is 5MHz, so these timing come out to 1 usec. When combined with
    // internal delays, this is slightly slower than 400kHz
    hi2c->Init.Timing = __LL_I2C_CONVERT_TIMINGS(0, 0, 0, 4, 4);
    #else
    hi2c->Init.ClockSpeed = 400000;
    #endif
    hi2c->Init.OwnAddress1 = 0;
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(hi2c);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_device_id_get(&child, ctx, &id));

    if (id != LSM6DS3TR_C_ID) {
        imu_dev->init_state = IMU_INIT_STATE_FAILED;
        PT_EXIT(pt);
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
    PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_full_scale_set(&child, ctx, LSM6DS3TR_C_8g));
    imu_dev->accel_scale = lsm6ds3tr_c_from_fs8g_to_mg(1) * 9.81f;

    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_full_scale_set(&child, ctx, LSM6DS3TR_C_1000dps));
    imu_dev->gyro_scale = lsm6ds3tr_c_from_fs1000dps_to_mdps(1) / 1000.0f;

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
    // PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_band_pass_set(&child, ctx, LSM6DS3TR_C_HP_16mHz_LP1_LIGHT));

    if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_NONE) {
        imu_dev->init_state = IMU_INIT_STATE_FAILED;
        PT_EXIT(pt);
    }

    imu_dev->init_state = IMU_INIT_STATE_COMPLETE;

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_imu_lsm6ds3tr_c_stm32_process, ev, data) {
    pbdrv_imu_dev_t *imu_dev = &global_imu_dev;
    I2C_HandleTypeDef *hi2c = &imu_dev->hi2c;

    static struct pt child;
    static uint8_t buf[6];

    PROCESS_BEGIN();

    PROCESS_PT_SPAWN(&child, pbdrv_imu_lsm6ds3tr_c_stm32_init(&child));

    pbdrv_init_busy_down();

    if (imu_dev->init_state != IMU_INIT_STATE_COMPLETE) {
        // The IMU is not essential. It just won't be available if init fails.
        PROCESS_EXIT();
    }

    for (;;) {
        PROCESS_PT_SPAWN(&child, lsm6ds3tr_c_acceleration_raw_get(&child, &imu_dev->ctx, buf));

        if (HAL_I2C_GetError(hi2c) == HAL_I2C_ERROR_NONE) {
            memcpy(&imu_dev->data[0], buf, 6);
        } else {
            pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
        }

        PROCESS_PT_SPAWN(&child, lsm6ds3tr_c_angular_rate_raw_get(&child, &imu_dev->ctx, buf));

        if (HAL_I2C_GetError(hi2c) == HAL_I2C_ERROR_NONE) {
            memcpy(&imu_dev->data[3], buf, 6);
        } else {
            pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
        }

        PROCESS_PT_SPAWN(&child, lsm6ds3tr_c_temperature_raw_get(&child, &imu_dev->ctx, buf));

        if (HAL_I2C_GetError(hi2c) == HAL_I2C_ERROR_NONE) {
            memcpy(&imu_dev->data[6], buf, 2);
        } else {
            pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
        }
    }

    PROCESS_END();
}

// internal driver interface implementation

void pbdrv_imu_init(void) {
    pbdrv_init_busy_up();
    process_start(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

// public driver interface implementation

pbio_error_t pbdrv_imu_get_imu(pbdrv_imu_dev_t **imu_dev) {
    *imu_dev = &global_imu_dev;

    if ((*imu_dev)->init_state == IMU_INIT_STATE_BUSY) {
        return PBIO_ERROR_AGAIN;
    }

    if ((*imu_dev)->init_state == IMU_INIT_STATE_FAILED) {
        return PBIO_ERROR_FAILED;
    }

    return PBIO_SUCCESS;
}

void pbdrv_imu_accel_read(pbdrv_imu_dev_t *imu_dev, float *values) {
    #if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_INVERTED
    // Sensor is upside down
    values[0] = -imu_dev->data[0] * imu_dev->accel_scale;
    values[1] = imu_dev->data[1] * imu_dev->accel_scale;
    values[2] = -imu_dev->data[2] * imu_dev->accel_scale;
    #else
    values[0] = imu_dev->data[0] * imu_dev->accel_scale;
    values[1] = imu_dev->data[1] * imu_dev->accel_scale;
    values[2] = imu_dev->data[2] * imu_dev->accel_scale;
    #endif
}

void pbdrv_imu_gyro_read(pbdrv_imu_dev_t *imu_dev, float *values) {
    #if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_INVERTED
    // Sensor is upside down
    values[0] = -imu_dev->data[3] * imu_dev->gyro_scale;
    values[1] = imu_dev->data[4] * imu_dev->gyro_scale;
    values[2] = -imu_dev->data[5] * imu_dev->gyro_scale;
    #else
    values[0] = imu_dev->data[3] * imu_dev->gyro_scale;
    values[1] = imu_dev->data[4] * imu_dev->gyro_scale;
    values[2] = imu_dev->data[5] * imu_dev->gyro_scale;
    #endif
}

float pbdrv_imu_temperature_read(pbdrv_imu_dev_t *imu_dev) {
    return lsm6ds3tr_c_from_lsb_to_celsius(imu_dev->data[6]);
}

#endif // PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32
