// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// IMU driver for STMicroelectronics LSM6DS3TR-C accel/gyro connected to STM32 MCU.


#include <pbdrv/config.h>

#if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/imu.h>
#include <pbio/orientation.h>

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
    int16_t data[6];
    /** Raw data point to which new samples are compared to detect stationary. */
    int16_t stationary_data_start[6];
    /** Sum of gyro samples during the stationary period. */
    int32_t stationary_gyro_data_sum[3];
    /** Number of sequential stationary samples. */
    uint32_t stationary_sample_count;
    /** Whether it is currently stationary, to be polled by higher level APIs. */
    bool stationary_now;
    /** Initialization state. */
    imu_init_state_t init_state;
    /** INT1 oneshot. */
    volatile bool int1;
};

/** The size of the data field in pbdrv_imu_dev_t in bytes. */
#define NUM_DATA_BYTES sizeof(((struct _pbdrv_imu_dev_t *)0)->data)

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

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    global_imu_dev.ctx.read_write_done = true;
    process_poll(&pbdrv_imu_lsm6ds3tr_c_stm32_process);
}

void pbdrv_imu_lsm6ds3tr_c_stm32_handle_int1_irq(void) {
    global_imu_dev.int1 = true;
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
     * Set Output Data Rate
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_data_rate_set(&child, ctx, LSM6DS3TR_C_XL_ODR_833Hz));
    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_data_rate_set(&child, ctx, LSM6DS3TR_C_GY_ODR_833Hz));

    /*
     * Set scale
     */
    PT_SPAWN(pt, &child, lsm6ds3tr_c_xl_full_scale_set(&child, ctx, LSM6DS3TR_C_8g));
    imu_dev->accel_scale = lsm6ds3tr_c_from_fs8g_to_mg(1) * 9.81f;

    PT_SPAWN(pt, &child, lsm6ds3tr_c_gy_full_scale_set(&child, ctx, LSM6DS3TR_C_2000dps));
    imu_dev->gyro_scale = lsm6ds3tr_c_from_fs2000dps_to_mdps(1) / 1000.0f;

    // Configure INT1 to trigger when new gyro data is ready.
    PT_SPAWN(pt, &child, lsm6ds3tr_c_pin_int1_route_set(&child, ctx, (lsm6ds3tr_c_int1_route_t) {
        .int1_drdy_g = 1,
    }));

    // If we leave the default latched mode, sometimes we don't get the INT1 interrupt.
    PT_SPAWN(pt, &child, lsm6ds3tr_c_data_ready_mode_set(&child, ctx, LSM6DS3TR_C_DRDY_PULSED));

    // Enable rounding mode so we can get gyro + accel in continuous reads.
    PT_SPAWN(pt, &child, lsm6ds3tr_c_rounding_mode_set(&child, ctx, LSM6DS3TR_C_ROUND_GY_XL));

    if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_NONE) {
        imu_dev->init_state = IMU_INIT_STATE_FAILED;
        PT_EXIT(pt);
    }

    imu_dev->init_state = IMU_INIT_STATE_COMPLETE;

    PT_END(pt);
}

// REVISIT: Should be selected based on expected noise characteristics given the selected parameters
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_ACCL_NOISE (60)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_GYRO_NOISE (10)

static inline bool bounded(int16_t diff, int16_t threshold) {
    return diff < threshold && diff > -threshold;
}

static void pbdrv_imu_lsm6ds3tr_c_stm32_update_stationary_status(pbdrv_imu_dev_t *imu_dev) {

    // Check whether still stationary compared to constant start sample.
    if (bounded(imu_dev->data[0] - imu_dev->stationary_data_start[0], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_GYRO_NOISE) &&
        bounded(imu_dev->data[1] - imu_dev->stationary_data_start[1], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_GYRO_NOISE) &&
        bounded(imu_dev->data[2] - imu_dev->stationary_data_start[2], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_GYRO_NOISE) &&
        bounded(imu_dev->data[3] - imu_dev->stationary_data_start[3], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_ACCL_NOISE) &&
        bounded(imu_dev->data[4] - imu_dev->stationary_data_start[4], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_ACCL_NOISE) &&
        bounded(imu_dev->data[5] - imu_dev->stationary_data_start[5], PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_ACCL_NOISE)
        ) {
        // Still not moved, so increment stationary sample counter.
        imu_dev->stationary_sample_count++;

        // Update sum of gyro data so they can be used to update offset later.
        imu_dev->stationary_gyro_data_sum[0] += imu_dev->data[0];
        imu_dev->stationary_gyro_data_sum[1] += imu_dev->data[1];
        imu_dev->stationary_gyro_data_sum[2] += imu_dev->data[2];
    } else {
        // Not stationary anymore, so reset. Current sample becomes new start.
        memcpy(&imu_dev->stationary_data_start[0], &imu_dev->data[0], NUM_DATA_BYTES);
        imu_dev->stationary_now = false;

        // Reset counter and gyro sum data so we can start over.
        imu_dev->stationary_sample_count = 0;
        imu_dev->stationary_gyro_data_sum[0] = 0;
        imu_dev->stationary_gyro_data_sum[1] = 0;
        imu_dev->stationary_gyro_data_sum[2] = 0;
    }

    // After about one second of stationary data, poke higher level code to update gyro bias.
    if (imu_dev->stationary_sample_count == 833) { // REVISIT: get from ODR config value

        // We can confidently tell external APIs now that we are really stationary.
        imu_dev->stationary_now = true;

        // Send average gyro data up for maintaining gyro bias or filtering.
        float average_gyro_data[] = {
            PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_X *imu_dev->stationary_gyro_data_sum[0] * imu_dev->gyro_scale / imu_dev->stationary_sample_count,
            PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Y *imu_dev->stationary_gyro_data_sum[1] * imu_dev->gyro_scale / imu_dev->stationary_sample_count,
            PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Z *imu_dev->stationary_gyro_data_sum[2] * imu_dev->gyro_scale / imu_dev->stationary_sample_count,
        };
        pbio_orientation_imu_update_gyro_rate_bias(average_gyro_data);

        // Reset counter and gyro sum data so we can start over.
        imu_dev->stationary_sample_count = 0;
        imu_dev->stationary_gyro_data_sum[0] = 0;
        imu_dev->stationary_gyro_data_sum[1] = 0;
        imu_dev->stationary_gyro_data_sum[2] = 0;
    }
}

PROCESS_THREAD(pbdrv_imu_lsm6ds3tr_c_stm32_process, ev, data) {
    pbdrv_imu_dev_t *imu_dev = &global_imu_dev;
    I2C_HandleTypeDef *hi2c = &imu_dev->hi2c;

    static struct pt child;
    static uint8_t buf[NUM_DATA_BYTES];

    PROCESS_BEGIN();

    PROCESS_PT_SPAWN(&child, pbdrv_imu_lsm6ds3tr_c_stm32_init(&child));

    pbdrv_init_busy_down();

    if (imu_dev->init_state != IMU_INIT_STATE_COMPLETE) {
        // The IMU is not essential. It just won't be available if init fails.
        PROCESS_EXIT();
    }

retry:
    // Write the register address of the start of the gyro and accel data.
    buf[0] = LSM6DS3TR_C_OUTX_L_G;
    imu_dev->ctx.read_write_done = false;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Seq_Transmit_IT(
        &imu_dev->hi2c, LSM6DS3TR_C_I2C_ADD_L, buf, 1, I2C_FIRST_FRAME);

    if (ret != HAL_OK) {
        pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
        goto retry;
    }

    PROCESS_WAIT_UNTIL(imu_dev->ctx.read_write_done);

    if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_NONE) {
        pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
        goto retry;
    }

    // Since we configured the IMU to enable "rounding" on the accel and gyro
    // data registers, we can just keep reading forever and it automatically
    // loops around. This way we don't have to keep writing the register
    // value each time we want to read new data. This saves CPU usage since
    // we have fewer interrupts per sample.

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(atomic_exchange(&imu_dev->int1, false));

        imu_dev->ctx.read_write_done = false;
        ret = HAL_I2C_Master_Seq_Receive_IT(
            &imu_dev->hi2c, LSM6DS3TR_C_I2C_ADD_L, buf, NUM_DATA_BYTES, I2C_NEXT_FRAME);

        if (ret != HAL_OK) {
            pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
            goto retry;
        }

        PROCESS_WAIT_UNTIL(imu_dev->ctx.read_write_done);

        if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_NONE) {
            pbdrv_imu_lsm6ds3tr_c_stm32_i2c_reset(hi2c);
            goto retry;
        }

        memcpy(&imu_dev->data[0], buf, NUM_DATA_BYTES);
        pbdrv_imu_lsm6ds3tr_c_stm32_update_stationary_status(imu_dev);
        pbio_orientation_imu_new_data_handler(imu_dev);
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

bool pbdrv_imu_is_stationary(pbdrv_imu_dev_t *imu_dev) {
    return imu_dev->stationary_now;
}

void pbdrv_imu_accel_read(pbdrv_imu_dev_t *imu_dev, float *values) {
    // Output is signed such that we have a right handed coordinate system where:
    // Forward acceleration is +X, upward acceleration is +Z and acceleration to the left is +Y.
    values[0] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_X * imu_dev->data[3] * imu_dev->accel_scale;
    values[1] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Y * imu_dev->data[4] * imu_dev->accel_scale;
    values[2] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Z * imu_dev->data[5] * imu_dev->accel_scale;
}

void pbdrv_imu_gyro_read(pbdrv_imu_dev_t *imu_dev, float *values) {
    // Output is signed such that we have a right handed coordinate system
    // consistent with the coordinate system above. Positive rotations along
    // those axes then follow the right hand rule.
    values[0] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_X * imu_dev->data[0] * imu_dev->gyro_scale;
    values[1] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Y * imu_dev->data[1] * imu_dev->gyro_scale;
    values[2] = PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Z * imu_dev->data[2] * imu_dev->gyro_scale;
}

#endif // PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32
