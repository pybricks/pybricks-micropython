/*
 ******************************************************************************
 * @file    lsm6ds3tr_c_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LSM6DS3TR_C driver file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

// Additional modifications
// Copyright (c) 2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include "lsm6ds3tr_c_reg.h"

/**
  * @defgroup    LSM6DS3TR_C
  * @brief       This file provides a set of functions needed to drive the
  *              lsm6ds3tr_c enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_interfaces_functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

/**
  * @brief  Read generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to read
  * @param  data  pointer to buffer that store the data read(ptr)
  * @param  len   number of consecutive register to read
  *
  */
void lsm6ds3tr_c_read_reg(stmdev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len) {
    ctx->read_write_done = false;
    ctx->read_reg(ctx->handle, reg, data, len);
}

/**
  * @brief  Write generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to write
  * @param  data  pointer to data to write in register reg(ptr)
  * @param  len   number of consecutive register to write
  */
void lsm6ds3tr_c_write_reg(stmdev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len) {
    ctx->read_write_done = false;
    ctx->write_reg(ctx->handle, reg, data, len);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float lsm6ds3tr_c_from_fs2g_to_mg(int16_t lsb)
{
  return ((float)lsb * 0.061f);
}

float lsm6ds3tr_c_from_fs4g_to_mg(int16_t lsb)
{
  return ((float)lsb * 0.122f);
}

float lsm6ds3tr_c_from_fs8g_to_mg(int16_t lsb)
{
  return ((float)lsb * 0.244f);
}

float lsm6ds3tr_c_from_fs16g_to_mg(int16_t lsb)
{
  return ((float)lsb * 0.488f);
}

float lsm6ds3tr_c_from_fs125dps_to_mdps(int16_t lsb)
{
  return ((float)lsb * 4.375f);
}

float lsm6ds3tr_c_from_fs250dps_to_mdps(int16_t lsb)
{
  return ((float)lsb * 8.750f);
}

float lsm6ds3tr_c_from_fs500dps_to_mdps(int16_t lsb)
{
  return ((float)lsb * 17.50f);
}

float lsm6ds3tr_c_from_fs1000dps_to_mdps(int16_t lsb)
{
  return ((float)lsb * 35.0f);
}

float lsm6ds3tr_c_from_fs2000dps_to_mdps(int16_t lsb)
{
  return ((float)lsb * 70.0f);
}

float lsm6ds3tr_c_from_lsb_to_celsius(int16_t lsb)
{
  return (((float)lsb / 256.0f) + 25.0f);
}

/**
  * @}
  *
  */


/**
  * @defgroup    LSM6DS3TR_C_data_generation
  * @brief       This section groups all the functions concerning data
  *              generation
  * @{
  *
*/

static uint8_t lsm6ds3tr_data_buffer[8];

/**
  * @brief  Accelerometer full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fs_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_full_scale_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fs_xl_t val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl1_xl->fs_xl = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of fs_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_full_scale_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fs_xl_t *val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl1_xl->fs_xl) {
        case LSM6DS3TR_C_2g:
            *val = LSM6DS3TR_C_2g;
            break;
        case LSM6DS3TR_C_16g:
            *val = LSM6DS3TR_C_16g;
            break;
        case LSM6DS3TR_C_4g:
            *val = LSM6DS3TR_C_4g;
            break;
        case LSM6DS3TR_C_8g:
            *val = LSM6DS3TR_C_8g;
            break;
        default:
            *val = LSM6DS3TR_C_XL_FS_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Accelerometer data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of odr_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_data_rate_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_xl_t val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl1_xl->odr_xl = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of odr_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_data_rate_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_xl_t *val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl1_xl->odr_xl) {
        case LSM6DS3TR_C_XL_ODR_OFF:
            *val = LSM6DS3TR_C_XL_ODR_OFF;
            break;
        case LSM6DS3TR_C_XL_ODR_12Hz5:
            *val = LSM6DS3TR_C_XL_ODR_12Hz5;
            break;
        case LSM6DS3TR_C_XL_ODR_26Hz:
            *val = LSM6DS3TR_C_XL_ODR_26Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_52Hz:
            *val = LSM6DS3TR_C_XL_ODR_52Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_104Hz:
            *val = LSM6DS3TR_C_XL_ODR_104Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_208Hz:
            *val = LSM6DS3TR_C_XL_ODR_208Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_416Hz:
            *val = LSM6DS3TR_C_XL_ODR_416Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_833Hz:
            *val = LSM6DS3TR_C_XL_ODR_833Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_1k66Hz:
            *val = LSM6DS3TR_C_XL_ODR_1k66Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_3k33Hz:
            *val = LSM6DS3TR_C_XL_ODR_3k33Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_6k66Hz:
            *val = LSM6DS3TR_C_XL_ODR_6k66Hz;
            break;
        case LSM6DS3TR_C_XL_ODR_1Hz6:
            *val = LSM6DS3TR_C_XL_ODR_1Hz6;
            break;
        default:
            *val = LSM6DS3TR_C_XL_ODR_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Gyroscope chain full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fs_g in reg CTRL2_G
  */
PT_THREAD(lsm6ds3tr_c_gy_full_scale_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fs_g_t val))
{
    lsm6ds3tr_c_ctrl2_g_t *ctrl2_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl2_g->fs_g = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Gyroscope chain full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of fs_g in reg CTRL2_G
  */
PT_THREAD(lsm6ds3tr_c_gy_full_scale_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fs_g_t *val))
{
    lsm6ds3tr_c_ctrl2_g_t *ctrl2_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl2_g->fs_g) {
        case LSM6DS3TR_C_250dps:
            *val = LSM6DS3TR_C_250dps;
            break;
        case LSM6DS3TR_C_125dps:
            *val = LSM6DS3TR_C_125dps;
            break;
        case LSM6DS3TR_C_500dps:
            *val = LSM6DS3TR_C_500dps;
            break;
        case LSM6DS3TR_C_1000dps:
            *val = LSM6DS3TR_C_1000dps;
            break;
        case LSM6DS3TR_C_2000dps:
            *val = LSM6DS3TR_C_2000dps;
            break;
        default:
            *val = LSM6DS3TR_C_GY_FS_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Gyroscope data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of odr_g in reg CTRL2_G
  */
PT_THREAD(lsm6ds3tr_c_gy_data_rate_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_g_t val))
{
    lsm6ds3tr_c_ctrl2_g_t *ctrl2_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl2_g->odr_g = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Gyroscope data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of odr_g in reg CTRL2_G
  */
PT_THREAD(lsm6ds3tr_c_gy_data_rate_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_g_t *val))
{
    lsm6ds3tr_c_ctrl2_g_t *ctrl2_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL2_G, (uint8_t *)ctrl2_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl2_g->odr_g) {
        case LSM6DS3TR_C_GY_ODR_OFF:
            *val = LSM6DS3TR_C_GY_ODR_OFF;
            break;
        case LSM6DS3TR_C_GY_ODR_12Hz5:
            *val = LSM6DS3TR_C_GY_ODR_12Hz5;
            break;
        case LSM6DS3TR_C_GY_ODR_26Hz:
            *val = LSM6DS3TR_C_GY_ODR_26Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_52Hz:
            *val = LSM6DS3TR_C_GY_ODR_52Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_104Hz:
            *val = LSM6DS3TR_C_GY_ODR_104Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_208Hz:
            *val = LSM6DS3TR_C_GY_ODR_208Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_416Hz:
            *val = LSM6DS3TR_C_GY_ODR_416Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_833Hz:
            *val = LSM6DS3TR_C_GY_ODR_833Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_1k66Hz:
            *val = LSM6DS3TR_C_GY_ODR_1k66Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_3k33Hz:
            *val = LSM6DS3TR_C_GY_ODR_3k33Hz;
            break;
        case LSM6DS3TR_C_GY_ODR_6k66Hz:
            *val = LSM6DS3TR_C_GY_ODR_6k66Hz;
            break;
        default:
            *val = LSM6DS3TR_C_GY_ODR_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of bdu in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_block_data_update_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->bdu = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of bdu in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_block_data_update_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl3_c->bdu;

    PT_END(pt);
}

/**
  * @brief  Weight of XL user offset bits of registers
  *         X_OFS_USR(73h), Y_OFS_USR(74h), Z_OFS_USR(75h).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of usr_off_w in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_xl_offset_weight_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_usr_off_w_t val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl6_c->usr_off_w = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Weight of XL user offset bits of registers
  *         X_OFS_USR(73h), Y_OFS_USR(74h), Z_OFS_USR(75h).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of usr_off_w in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_xl_offset_weight_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_usr_off_w_t *val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl6_c->usr_off_w) {
        case LSM6DS3TR_C_LSb_1mg:
            *val = LSM6DS3TR_C_LSb_1mg;
            break;
        case LSM6DS3TR_C_LSb_16mg:
            *val = LSM6DS3TR_C_LSb_16mg;
            break;
        default:
            *val = LSM6DS3TR_C_WEIGHT_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  High-performance operating mode for accelerometer[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of xl_hm_mode in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_xl_power_mode_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_xl_hm_mode_t val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl6_c->xl_hm_mode = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  High-performance operating mode for accelerometer.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of xl_hm_mode in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_xl_power_mode_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_xl_hm_mode_t *val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl6_c->xl_hm_mode) {
        case LSM6DS3TR_C_XL_HIGH_PERFORMANCE:
            *val = LSM6DS3TR_C_XL_HIGH_PERFORMANCE;
            break;
        case LSM6DS3TR_C_XL_NORMAL:
            *val = LSM6DS3TR_C_XL_NORMAL;
            break;
        default:
            *val = LSM6DS3TR_C_XL_PW_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Source register rounding function on WAKE_UP_SRC (1Bh),
  *         TAP_SRC (1Ch), D6D_SRC (1Dh), STATUS_REG (1Eh) and
  *         FUNC_SRC1 (53h) registers in the primary interface.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of rounding_status in reg CTRL7_G
  */
PT_THREAD(lsm6ds3tr_c_rounding_on_status_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_rounding_status_t val))
{
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl7_g->rounding_status = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Source register rounding function on WAKE_UP_SRC (1Bh),
  *         TAP_SRC (1Ch), D6D_SRC (1Dh), STATUS_REG (1Eh) and
  *         FUNC_SRC1 (53h) registers in the primary interface.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of rounding_status in reg CTRL7_G
  */
PT_THREAD(lsm6ds3tr_c_rounding_on_status_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_rounding_status_t *val))
{
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl7_g->rounding_status) {
        case LSM6DS3TR_C_STAT_RND_DISABLE:
            *val = LSM6DS3TR_C_STAT_RND_DISABLE;
            break;
        case LSM6DS3TR_C_STAT_RND_ENABLE:
            *val = LSM6DS3TR_C_STAT_RND_ENABLE;
            break;
        default:
            *val = LSM6DS3TR_C_STAT_RND_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  High-performance operating mode disable for gyroscope.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of g_hm_mode in reg CTRL7_G
  */
PT_THREAD(lsm6ds3tr_c_gy_power_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_g_hm_mode_t val))
{
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl7_g->g_hm_mode = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  High-performance operating mode disable for gyroscope.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of g_hm_mode in reg CTRL7_G
  */
PT_THREAD(lsm6ds3tr_c_gy_power_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_g_hm_mode_t *val))
{
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl7_g->g_hm_mode) {
        case LSM6DS3TR_C_GY_HIGH_PERFORMANCE:
            *val = LSM6DS3TR_C_GY_HIGH_PERFORMANCE;
            break;
        case LSM6DS3TR_C_GY_NORMAL:
            *val = LSM6DS3TR_C_GY_NORMAL;
            break;
        default:
            *val = LSM6DS3TR_C_GY_PW_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Read all the interrupt/status flag of the device.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    WAKE_UP_SRC, TAP_SRC, D6D_SRC, STATUS_REG,
  *                FUNC_SRC1, FUNC_SRC2, WRIST_TILT_IA, A_WRIST_TILT_Mask
  */
PT_THREAD(lsm6ds3tr_c_all_sources_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_all_sources_t *val))
{
    static struct pt child;

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_SRC, (uint8_t *)&(val->wake_up_src), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_SRC, (uint8_t *)&(val->tap_src), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_D6D_SRC, (uint8_t *)&(val->d6d_src), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STATUS_REG, (uint8_t *)&(val->status_reg), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FUNC_SRC1, (uint8_t *)&(val->func_src1), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FUNC_SRC2, (uint8_t *)&(val->func_src2), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WRIST_TILT_IA, (uint8_t *)&(val->wrist_tilt_ia), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_MASK, (uint8_t *)&(val->a_wrist_tilt_mask), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}
/**
  * @brief  The STATUS_REG register is read by the primary interface[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Registers STATUS_REG
  */
PT_THREAD(lsm6ds3tr_c_status_reg_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_status_reg_t *val))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STATUS_REG, (uint8_t *)val, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of xlda in reg STATUS_REG
  */
PT_THREAD(lsm6ds3tr_c_xl_flag_data_ready_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_status_reg_t *status_reg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STATUS_REG, (uint8_t *)status_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);
    *val = status_reg->xlda;

    PT_END(pt);
}

/**
  * @brief  Gyroscope new data available.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of gda in reg STATUS_REG
  */
PT_THREAD(lsm6ds3tr_c_gy_flag_data_ready_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_status_reg_t *status_reg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STATUS_REG, (uint8_t *)status_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);
    *val = status_reg->gda;

    PT_END(pt);
}

/**
  * @brief  Temperature new data available.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tda in reg STATUS_REG
  */
PT_THREAD(lsm6ds3tr_c_temp_flag_data_ready_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_status_reg_t *status_reg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STATUS_REG, (uint8_t *)status_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);
    *val = status_reg->tda;

    PT_END(pt);
}

/**
  * @brief  Accelerometer axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C.
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_xl_usr_offset_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_X_OFS_USR, buff, 3);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer axis user offset correction xpressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C.
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_xl_usr_offset_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_X_OFS_USR, buff, 3);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Timestamp
  * @brief       This section groups all the functions that manage the
  *              timestamp generation.
  * @{
  *
  */

/**
  * @brief  Enable timestamp count. The count is saved in TIMESTAMP0_REG (40h),
  *         TIMESTAMP1_REG (41h) and TIMESTAMP2_REG (42h).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of timer_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_timestamp_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->timer_en = val;
    if (val != 0x00U) {
        ctrl10_c->func_en = val;
        lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
        PT_WAIT_UNTIL(pt, ctx->read_write_done);
    }

    PT_END(pt);
}

/**
  * @brief  Enable timestamp count. The count is saved in TIMESTAMP0_REG (40h),
  *         TIMESTAMP1_REG (41h) and TIMESTAMP2_REG (42h).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of timer_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_timestamp_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->timer_en;

    PT_END(pt);
}

/**
  * @brief  Timestamp register resolution setting.
  *         Configuration of this bit affects
  *         TIMESTAMP0_REG(40h), TIMESTAMP1_REG(41h),
  *         TIMESTAMP2_REG(42h), STEP_TIMESTAMP_L(49h),
  *         STEP_TIMESTAMP_H(4Ah) and
  *         STEP_COUNT_DELTA(15h) registers.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of timer_hr in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_timestamp_res_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_timer_hr_t val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_dur->timer_hr = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Timestamp register resolution setting.
  *         Configuration of this bit affects
  *         TIMESTAMP0_REG(40h), TIMESTAMP1_REG(41h),
  *         TIMESTAMP2_REG(42h), STEP_TIMESTAMP_L(49h),
  *         STEP_TIMESTAMP_H(4Ah) and
  *         STEP_COUNT_DELTA(15h) registers.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of timer_hr in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_timestamp_res_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_timer_hr_t *val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (wake_up_dur->timer_hr) {
        case LSM6DS3TR_C_LSB_6ms4:
            *val = LSM6DS3TR_C_LSB_6ms4;
            break;
        case LSM6DS3TR_C_LSB_25us:
            *val = LSM6DS3TR_C_LSB_25us;
            break;
        default:
            *val = LSM6DS3TR_C_TS_RES_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Dataoutput
  * @brief       This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Circular burst-mode (rounding) read from output registers
  *         through the primary interface.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of rounding in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_rounding_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_rounding_t val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl5_c->rounding = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Circular burst-mode (rounding) read from output registers
  *         through the primary interface.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of rounding in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_rounding_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_rounding_t *val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl5_c->rounding) {
        case LSM6DS3TR_C_ROUND_DISABLE:
            *val = LSM6DS3TR_C_ROUND_DISABLE;
            break;
        case LSM6DS3TR_C_ROUND_XL:
            *val = LSM6DS3TR_C_ROUND_XL;
            break;
        case LSM6DS3TR_C_ROUND_GY:
            *val = LSM6DS3TR_C_ROUND_GY;
            break;
        case LSM6DS3TR_C_ROUND_GY_XL:
            *val = LSM6DS3TR_C_ROUND_GY_XL;
            break;
        case LSM6DS3TR_C_ROUND_SH1_TO_SH6:
            *val = LSM6DS3TR_C_ROUND_SH1_TO_SH6;
            break;
        case LSM6DS3TR_C_ROUND_XL_SH1_TO_SH6:
            *val = LSM6DS3TR_C_ROUND_XL_SH1_TO_SH6;
            break;
        case LSM6DS3TR_C_ROUND_GY_XL_SH1_TO_SH12:
            *val = LSM6DS3TR_C_ROUND_GY_XL_SH1_TO_SH12;
            break;
        case LSM6DS3TR_C_ROUND_GY_XL_SH1_TO_SH6:
            *val = LSM6DS3TR_C_ROUND_GY_XL_SH1_TO_SH6;
            break;
        default:
            *val = LSM6DS3TR_C_ROUND_OUT_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Temperature data output register (r). L and H registers together
  *         express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_temperature_raw_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_OUT_TEMP_L, buff, 2);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Angular rate sensor. The value is expressed as a 16-bit word in
  *         two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_angular_rate_raw_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_OUTX_L_G, buff, 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Linear acceleration output register. The value is expressed
  *         as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_acceleration_raw_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_OUTX_L_XL, buff, 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  External magnetometer raw data.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_mag_calibrated_raw_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_OUT_MAG_RAW_X_L, buff, 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Read data in FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buffer Data buffer to store FIFO data.
  * @param  len    Number of data to read from FIFO.
  */
PT_THREAD(lsm6ds3tr_c_fifo_raw_data_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buffer,
    uint8_t len))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_DATA_OUT_L, buffer, len);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_common
  * @brief       This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief  Enable access to the embedded functions/sensor hub
  *         configuration registers[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of func_cfg_en in reg FUNC_CFG_ACCESS
  */
PT_THREAD(lsm6ds3tr_c_mem_bank_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_func_cfg_en_t val))
{
    lsm6ds3tr_c_func_cfg_access_t *func_cfg_access = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FUNC_CFG_ACCESS, (uint8_t *)func_cfg_access, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    func_cfg_access->func_cfg_en = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FUNC_CFG_ACCESS, (uint8_t *)func_cfg_access, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable access to the embedded functions/sensor hub configuration
  *         registers[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of func_cfg_en in reg FUNC_CFG_ACCESS
  */
PT_THREAD(lsm6ds3tr_c_mem_bank_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_func_cfg_en_t *val))
{
    lsm6ds3tr_c_func_cfg_access_t *func_cfg_access = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FUNC_CFG_ACCESS,(uint8_t *)func_cfg_access, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (func_cfg_access->func_cfg_en) {
        case LSM6DS3TR_C_USER_BANK:
            *val = LSM6DS3TR_C_USER_BANK;
            break;
        case LSM6DS3TR_C_BANK_B:
            *val = LSM6DS3TR_C_BANK_B;
            break;
        default:
            *val = LSM6DS3TR_C_BANK_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Data-ready pulsed / letched mode[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy_pulsed in reg DRDY_PULSE_CFG
  */
PT_THREAD(lsm6ds3tr_c_data_ready_mode_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_drdy_pulsed_g_t val))
{
    lsm6ds3tr_c_drdy_pulse_cfg_g_t *drdy_pulse_cfg_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G,(uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    drdy_pulse_cfg_g->drdy_pulsed = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G, (uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Data-ready pulsed / letched mode[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of drdy_pulsed in reg DRDY_PULSE_CFG
  */
PT_THREAD(lsm6ds3tr_c_data_ready_mode_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_drdy_pulsed_g_t *val))
{
    lsm6ds3tr_c_drdy_pulse_cfg_g_t *drdy_pulse_cfg_g = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G, (uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (drdy_pulse_cfg_g->drdy_pulsed) {
        case LSM6DS3TR_C_DRDY_LATCHED:
            *val = LSM6DS3TR_C_DRDY_LATCHED;
            break;
        case LSM6DS3TR_C_DRDY_PULSED:
            *val = LSM6DS3TR_C_DRDY_PULSED;
            break;
        default:
            *val = LSM6DS3TR_C_DRDY_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_device_id_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WHO_AM_I, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Software reset. Restore the default values in user registers[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sw_reset in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_reset_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->sw_reset = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Software reset. Restore the default values in user registers[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sw_reset in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_reset_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl3_c->sw_reset;

    PT_END(pt);
}

/**
  * @brief  Big/Little Endian Data selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ble in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_data_format_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_ble_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->ble = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Big/Little Endian Data selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of ble in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_data_format_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_ble_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl3_c->ble) {
        case LSM6DS3TR_C_LSB_AT_LOW_ADD:
            *val = LSM6DS3TR_C_LSB_AT_LOW_ADD;
            break;
        case LSM6DS3TR_C_MSB_AT_LOW_ADD:
            *val = LSM6DS3TR_C_MSB_AT_LOW_ADD;
            break;
        default:
            *val = LSM6DS3TR_C_DATA_FMT_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of if_inc in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_auto_increment_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->if_inc = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of if_inc in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_auto_increment_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl3_c->if_inc;

    PT_END(pt);
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of boot in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_boot_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->boot = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of boot in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_boot_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl3_c->boot;

    PT_END(pt);
}

/**
  * @brief  Linear acceleration sensor self-test enable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of st_xl in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_xl_self_test_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_st_xl_t val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl5_c->st_xl = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Linear acceleration sensor self-test enable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of st_xl in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_xl_self_test_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_st_xl_t *val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl5_c->st_xl) {
        case LSM6DS3TR_C_XL_ST_DISABLE:
            *val = LSM6DS3TR_C_XL_ST_DISABLE;
            break;
        case LSM6DS3TR_C_XL_ST_POSITIVE:
            *val = LSM6DS3TR_C_XL_ST_POSITIVE;
            break;
        case LSM6DS3TR_C_XL_ST_NEGATIVE:
            *val = LSM6DS3TR_C_XL_ST_NEGATIVE;
            break;
        default:
            *val = LSM6DS3TR_C_XL_ST_ND;
            break;
    }
    PT_END(pt);
}

/**
  * @brief  Angular rate sensor self-test enable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of st_g in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_gy_self_test_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_st_g_t val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl5_c->st_g = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Angular rate sensor self-test enable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of st_g in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_gy_self_test_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_st_g_t *val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl5_c->st_g) {
        case LSM6DS3TR_C_GY_ST_DISABLE:
            *val = LSM6DS3TR_C_GY_ST_DISABLE;
            break;
        case LSM6DS3TR_C_GY_ST_POSITIVE:
            *val = LSM6DS3TR_C_GY_ST_POSITIVE;
            break;
        case LSM6DS3TR_C_GY_ST_NEGATIVE:
            *val = LSM6DS3TR_C_GY_ST_NEGATIVE;
            break;
        default:
            *val = LSM6DS3TR_C_GY_ST_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_filters
  * @brief       This section group all the functions concerning the filters
  *              configuration that impact both accelerometer and gyro.
  * @{
  *
  */

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy_mask in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_filter_settling_mask_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->drdy_mask = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy_mask in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_filter_settling_mask_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl4_c->drdy_mask;

    PT_END(pt);
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of slope_fds in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_xl_hp_path_internal_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slope_fds_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->slope_fds = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of slope_fds in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_xl_hp_path_internal_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slope_fds_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (tap_cfg->slope_fds) {
        case LSM6DS3TR_C_USE_SLOPE:
            *val = LSM6DS3TR_C_USE_SLOPE;
            break;
        case LSM6DS3TR_C_USE_HPF:
            *val = LSM6DS3TR_C_USE_HPF;
            break;
        default:
            *val = LSM6DS3TR_C_HP_PATH_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_accelerometer_filters
  * @brief       This section group all the functions concerning the filters
  *              configuration that impact accelerometer in every mode.
  * @{
  *
  */

/**
  * @brief  Accelerometer analog chain bandwidth selection (only for
  *         accelerometer ODR ≥ 1.67 kHz).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of bw0_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_filter_analog_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_bw0_xl_t val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl1_xl->bw0_xl = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer analog chain bandwidth selection (only for
  *         accelerometer ODR ≥ 1.67 kHz).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of bw0_xl in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_filter_analog_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_bw0_xl_t *val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl1_xl->bw0_xl) {
        case LSM6DS3TR_C_XL_ANA_BW_1k5Hz:
            *val = LSM6DS3TR_C_XL_ANA_BW_1k5Hz;
            break;
        case LSM6DS3TR_C_XL_ANA_BW_400Hz:
            *val = LSM6DS3TR_C_XL_ANA_BW_400Hz;
            break;
        default:
            *val = LSM6DS3TR_C_XL_ANA_BW_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_accelerometer_filters
  * @brief       This section group all the functions concerning the filters
  *              configuration that impact accelerometer.
  * @{
  *
  */

/**
  * @brief  Accelerometer digital LPF (LPF1) bandwidth selection LPF2 is
  *         not used.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lpf1_bw_sel in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_lp1_bandwidth_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_lpf1_bw_sel_t val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl1_xl->lpf1_bw_sel = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl8_xl->lpf2_xl_en = 0;
    ctrl8_xl->hp_slope_xl_en = 0;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Accelerometer digital LPF (LPF1) bandwidth selection LPF2
  *         is not used.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of lpf1_bw_sel in reg CTRL1_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_lp1_bandwidth_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_lpf1_bw_sel_t *val))
{
    lsm6ds3tr_c_ctrl1_xl_t *ctrl1_xl = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if ((ctrl8_xl->lpf2_xl_en != 0x00U) || (ctrl8_xl->hp_slope_xl_en != 0x00U)) {
        *val = LSM6DS3TR_C_XL_LP1_NA;
    } else {
        lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL1_XL, (uint8_t *)ctrl1_xl, 1);
        PT_WAIT_UNTIL(pt, ctx->read_write_done);

        switch (ctrl1_xl->lpf1_bw_sel) {
            case LSM6DS3TR_C_XL_LP1_ODR_DIV_2:
                *val = LSM6DS3TR_C_XL_LP1_ODR_DIV_2;
                break;
            case LSM6DS3TR_C_XL_LP1_ODR_DIV_4:
                *val = LSM6DS3TR_C_XL_LP1_ODR_DIV_4;
                break;
            default:
                *val = LSM6DS3TR_C_XL_LP1_NA;
                break;
        }
    }

    PT_END(pt);
}

/**
  * @brief  LPF2 on outputs[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of input_composite in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_lp2_bandwidth_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_input_composite_t val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl8_xl->input_composite = ((uint8_t)val & 0x10U) >> 4;
    ctrl8_xl->hpcf_xl = (uint8_t)val & 0x03U;
    ctrl8_xl->lpf2_xl_en = 1;
    ctrl8_xl->hp_slope_xl_en = 0;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  LPF2 on outputs[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of input_composite in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_lp2_bandwidth_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_input_composite_t *val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if ((ctrl8_xl->lpf2_xl_en == 0x00U) || (ctrl8_xl->hp_slope_xl_en != 0x00U)) {
        *val = LSM6DS3TR_C_XL_LP_NA;
    } else {
        switch ((ctrl8_xl->input_composite << 4) + ctrl8_xl->hpcf_xl) {
            case LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_50:
                *val = LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_50;
                break;
            case LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_100:
                *val = LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_100;
                break;
            case LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_9:
                *val = LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_9;
                break;
            case LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_400:
                *val = LSM6DS3TR_C_XL_LOW_LAT_LP_ODR_DIV_400;
                break;
            case LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_50:
                *val = LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_50;
                break;
            case LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100:
                *val = LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100;
                break;
            case LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_9:
                *val = LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_9;
                break;
            case LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_400:
                *val = LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_400;
                break;
            default:
                *val = LSM6DS3TR_C_XL_LP_NA;
                break;
        }
    }

    PT_END(pt);
}

/**
  * @brief  Enable HP filter reference mode.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of hp_ref_mode in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_reference_mode_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl8_xl->hp_ref_mode = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable HP filter reference mode.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of hp_ref_mode in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_reference_mode_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl8_xl->hp_ref_mode;

    PT_END(pt);
}

/**
  * @brief  High pass/Slope on outputs.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of hpcf_xl in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_hp_bandwidth_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_hpcf_xl_t val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl8_xl->input_composite = 0;
    ctrl8_xl->hpcf_xl = (uint8_t)val & 0x03U;
    ctrl8_xl->hp_slope_xl_en = 1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  High pass/Slope on outputs.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of hpcf_xl in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_xl_hp_bandwidth_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_hpcf_xl_t *val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if (ctrl8_xl->hp_slope_xl_en == 0x00U) {
        *val = LSM6DS3TR_C_XL_HP_NA;
    }
    switch (ctrl8_xl->hpcf_xl) {
        case LSM6DS3TR_C_XL_HP_ODR_DIV_4:
            *val = LSM6DS3TR_C_XL_HP_ODR_DIV_4;
            break;
        case LSM6DS3TR_C_XL_HP_ODR_DIV_100:
            *val = LSM6DS3TR_C_XL_HP_ODR_DIV_100;
            break;
        case LSM6DS3TR_C_XL_HP_ODR_DIV_9:
            *val = LSM6DS3TR_C_XL_HP_ODR_DIV_9;
            break;
        case LSM6DS3TR_C_XL_HP_ODR_DIV_400:
            *val = LSM6DS3TR_C_XL_HP_ODR_DIV_400;
            break;
        default:
            *val = LSM6DS3TR_C_XL_HP_NA;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_gyroscope_filters
  * @brief       This section group all the functions concerning the filters
  *              configuration that impact gyroscope.
  * @{
  *
  */

/**
  * @brief  Gyroscope low pass path bandwidth.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    gyroscope filtering chain configuration.
  */
PT_THREAD(lsm6ds3tr_c_gy_band_pass_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_lpf1_sel_g_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[2];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl7_g->hpm_g = ((uint8_t)val & 0x30U) >> 4;
    ctrl7_g->hp_en_g = ((uint8_t)val & 0x80U) >> 7;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl6_c->ftype = (uint8_t)val & 0x03U;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->lpf1_sel_g = ((uint8_t)val & 0x08U) >> 3;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Gyroscope low pass path bandwidth.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    gyroscope filtering chain
  */
PT_THREAD(lsm6ds3tr_c_gy_band_pass_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_lpf1_sel_g_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_ctrl7_g_t *ctrl7_g = (void *)&lsm6ds3tr_data_buffer[2];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL7_G, (uint8_t *)ctrl7_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch ((ctrl7_g->hp_en_g << 7) + (ctrl7_g->hpm_g << 4) +
            (ctrl4_c->lpf1_sel_g << 3) + ctrl6_c->ftype) {
        case LSM6DS3TR_C_HP_16mHz_LP2:
            *val = LSM6DS3TR_C_HP_16mHz_LP2;
            break;
        case LSM6DS3TR_C_HP_65mHz_LP2:
            *val = LSM6DS3TR_C_HP_65mHz_LP2;
            break;
        case LSM6DS3TR_C_HP_260mHz_LP2:
            *val = LSM6DS3TR_C_HP_260mHz_LP2;
            break;
        case LSM6DS3TR_C_HP_1Hz04_LP2:
            *val = LSM6DS3TR_C_HP_1Hz04_LP2;
            break;
        case LSM6DS3TR_C_HP_DISABLE_LP1_LIGHT:
            *val = LSM6DS3TR_C_HP_DISABLE_LP1_LIGHT;
            break;
        case LSM6DS3TR_C_HP_DISABLE_LP1_NORMAL:
            *val = LSM6DS3TR_C_HP_DISABLE_LP1_NORMAL;
            break;
        case LSM6DS3TR_C_HP_DISABLE_LP_STRONG:
            *val = LSM6DS3TR_C_HP_DISABLE_LP_STRONG;
            break;
        case LSM6DS3TR_C_HP_DISABLE_LP1_AGGRESSIVE:
            *val = LSM6DS3TR_C_HP_DISABLE_LP1_AGGRESSIVE;
            break;
        case LSM6DS3TR_C_HP_16mHz_LP1_LIGHT:
            *val = LSM6DS3TR_C_HP_16mHz_LP1_LIGHT;
            break;
        case LSM6DS3TR_C_HP_65mHz_LP1_NORMAL:
            *val = LSM6DS3TR_C_HP_65mHz_LP1_NORMAL;
            break;
        case LSM6DS3TR_C_HP_260mHz_LP1_STRONG:
            *val = LSM6DS3TR_C_HP_260mHz_LP1_STRONG;
            break;
        case LSM6DS3TR_C_HP_1Hz04_LP1_AGGRESSIVE:
            *val = LSM6DS3TR_C_HP_1Hz04_LP1_AGGRESSIVE;
            break;
        default:
            *val = LSM6DS3TR_C_HP_GY_BAND_NA;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_serial_interface
  * @brief       This section groups all the functions concerning serial
  *              interface management
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sim in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_spi_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_sim_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->sim = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of sim in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_spi_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_sim_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl3_c->sim) {
        case LSM6DS3TR_C_SPI_4_WIRE:
            *val = LSM6DS3TR_C_SPI_4_WIRE;
            break;
        case LSM6DS3TR_C_SPI_3_WIRE:
            *val = LSM6DS3TR_C_SPI_3_WIRE;
            break;
        default:
            *val = LSM6DS3TR_C_SPI_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Disable / Enable I2C interface.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of i2c_disable in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_i2c_interface_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_i2c_disable_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->i2c_disable = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Disable / Enable I2C interface.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of i2c_disable in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_i2c_interface_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_i2c_disable_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl4_c->i2c_disable) {
        case LSM6DS3TR_C_I2C_ENABLE:
            *val = LSM6DS3TR_C_I2C_ENABLE;
            break;
        case LSM6DS3TR_C_I2C_DISABLE:
            *val = LSM6DS3TR_C_I2C_DISABLE;
            break;
        default:
            *val = LSM6DS3TR_C_I2C_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_interrupt_pins
  * @brief       This section groups all the functions that manage
  *              interrup pins
  * @{
  *
  */

/**
  * @brief  Select the signal that need to route on int1 pad[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    configure INT1_CTRL, MD1_CFG, CTRL4_C(den_drdy_int1),
  *                MASTER_CONFIG(drdy_on_int1)
  */
PT_THREAD(lsm6ds3tr_c_pin_int1_route_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_int1_route_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_int1_ctrl_t *int1_ctrl = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_md1_cfg_t *md1_cfg = (void *)&lsm6ds3tr_data_buffer[2];
    lsm6ds3tr_c_md2_cfg_t *md2_cfg = (void *)&lsm6ds3tr_data_buffer[3];
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[4];
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[5];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT1_CTRL, (uint8_t *)int1_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    int1_ctrl->int1_drdy_xl = val.int1_drdy_xl;
    int1_ctrl->int1_drdy_g = val.int1_drdy_g;
    int1_ctrl->int1_boot = val.int1_boot;
    int1_ctrl->int1_fth = val.int1_fth;
    int1_ctrl->int1_fifo_ovr = val.int1_fifo_ovr;
    int1_ctrl->int1_full_flag = val.int1_full_flag;
    int1_ctrl->int1_sign_mot = val.int1_sign_mot;
    int1_ctrl->int1_step_detector = val.int1_step_detector;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_INT1_CTRL, (uint8_t *)int1_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD1_CFG, (uint8_t *)md1_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD2_CFG, (uint8_t *)md2_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    md1_cfg->int1_timer = val.int1_timer;
    md1_cfg->int1_tilt = val.int1_tilt;
    md1_cfg->int1_6d = val.int1_6d;
    md1_cfg->int1_double_tap = val.int1_double_tap;
    md1_cfg->int1_ff = val.int1_ff;
    md1_cfg->int1_wu = val.int1_wu;
    md1_cfg->int1_single_tap = val.int1_single_tap;
    md1_cfg->int1_inact_state = val.int1_inact_state;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MD1_CFG, (uint8_t *)md1_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->den_drdy_int1 = val.den_drdy_int1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->drdy_on_int1 = val.den_drdy_int1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if ((val.int1_6d != 0x00U) ||
        (val.int1_ff != 0x00U) ||
        (val.int1_wu != 0x00U) ||
        (val.int1_single_tap != 0x00U) ||
        (val.int1_double_tap != 0x00U) ||
        (val.int1_inact_state != 0x00U) ||
        (md2_cfg->int2_6d != 0x00U) ||
        (md2_cfg->int2_ff != 0x00U) ||
        (md2_cfg->int2_wu != 0x00U) ||
        (md2_cfg->int2_single_tap != 0x00U) ||
        (md2_cfg->int2_double_tap != 0x00U) ||
        (md2_cfg->int2_inact_state != 0x00U)) {
        tap_cfg->interrupts_enable = PROPERTY_ENABLE;
    } else {
        tap_cfg->interrupts_enable = PROPERTY_DISABLE;
    }

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Select the signal that need to route on int1 pad[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    read INT1_CTRL, MD1_CFG, CTRL4_C(den_drdy_int1),
  *                MASTER_CONFIG(drdy_on_int1)
  */
PT_THREAD(lsm6ds3tr_c_pin_int1_route_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_int1_route_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_int1_ctrl_t *int1_ctrl = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_md1_cfg_t *md1_cfg = (void *)&lsm6ds3tr_data_buffer[2];
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[3];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT1_CTRL, (uint8_t *)int1_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->int1_drdy_xl = int1_ctrl->int1_drdy_xl;
    val->int1_drdy_g = int1_ctrl->int1_drdy_g;
    val->int1_boot = int1_ctrl->int1_boot;
    val->int1_fth = int1_ctrl->int1_fth;
    val->int1_fifo_ovr = int1_ctrl->int1_fifo_ovr;
    val->int1_full_flag = int1_ctrl->int1_full_flag;
    val->int1_sign_mot = int1_ctrl->int1_sign_mot;
    val->int1_step_detector = int1_ctrl->int1_step_detector;

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD1_CFG, (uint8_t *)md1_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->int1_timer = md1_cfg->int1_timer;
    val->int1_tilt = md1_cfg->int1_tilt;
    val->int1_6d = md1_cfg->int1_6d;
    val->int1_double_tap = md1_cfg->int1_double_tap;
    val->int1_ff = md1_cfg->int1_ff;
    val->int1_wu = md1_cfg->int1_wu;
    val->int1_single_tap = md1_cfg->int1_single_tap;
    val->int1_inact_state = md1_cfg->int1_inact_state;

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->den_drdy_int1 = ctrl4_c->den_drdy_int1;
    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG,(uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->den_drdy_int1 = master_config->drdy_on_int1;

    PT_END(pt);
}

/**
  * @brief  Select the signal that need to route on int2 pad[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    INT2_CTRL, DRDY_PULSE_CFG(int2_wrist_tilt), MD2_CFG
  */
PT_THREAD(lsm6ds3tr_c_pin_int2_route_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_int2_route_t val))
{
    lsm6ds3tr_c_int2_ctrl_t *int2_ctrl = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_md1_cfg_t *md1_cfg = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_md2_cfg_t *md2_cfg = (void *)&lsm6ds3tr_data_buffer[2];
    lsm6ds3tr_c_drdy_pulse_cfg_g_t *drdy_pulse_cfg_g = (void *)&lsm6ds3tr_data_buffer[3];
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[4];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT2_CTRL, (uint8_t *)int2_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    int2_ctrl->int2_drdy_xl = val.int2_drdy_xl;
    int2_ctrl->int2_drdy_g = val.int2_drdy_g;
    int2_ctrl->int2_drdy_temp = val.int2_drdy_temp;
    int2_ctrl->int2_fth = val.int2_fth;
    int2_ctrl->int2_fifo_ovr = val.int2_fifo_ovr;
    int2_ctrl->int2_full_flag = val.int2_full_flag;
    int2_ctrl->int2_step_count_ov = val.int2_step_count_ov;
    int2_ctrl->int2_step_delta = val.int2_step_delta;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_INT2_CTRL, (uint8_t *)int2_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD1_CFG, (uint8_t *)md1_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD2_CFG, (uint8_t *)md2_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    md2_cfg->int2_iron = val.int2_iron;
    md2_cfg->int2_tilt = val.int2_tilt;
    md2_cfg->int2_6d = val.int2_6d;
    md2_cfg->int2_double_tap = val.int2_double_tap;
    md2_cfg->int2_ff = val.int2_ff;
    md2_cfg->int2_wu = val.int2_wu;
    md2_cfg->int2_single_tap = val.int2_single_tap;
    md2_cfg->int2_inact_state = val.int2_inact_state;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MD2_CFG, (uint8_t *)md2_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G, (uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    drdy_pulse_cfg_g->int2_wrist_tilt = val.int2_wrist_tilt;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G, (uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if ((md1_cfg->int1_6d != 0x00U) ||
        (md1_cfg->int1_ff != 0x00U) ||
        (md1_cfg->int1_wu != 0x00U) ||
        (md1_cfg->int1_single_tap != 0x00U) ||
        (md1_cfg->int1_double_tap != 0x00U) ||
        (md1_cfg->int1_inact_state != 0x00U) ||
        (val.int2_6d != 0x00U) ||
        (val.int2_ff != 0x00U) ||
        (val.int2_wu != 0x00U) ||
        (val.int2_single_tap != 0x00U) ||
        (val.int2_double_tap != 0x00U) ||
        (val.int2_inact_state != 0x00U)) {
        tap_cfg->interrupts_enable = PROPERTY_ENABLE;
    } else {
        tap_cfg->interrupts_enable = PROPERTY_DISABLE;
    }

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Select the signal that need to route on int2 pad[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    INT2_CTRL, DRDY_PULSE_CFG(int2_wrist_tilt), MD2_CFG
  */
PT_THREAD(lsm6ds3tr_c_pin_int2_route_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_int2_route_t *val))
{
    lsm6ds3tr_c_int2_ctrl_t *int2_ctrl = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_md2_cfg_t *md2_cfg = (void *)&lsm6ds3tr_data_buffer[1];
    lsm6ds3tr_c_drdy_pulse_cfg_g_t *drdy_pulse_cfg_g = (void *)&lsm6ds3tr_data_buffer[2];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT2_CTRL, (uint8_t *)int2_ctrl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->int2_drdy_xl = int2_ctrl->int2_drdy_xl;
    val->int2_drdy_g = int2_ctrl->int2_drdy_g;
    val->int2_drdy_temp = int2_ctrl->int2_drdy_temp;
    val->int2_fth = int2_ctrl->int2_fth;
    val->int2_fifo_ovr = int2_ctrl->int2_fifo_ovr;
    val->int2_full_flag = int2_ctrl->int2_full_flag;
    val->int2_step_count_ov = int2_ctrl->int2_step_count_ov;
    val->int2_step_delta = int2_ctrl->int2_step_delta;

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MD2_CFG, (uint8_t *)md2_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->int2_iron = md2_cfg->int2_iron;
    val->int2_tilt = md2_cfg->int2_tilt;
    val->int2_6d = md2_cfg->int2_6d;
    val->int2_double_tap = md2_cfg->int2_double_tap;
    val->int2_ff = md2_cfg->int2_ff;
    val->int2_wu = md2_cfg->int2_wu;
    val->int2_single_tap = md2_cfg->int2_single_tap;
    val->int2_inact_state = md2_cfg->int2_inact_state;

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_DRDY_PULSE_CFG_G, (uint8_t *)drdy_pulse_cfg_g, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    val->int2_wrist_tilt = drdy_pulse_cfg_g->int2_wrist_tilt;

    PT_END(pt);
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pp_od in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_pin_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pp_od_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->pp_od = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of pp_od in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_pin_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pp_od_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl3_c->pp_od) {
        case LSM6DS3TR_C_PUSH_PULL:
            *val = LSM6DS3TR_C_PUSH_PULL;
            break;
        case LSM6DS3TR_C_OPEN_DRAIN:
            *val = LSM6DS3TR_C_OPEN_DRAIN;
            break;
        default:
            *val = LSM6DS3TR_C_PIN_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of h_lactive in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_pin_polarity_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_h_lactive_t val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl3_c->h_lactive = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of h_lactive in reg CTRL3_C
  */
PT_THREAD(lsm6ds3tr_c_pin_polarity_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_h_lactive_t *val))
{
    lsm6ds3tr_c_ctrl3_c_t *ctrl3_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL3_C, (uint8_t *)ctrl3_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl3_c->h_lactive) {
        case LSM6DS3TR_C_ACTIVE_HIGH:
            *val = LSM6DS3TR_C_ACTIVE_HIGH;
            break;
        case LSM6DS3TR_C_ACTIVE_LOW:
            *val = LSM6DS3TR_C_ACTIVE_LOW;
            break;
        default:
            *val = LSM6DS3TR_C_POLARITY_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of int2_on_int1 in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_all_on_int1_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->int2_on_int1 = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of int2_on_int1 in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_all_on_int1_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl4_c->int2_on_int1;

    PT_END(pt);
}

/**
  * @brief  Latched/pulsed interrupt.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lir in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_int_notification_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_lir_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->lir = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Latched/pulsed interrupt.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of lir in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_int_notification_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_lir_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (tap_cfg->lir) {
        case LSM6DS3TR_C_INT_PULSED:
            *val = LSM6DS3TR_C_INT_PULSED;
            break;
        case LSM6DS3TR_C_INT_LATCHED:
            *val = LSM6DS3TR_C_INT_LATCHED;
            break;
        default:
            *val = LSM6DS3TR_C_INT_MODE;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Wake_Up_event
  * @brief       This section groups all the functions that manage the
  *              Wake Up event generation.
  * @{
  *
  */

/**
  * @brief  Threshold for wakeup.1 LSB = FS_XL / 64.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wk_ths in reg WAKE_UP_THS
  */
PT_THREAD(lsm6ds3tr_c_wkup_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_wake_up_ths_t *wake_up_ths = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_ths->wk_ths = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Threshold for wakeup.1 LSB = FS_XL / 64.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wk_ths in reg WAKE_UP_THS
  */
PT_THREAD(lsm6ds3tr_c_wkup_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_wake_up_ths_t *wake_up_ths = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = wake_up_ths->wk_ths;

    PT_END(pt);
}

/**
  * @brief  Wake up duration event.1LSb = 1 / ODR[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wake_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_wkup_dur_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_dur->wake_dur = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Wake up duration event.1LSb = 1 / ODR[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wake_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_wkup_dur_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = wake_up_dur->wake_dur;

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Activity/Inactivity_detection
  * @brief       This section groups all the functions concerning
  *              activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief  Enables gyroscope Sleep mode.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sleep in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_gy_sleep_mode_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->sleep = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enables gyroscope Sleep mode.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sleep in reg CTRL4_C
  */
PT_THREAD(lsm6ds3tr_c_gy_sleep_mode_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl4_c->sleep;

    PT_END(pt);
}

/**
  * @brief  Enable inactivity function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of inact_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_act_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_inact_en_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->inact_en = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable inactivity function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of inact_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_act_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_inact_en_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (tap_cfg->inact_en) {
        case LSM6DS3TR_C_PROPERTY_DISABLE:
            *val = LSM6DS3TR_C_PROPERTY_DISABLE;
            break;
        case LSM6DS3TR_C_XL_12Hz5_GY_NOT_AFFECTED:
            *val = LSM6DS3TR_C_XL_12Hz5_GY_NOT_AFFECTED;
            break;
        case LSM6DS3TR_C_XL_12Hz5_GY_SLEEP:
            *val = LSM6DS3TR_C_XL_12Hz5_GY_SLEEP;
            break;
        case LSM6DS3TR_C_XL_12Hz5_GY_PD:
            *val = LSM6DS3TR_C_XL_12Hz5_GY_PD;
            break;
        default:
            *val = LSM6DS3TR_C_ACT_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Duration to go in sleep mode.1 LSb = 512 / ODR[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sleep_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_act_sleep_dur_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_dur->sleep_dur = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Duration to go in sleep mode. 1 LSb = 512 / ODR[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sleep_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_act_sleep_dur_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = wake_up_dur->sleep_dur;

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_tap_generator
  * @brief       This section groups all the functions that manage the
  *              tap and double tap event generation.
  * @{
  *
  */

/**
  * @brief  Read the tap / double tap source register.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure of registers from TAP_SRC
  */
PT_THREAD(lsm6ds3tr_c_tap_src_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_tap_src_t *val))
{
    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_SRC, (uint8_t *)val, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable Z direction in tap recognition.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_z_en in reg TAP_CFG
  *
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_z_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->tap_z_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable Z direction in tap recognition.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_z_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_z_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = tap_cfg->tap_z_en;

    PT_END(pt);
}

/**
  * @brief  Enable Y direction in tap recognition.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_y_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_y_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->tap_y_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable Y direction in tap recognition.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_y_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_y_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = tap_cfg->tap_y_en;

    PT_END(pt);
}

/**
  * @brief  Enable X direction in tap recognition.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_x_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_x_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_cfg->tap_x_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable X direction in tap recognition.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_x_en in reg TAP_CFG
  */
PT_THREAD(lsm6ds3tr_c_tap_detection_on_x_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_tap_cfg_t *tap_cfg = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_CFG, (uint8_t *)tap_cfg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = tap_cfg->tap_x_en;

    PT_END(pt);
}

/**
  * @brief  Threshold for tap recognition.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_ths in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_tap_threshold_x_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_ths_6d->tap_ths = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Threshold for tap recognition.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tap_ths in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_tap_threshold_x_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = tap_ths_6d->tap_ths;

    PT_END(pt);
}

/**
  * @brief  Maximum duration is the maximum time of an overthreshold signal
  *         detection to be recognized as a tap event.
  *         The default value of these bits is 00b which corresponds to
  *         4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different
  *         value, 1LSB corresponds to 8*ODR_XL time.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of shock in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_shock_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    int_dur2->shock = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Maximum duration is the maximum time of an overthreshold signal
  *         detection to be recognized as a tap event.
  *         The default value of these bits is 00b which corresponds to
  *         4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different value, 1LSB
  *         corresponds to 8*ODR_XL time.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of shock in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_shock_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = int_dur2->shock;

    PT_END(pt);
}

/**
  * @brief  Quiet time is the time after the first detected tap in which there
  *         must not be any overthreshold event.
  *         The default value of these bits is 00b which corresponds to
  *         2*ODR_XL time.
  *         If the QUIET[1:0] bits are set to a different value, 1LSB
  *         corresponds to 4*ODR_XL time.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of quiet in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_quiet_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    int_dur2->quiet = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Quiet time is the time after the first detected tap in which there
  *         must not be any overthreshold event.
  *         The default value of these bits is 00b which corresponds to
  *         2*ODR_XL time.
  *         If the QUIET[1:0] bits are set to a different value, 1LSB
  *         corresponds to 4*ODR_XL time.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of quiet in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_quiet_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = int_dur2->quiet;

    PT_END(pt);
}

/**
  * @brief  When double tap recognition is enabled, this register expresses the
  *         maximum time between two consecutive detected taps to determine a
  *         double tap event.
  *         The default value of these bits is 0000b which corresponds to
  *         16*ODR_XL time.
  *         If the DUR[3:0] bits are set to a different value,1LSB corresponds
  *         to 32*ODR_XL time.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dur in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_dur_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    int_dur2->dur = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  When double tap recognition is enabled, this register expresses the
  *         maximum time between two consecutive detected taps to determine a
  *         double tap event.
  *         The default value of these bits is 0000b which corresponds to
  *         16*ODR_XL time.
  *         If the DUR[3:0] bits are set to a different value,1LSB corresponds
  *         to 32*ODR_XL time.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dur in reg INT_DUR2
  */
PT_THREAD(lsm6ds3tr_c_tap_dur_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_int_dur2_t *int_dur2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_INT_DUR2, (uint8_t *)int_dur2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = int_dur2->dur;

    PT_END(pt);
}

/**
  * @brief  Single/double-tap event enable/disable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of
  *                                      single_double_tap in reg WAKE_UP_THS
  */
PT_THREAD(lsm6ds3tr_c_tap_mode_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_single_double_tap_t val))
{
    lsm6ds3tr_c_wake_up_ths_t *wake_up_ths = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_ths->single_double_tap = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Single/double-tap event enable/disable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of single_double_tap
  *                                      in reg WAKE_UP_THS
  */
PT_THREAD(lsm6ds3tr_c_tap_mode_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_single_double_tap_t *val))
{
    lsm6ds3tr_c_wake_up_ths_t *wake_up_ths = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_THS, (uint8_t *)wake_up_ths, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (wake_up_ths->single_double_tap) {
        case LSM6DS3TR_C_ONLY_SINGLE:
            *val = LSM6DS3TR_C_ONLY_SINGLE;
            break;
        case LSM6DS3TR_C_BOTH_SINGLE_DOUBLE:
            *val = LSM6DS3TR_C_BOTH_SINGLE_DOUBLE;
            break;
        default:
            *val = LSM6DS3TR_C_TAP_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_ Six_position_detection(6D/4D)
  * @brief       This section groups all the functions concerning six
  *              position detection (6D).
  * @{
  *
  */

/**
  * @brief  LPF2 feed 6D function selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of low_pass_on_6d in
  *                                   reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_6d_feed_data_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_low_pass_on_6d_t val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl8_xl->low_pass_on_6d = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  LPF2 feed 6D function selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of low_pass_on_6d in reg CTRL8_XL
  */
PT_THREAD(lsm6ds3tr_c_6d_feed_data_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_low_pass_on_6d_t *val))
{
    lsm6ds3tr_c_ctrl8_xl_t *ctrl8_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL8_XL, (uint8_t *)ctrl8_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl8_xl->low_pass_on_6d) {
        case LSM6DS3TR_C_ODR_DIV_2_FEED:
            *val = LSM6DS3TR_C_ODR_DIV_2_FEED;
            break;
        case LSM6DS3TR_C_LPF2_FEED:
            *val = LSM6DS3TR_C_LPF2_FEED;
            break;
        default:
            *val = LSM6DS3TR_C_6D_FEED_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Threshold for 4D/6D function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sixd_ths in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_6d_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_sixd_ths_t val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_ths_6d->sixd_ths = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Threshold for 4D/6D function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of sixd_ths in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_6d_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_sixd_ths_t *val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (tap_ths_6d->sixd_ths) {
        case LSM6DS3TR_C_DEG_80:
            *val = LSM6DS3TR_C_DEG_80;
            break;
        case LSM6DS3TR_C_DEG_70:
            *val = LSM6DS3TR_C_DEG_70;
            break;
        case LSM6DS3TR_C_DEG_60:
            *val = LSM6DS3TR_C_DEG_60;
            break;
        case LSM6DS3TR_C_DEG_50:
            *val = LSM6DS3TR_C_DEG_50;
            break;
        default:
            *val = LSM6DS3TR_C_6D_TH_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  4D orientation detection enable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of d4d_en in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_4d_mode_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    tap_ths_6d->d4d_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  4D orientation detection enable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of d4d_en in reg TAP_THS_6D
  */
PT_THREAD(lsm6ds3tr_c_4d_mode_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_tap_ths_6d_t *tap_ths_6d = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_TAP_THS_6D, (uint8_t *)tap_ths_6d, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = tap_ths_6d->d4d_en;

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_free_fall
  * @brief       This section group all the functions concerning the free
  *              fall detection.
  * @{
  *
  */

/**
  * @brief Free-fall duration event. 1LSb = 1 / ODR[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ff_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_ff_dur_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_free_fall_t *free_fall = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    free_fall->ff_dur = (val & 0x1FU);
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    wake_up_dur->ff_dur = (val & 0x20U) >> 5;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Free-fall duration event. 1LSb = 1 / ODR[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ff_dur in reg WAKE_UP_DUR
  */
PT_THREAD(lsm6ds3tr_c_ff_dur_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_wake_up_dur_t *wake_up_dur = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_free_fall_t *free_fall = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_WAKE_UP_DUR, (uint8_t *)wake_up_dur, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = (wake_up_dur->ff_dur << 5) + free_fall->ff_dur;

    PT_END(pt);
}

/**
  * @brief  Free fall threshold setting.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ff_ths in reg FREE_FALL
  */
PT_THREAD(lsm6ds3tr_c_ff_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_ff_ths_t val))
{
    lsm6ds3tr_c_free_fall_t *free_fall = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    free_fall->ff_ths = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Free fall threshold setting.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of ff_ths in reg FREE_FALL
  */
PT_THREAD(lsm6ds3tr_c_ff_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_ff_ths_t *val))
{
    lsm6ds3tr_c_free_fall_t *free_fall = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FREE_FALL, (uint8_t *)free_fall, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (free_fall->ff_ths) {
        case LSM6DS3TR_C_FF_TSH_156mg:
            *val = LSM6DS3TR_C_FF_TSH_156mg;
            break;
        case LSM6DS3TR_C_FF_TSH_219mg:
            *val = LSM6DS3TR_C_FF_TSH_219mg;
            break;
        case LSM6DS3TR_C_FF_TSH_250mg:
            *val = LSM6DS3TR_C_FF_TSH_250mg;
            break;
        case LSM6DS3TR_C_FF_TSH_312mg:
            *val = LSM6DS3TR_C_FF_TSH_312mg;
            break;
        case LSM6DS3TR_C_FF_TSH_344mg:
            *val = LSM6DS3TR_C_FF_TSH_344mg;
            break;
        case LSM6DS3TR_C_FF_TSH_406mg:
            *val = LSM6DS3TR_C_FF_TSH_406mg;
            break;
        case LSM6DS3TR_C_FF_TSH_469mg:
            *val = LSM6DS3TR_C_FF_TSH_469mg;
            break;
        case LSM6DS3TR_C_FF_TSH_500mg:
            *val = LSM6DS3TR_C_FF_TSH_500mg;
            break;
        default:
            *val = LSM6DS3TR_C_FF_TSH_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_fifo
  * @brief       This section group all the functions concerning the
  *              fifo usage
  * @{
  *
  */

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fth in reg FIFO_CTRL1
  */
PT_THREAD(lsm6ds3tr_c_fifo_watermark_set(struct pt *pt, stmdev_ctx_t *ctx, uint16_t val))
{
    lsm6ds3tr_c_fifo_ctrl1_t *fifo_ctrl1 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl1->fth = (uint8_t)(0x00FFU & val);
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL1, (uint8_t *)fifo_ctrl1, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl2->fth = (uint8_t)((0x0700U & val) >> 8);
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fth in reg FIFO_CTRL1
  */
PT_THREAD(lsm6ds3tr_c_fifo_watermark_get(struct pt *pt, stmdev_ctx_t *ctx, uint16_t *val))
{
    lsm6ds3tr_c_fifo_ctrl1_t *fifo_ctrl1 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL1, (uint8_t *)fifo_ctrl1, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ((uint16_t)fifo_ctrl2->fth << 8) + (uint16_t)fifo_ctrl1->fth;

    PT_END(pt);
}

/**
  * @brief  FIFO data level.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    get the values of diff_fifo in reg  FIFO_STATUS1 and
  *                FIFO_STATUS2(diff_fifo), it is recommended to set the
  *                BDU bit.
  */
PT_THREAD(lsm6ds3tr_c_fifo_data_level_get(struct pt *pt, stmdev_ctx_t *ctx, uint16_t *val))
{
    lsm6ds3tr_c_fifo_status1_t *fifo_status1 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_fifo_status2_t *fifo_status2 = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_STATUS1, (uint8_t *)fifo_status1, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_STATUS2, (uint8_t *)fifo_status2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ((uint16_t)fifo_status2->diff_fifo << 8) +
        (uint16_t)fifo_status1->diff_fifo;

    PT_END(pt);
}

/**
  * @brief  FIFO watermark.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    get the values of watermark in reg  FIFO_STATUS2 and
  */
PT_THREAD(lsm6ds3tr_c_fifo_wtm_flag_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_fifo_status2_t *fifo_status2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_STATUS2, (uint8_t *)fifo_status2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = fifo_status2->waterm;

    PT_END(pt);
}

/**
  * @brief  FIFO pattern.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    get the values of fifo_pattern in reg  FIFO_STATUS3 and
  *                FIFO_STATUS4, it is recommended to set the BDU bit
  */
PT_THREAD(lsm6ds3tr_c_fifo_pattern_get(struct pt *pt, stmdev_ctx_t *ctx, uint16_t *val))
{
    lsm6ds3tr_c_fifo_status3_t *fifo_status3 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_fifo_status4_t *fifo_status4 = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_STATUS3, (uint8_t *)fifo_status3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_STATUS4, (uint8_t *)fifo_status4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ((uint16_t)fifo_status4->fifo_pattern << 8) +
        fifo_status3->fifo_pattern;

    PT_END(pt);
}

/**
  * @brief  Batching of temperature data[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fifo_temp_en in reg FIFO_CTRL2
  */
PT_THREAD(lsm6ds3tr_c_fifo_temp_batch_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl2->fifo_temp_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);


    PT_END(pt);
}

/**
  * @brief  Batching of temperature data[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fifo_temp_en in reg FIFO_CTRL2
  */
PT_THREAD(lsm6ds3tr_c_fifo_temp_batch_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = fifo_ctrl2->fifo_temp_en;

    PT_END(pt);
}

/**
  * @brief  Trigger signal for FIFO write operation.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    act on FIFO_CTRL2(timer_pedo_fifo_drdy)
  *                and MASTER_CONFIG(data_valid_sel_fifo)
  */
PT_THREAD(lsm6ds3tr_c_fifo_write_trigger_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_trigger_fifo_t val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl2->timer_pedo_fifo_drdy = (uint8_t)val & 0x01U;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->data_valid_sel_fifo = (((uint8_t)val & 0x02U) >> 1);
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);


    PT_END(pt);
}

/**
  * @brief  Trigger signal for FIFO write operation.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    act on FIFO_CTRL2(timer_pedo_fifo_drdy)
  *                and MASTER_CONFIG(data_valid_sel_fifo)
  */
PT_THREAD(lsm6ds3tr_c_fifo_write_trigger_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_trigger_fifo_t *val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);


    switch ((fifo_ctrl2->timer_pedo_fifo_drdy << 1) +
            fifo_ctrl2->timer_pedo_fifo_drdy) {
        case LSM6DS3TR_C_TRG_XL_GY_DRDY:
            *val = LSM6DS3TR_C_TRG_XL_GY_DRDY;
            break;
        case LSM6DS3TR_C_TRG_STEP_DETECT:
            *val = LSM6DS3TR_C_TRG_STEP_DETECT;
            break;
        case LSM6DS3TR_C_TRG_SH_DRDY:
            *val = LSM6DS3TR_C_TRG_SH_DRDY;
            break;
        default:
            *val = LSM6DS3TR_C_TRG_SH_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief   Enable pedometer step counter and timestamp as 4th
  *          FIFO data set.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of timer_pedo_fifo_en in reg FIFO_CTRL2
  */
PT_THREAD(lsm6ds3tr_c_fifo_pedo_and_timestamp_batch_set(struct pt *pt, stmdev_ctx_t *ctx,
    uint8_t val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl2->timer_pedo_fifo_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable pedometer step counter and timestamp as 4th
  *         FIFO data set.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of timer_pedo_fifo_en in reg FIFO_CTRL2
  */
PT_THREAD(lsm6ds3tr_c_fifo_pedo_and_timestamp_batch_get(struct pt *pt, stmdev_ctx_t *ctx,
    uint8_t *val))
{
    lsm6ds3tr_c_fifo_ctrl2_t *fifo_ctrl2 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL2, (uint8_t *)fifo_ctrl2, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = fifo_ctrl2->timer_pedo_fifo_en;

    PT_END(pt);
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO) for
  *         accelerometer data.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dec_fifo_xl in reg FIFO_CTRL3
  */
PT_THREAD(lsm6ds3tr_c_fifo_xl_batch_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_fifo_xl_t val))
{
    lsm6ds3tr_c_fifo_ctrl3_t *fifo_ctrl3 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl3->dec_fifo_xl = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO) for
  *         accelerometer data.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of dec_fifo_xl in reg FIFO_CTRL3
  */
PT_THREAD(lsm6ds3tr_c_fifo_xl_batch_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_fifo_xl_t *val))
{
    lsm6ds3tr_c_fifo_ctrl3_t *fifo_ctrl3 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl3->dec_fifo_xl) {
        case LSM6DS3TR_C_FIFO_XL_DISABLE:
            *val = LSM6DS3TR_C_FIFO_XL_DISABLE;
            break;
        case LSM6DS3TR_C_FIFO_XL_NO_DEC:
            *val = LSM6DS3TR_C_FIFO_XL_NO_DEC;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_2:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_2;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_3:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_3;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_4:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_4;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_8:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_8;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_16:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_16;
            break;
        case LSM6DS3TR_C_FIFO_XL_DEC_32:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_32;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_XL_DEC_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dec_fifo_gyro in reg FIFO_CTRL3
  */
PT_THREAD(lsm6ds3tr_c_fifo_gy_batch_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_fifo_gyro_t val))
{
    lsm6ds3tr_c_fifo_ctrl3_t *fifo_ctrl3 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl3->dec_fifo_gyro = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of dec_fifo_gyro in reg FIFO_CTRL3
  */
PT_THREAD(lsm6ds3tr_c_fifo_gy_batch_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_fifo_gyro_t *val))
{
    lsm6ds3tr_c_fifo_ctrl3_t *fifo_ctrl3 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL3, (uint8_t *)fifo_ctrl3, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl3->dec_fifo_gyro) {
        case LSM6DS3TR_C_FIFO_GY_DISABLE:
            *val = LSM6DS3TR_C_FIFO_GY_DISABLE;
            break;
        case LSM6DS3TR_C_FIFO_GY_NO_DEC:
            *val = LSM6DS3TR_C_FIFO_GY_NO_DEC;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_2:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_2;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_3:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_3;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_4:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_4;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_8:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_8;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_16:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_16;
            break;
        case LSM6DS3TR_C_FIFO_GY_DEC_32:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_32;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_GY_DEC_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for third data set.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dec_ds3_fifo in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_dataset_3_batch_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_ds3_fifo_t val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl4->dec_ds3_fifo = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for third data set.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of dec_ds3_fifo in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_dataset_3_batch_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_ds3_fifo_t *val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl4->dec_ds3_fifo) {
        case LSM6DS3TR_C_FIFO_DS3_DISABLE:
            *val = LSM6DS3TR_C_FIFO_DS3_DISABLE;
            break;
        case LSM6DS3TR_C_FIFO_DS3_NO_DEC:
            *val = LSM6DS3TR_C_FIFO_DS3_NO_DEC;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_2:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_2;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_3:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_3;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_4:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_4;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_8:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_8;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_16:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_16;
            break;
        case LSM6DS3TR_C_FIFO_DS3_DEC_32:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_32;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_DS3_DEC_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for fourth data set.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of dec_ds4_fifo in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_dataset_4_batch_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_ds4_fifo_t val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl4->dec_ds4_fifo = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO) for
  *          fourth data set.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of dec_ds4_fifo in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_dataset_4_batch_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_dec_ds4_fifo_t *val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl4->dec_ds4_fifo) {
        case LSM6DS3TR_C_FIFO_DS4_DISABLE:
            *val = LSM6DS3TR_C_FIFO_DS4_DISABLE;
            break;
        case LSM6DS3TR_C_FIFO_DS4_NO_DEC:
            *val = LSM6DS3TR_C_FIFO_DS4_NO_DEC;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_2:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_2;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_3:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_3;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_4:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_4;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_8:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_8;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_16:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_16;
            break;
        case LSM6DS3TR_C_FIFO_DS4_DEC_32:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_32;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_DS4_DEC_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief   8-bit data storage in FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of only_high_data in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_xl_gy_8bit_format_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl4->only_high_data = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  8-bit data storage in FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of only_high_data in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_xl_gy_8bit_format_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = fifo_ctrl4->only_high_data;

    PT_END(pt);
}

/**
  * @brief  Sensing chain FIFO stop values memorization at threshold
  *         level.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of stop_on_fth in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_stop_on_wtm_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl4->stop_on_fth = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Sensing chain FIFO stop values memorization at threshold
  *         level.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of stop_on_fth in reg FIFO_CTRL4
  */
PT_THREAD(lsm6ds3tr_c_fifo_stop_on_wtm_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_fifo_ctrl4_t *fifo_ctrl4 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL4, (uint8_t *)fifo_ctrl4, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = fifo_ctrl4->stop_on_fth;

    PT_END(pt);
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fifo_mode in reg FIFO_CTRL5
  */
PT_THREAD(lsm6ds3tr_c_fifo_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fifo_mode_t val))
{
    lsm6ds3tr_c_fifo_ctrl5_t *fifo_ctrl5 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl5->fifo_mode = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of fifo_mode in reg FIFO_CTRL5
  */
PT_THREAD(lsm6ds3tr_c_fifo_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_fifo_mode_t *val))
{
    lsm6ds3tr_c_fifo_ctrl5_t *fifo_ctrl5 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl5->fifo_mode) {
        case LSM6DS3TR_C_BYPASS_MODE:
            *val = LSM6DS3TR_C_BYPASS_MODE;
            break;
        case LSM6DS3TR_C_FIFO_MODE:
            *val = LSM6DS3TR_C_FIFO_MODE;
            break;
        case LSM6DS3TR_C_STREAM_TO_FIFO_MODE:
            *val = LSM6DS3TR_C_STREAM_TO_FIFO_MODE;
            break;
        case LSM6DS3TR_C_BYPASS_TO_STREAM_MODE:
            *val = LSM6DS3TR_C_BYPASS_TO_STREAM_MODE;
            break;
        case LSM6DS3TR_C_STREAM_MODE:
            *val = LSM6DS3TR_C_STREAM_MODE;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  FIFO ODR selection, setting FIFO_MODE also.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of odr_fifo in reg FIFO_CTRL5
  */
PT_THREAD(lsm6ds3tr_c_fifo_data_rate_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_fifo_t val))
{
    lsm6ds3tr_c_fifo_ctrl5_t *fifo_ctrl5 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    fifo_ctrl5->odr_fifo = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  FIFO ODR selection, setting FIFO_MODE also.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of odr_fifo in reg FIFO_CTRL5
  */
PT_THREAD(lsm6ds3tr_c_fifo_data_rate_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_odr_fifo_t *val))
{
    lsm6ds3tr_c_fifo_ctrl5_t *fifo_ctrl5 = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_FIFO_CTRL5, (uint8_t *)fifo_ctrl5, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (fifo_ctrl5->odr_fifo) {
        case LSM6DS3TR_C_FIFO_DISABLE:
            *val = LSM6DS3TR_C_FIFO_DISABLE;
            break;
        case LSM6DS3TR_C_FIFO_12Hz5:
            *val = LSM6DS3TR_C_FIFO_12Hz5;
            break;
        case LSM6DS3TR_C_FIFO_26Hz:
            *val = LSM6DS3TR_C_FIFO_26Hz;
            break;
        case LSM6DS3TR_C_FIFO_52Hz:
            *val = LSM6DS3TR_C_FIFO_52Hz;
            break;
        case LSM6DS3TR_C_FIFO_104Hz:
            *val = LSM6DS3TR_C_FIFO_104Hz;
            break;
        case LSM6DS3TR_C_FIFO_208Hz:
            *val = LSM6DS3TR_C_FIFO_208Hz;
            break;
        case LSM6DS3TR_C_FIFO_416Hz:
            *val = LSM6DS3TR_C_FIFO_416Hz;
            break;
        case LSM6DS3TR_C_FIFO_833Hz:
            *val = LSM6DS3TR_C_FIFO_833Hz;
            break;
        case LSM6DS3TR_C_FIFO_1k66Hz:
            *val = LSM6DS3TR_C_FIFO_1k66Hz;
            break;
        case LSM6DS3TR_C_FIFO_3k33Hz:
            *val = LSM6DS3TR_C_FIFO_3k33Hz;
            break;
        case LSM6DS3TR_C_FIFO_6k66Hz:
            *val = LSM6DS3TR_C_FIFO_6k66Hz;
            break;
        default:
            *val = LSM6DS3TR_C_FIFO_RATE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_DEN_functionality
  * @brief       This section groups all the functions concerning DEN
  *              functionality.
  * @{
  *
  */

/**
  * @brief  DEN active level configuration.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_lh in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_den_polarity_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_lh_t val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl5_c->den_lh = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  DEN active level configuration.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of den_lh in reg CTRL5_C
  */
PT_THREAD(lsm6ds3tr_c_den_polarity_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_lh_t *val))
{
    lsm6ds3tr_c_ctrl5_c_t *ctrl5_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL5_C, (uint8_t *)ctrl5_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl5_c->den_lh) {
        case LSM6DS3TR_C_DEN_ACT_LOW:
            *val = LSM6DS3TR_C_DEN_ACT_LOW;
            break;
        case LSM6DS3TR_C_DEN_ACT_HIGH:
            *val = LSM6DS3TR_C_DEN_ACT_HIGH;
            break;
        default:
            *val = LSM6DS3TR_C_DEN_POL_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  DEN functionality marking mode[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_mode in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_den_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_mode_t val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl6_c->den_mode = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  DEN functionality marking mode[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_mode in reg CTRL6_C
  */
PT_THREAD(lsm6ds3tr_c_den_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_mode_t *val))
{
    lsm6ds3tr_c_ctrl6_c_t *ctrl6_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL6_C, (uint8_t *)ctrl6_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (ctrl6_c->den_mode) {
        case LSM6DS3TR_C_DEN_DISABLE:
            *val = LSM6DS3TR_C_DEN_DISABLE;
            break;
        case LSM6DS3TR_C_LEVEL_LETCHED:
            *val = LSM6DS3TR_C_LEVEL_LETCHED;
            break;
        case LSM6DS3TR_C_LEVEL_TRIGGER:
            *val = LSM6DS3TR_C_LEVEL_TRIGGER;
            break;
        case LSM6DS3TR_C_EDGE_TRIGGER:
            *val = LSM6DS3TR_C_EDGE_TRIGGER;
            break;
        default:
            *val = LSM6DS3TR_C_DEN_MODE_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Extend DEN functionality to accelerometer sensor.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_xl_g in reg CTRL9_XL
  *                             and den_xl_en in CTRL4_C.
  */
PT_THREAD(lsm6ds3tr_c_den_enable_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_xl_en_t val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl9_xl->den_xl_g = (uint8_t)val & 0x01U;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl4_c->den_xl_en = (uint8_t)val & 0x02U;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Extend DEN functionality to accelerometer sensor. [get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of den_xl_g in reg CTRL9_XL
  *                             and den_xl_en in CTRL4_C.
  */
PT_THREAD(lsm6ds3tr_c_den_enable_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_den_xl_en_t *val))
{
    lsm6ds3tr_c_ctrl4_c_t *ctrl4_c = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL4_C, (uint8_t *)ctrl4_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch ((ctrl4_c->den_xl_en << 1) + ctrl9_xl->den_xl_g) {
        case LSM6DS3TR_C_STAMP_IN_GY_DATA:
            *val = LSM6DS3TR_C_STAMP_IN_GY_DATA;
            break;
        case LSM6DS3TR_C_STAMP_IN_XL_DATA:
            *val = LSM6DS3TR_C_STAMP_IN_XL_DATA;
            break;
        case LSM6DS3TR_C_STAMP_IN_GY_XL_DATA:
            *val = LSM6DS3TR_C_STAMP_IN_GY_XL_DATA;
            break;
        default:
            *val = LSM6DS3TR_C_DEN_STAMP_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_z in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_z_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl9_xl->den_z = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_z in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_z_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl9_xl->den_z;

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_y in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_y_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl9_xl->den_y = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_y in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_y_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl9_xl->den_y;

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of X-axis.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_x in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_x_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl9_xl->den_x = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  DEN value stored in LSB of X-axis.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of den_x in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_den_mark_axis_x_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl9_xl->den_x;

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Pedometer
  * @brief       This section groups all the functions that manage pedometer.
  * @{
  *
  */

/**
  * @brief  Reset pedometer step counter.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pedo_rst_step in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_pedo_step_reset_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->pedo_rst_step = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Reset pedometer step counter.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pedo_rst_step in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_pedo_step_reset_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->pedo_rst_step;

    PT_END(pt);
}

/**
  * @brief  Enable pedometer algorithm.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pedo_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_pedo_sens_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->pedo_en = val;
    if (val != 0x00U) {
        ctrl10_c->func_en = val;
    }
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);


    PT_END(pt);
}

/**
  * @brief  pedo_sens:   Enable pedometer algorithm.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pedo_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_pedo_sens_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->pedo_en;

    PT_END(pt);
}

/**
  * @brief  Minimum threshold to detect a peak. Default is 10h.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ths_min in reg
  *                      CONFIG_PEDO_THS_MIN
  */
PT_THREAD(lsm6ds3tr_c_pedo_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_config_pedo_ths_min_t *config_pedo_ths_min = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    config_pedo_ths_min->ths_min = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Minimum threshold to detect a peak. Default is 10h.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ths_min in reg  CONFIG_PEDO_THS_MIN
  */
PT_THREAD(lsm6ds3tr_c_pedo_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_config_pedo_ths_min_t *config_pedo_ths_min = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = config_pedo_ths_min->ths_min;
    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  pedo_full_scale:   Pedometer data range.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pedo_fs in
  *                            reg CONFIG_PEDO_THS_MIN
  */
PT_THREAD(lsm6ds3tr_c_pedo_full_scale_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pedo_fs_t val))
{
    lsm6ds3tr_c_config_pedo_ths_min_t *config_pedo_ths_min = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    config_pedo_ths_min->pedo_fs = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Pedometer data range.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of pedo_fs in
  *                            reg CONFIG_PEDO_THS_MIN
  */
PT_THREAD(lsm6ds3tr_c_pedo_full_scale_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pedo_fs_t *val))
{
    lsm6ds3tr_c_config_pedo_ths_min_t *config_pedo_ths_min = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CONFIG_PEDO_THS_MIN, (uint8_t *)config_pedo_ths_min, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (config_pedo_ths_min->pedo_fs) {
        case LSM6DS3TR_C_PEDO_AT_2g:
            *val = LSM6DS3TR_C_PEDO_AT_2g;
            break;
        case LSM6DS3TR_C_PEDO_AT_4g:
            *val = LSM6DS3TR_C_PEDO_AT_4g;
            break;
        default:
            *val = LSM6DS3TR_C_PEDO_FS_ND;
            break;
    }
    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Pedometer debounce configuration register (r/w).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of deb_step in reg PEDO_DEB_REG
  */
PT_THREAD(lsm6ds3tr_c_pedo_debounce_steps_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_pedo_deb_reg_t *pedo_deb_reg = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    pedo_deb_reg->deb_step = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Pedometer debounce configuration register (r/w).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of deb_step in reg PEDO_DEB_REG
  */
PT_THREAD(lsm6ds3tr_c_pedo_debounce_steps_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_pedo_deb_reg_t *pedo_deb_reg = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = pedo_deb_reg->deb_step;
    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Debounce time. If the time between two consecutive steps is
  *         greater than  DEB_TIME*80ms, the debouncer is reactivated.
  *         Default value: 01101[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of deb_time in reg PEDO_DEB_REG
  */
PT_THREAD(lsm6ds3tr_c_pedo_timeout_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_pedo_deb_reg_t *pedo_deb_reg = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    pedo_deb_reg->deb_time = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));
    PT_END(pt);
}

/**
  * @brief  Debounce time. If the time between two consecutive steps is
  *         greater than  DEB_TIME*80ms, the debouncer is reactivated.
  *         Default value: 01101[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of deb_time in reg PEDO_DEB_REG
  */
PT_THREAD(lsm6ds3tr_c_pedo_timeout_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_pedo_deb_reg_t *pedo_deb_reg = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_PEDO_DEB_REG, (uint8_t *)pedo_deb_reg, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = pedo_deb_reg->deb_time;
    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Time period register for step detection on delta time (r/w).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_pedo_steps_period_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_STEP_COUNT_DELTA, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Time period register for step detection on delta time (r/w).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_pedo_steps_period_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_STEP_COUNT_DELTA, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_significant_motion
  * @brief       This section groups all the functions that manage the
  *              significant motion detection.
  * @{
  *
  */

/**
  * @brief  Enable significant motion detection function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sign_motion_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_motion_sens_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->sign_motion_en = val;
    if (val != 0x00U) {
        ctrl10_c->func_en = val;
        lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
        PT_WAIT_UNTIL(pt, ctx->read_write_done);
    }

    PT_END(pt);
}

/**
  * @brief  Enable significant motion detection function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sign_motion_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_motion_sens_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->sign_motion_en;

    PT_END(pt);
}

/**
  * @brief  Significant motion threshold.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that store significant motion threshold.
  */
PT_THREAD(lsm6ds3tr_c_motion_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SM_THS, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Significant motion threshold.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that store significant motion threshold.
  */
PT_THREAD(lsm6ds3tr_c_motion_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SM_THS, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_tilt_detection
  * @brief       This section groups all the functions that manage the tilt
  *              event detection.
  * @{
  *
  */

/**
  * @brief  Enable tilt calculation.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tilt_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_tilt_sens_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->tilt_en = val;
    if (val != 0x00U) {
        ctrl10_c->func_en = val;
    }
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable tilt calculation.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tilt_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_tilt_sens_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->tilt_en;

    PT_END(pt);
}

/**
  * @brief  Enable tilt calculation.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tilt_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_wrist_tilt_sens_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->wrist_tilt_en = val;
    if (val != 0x00U) {
        ctrl10_c->func_en = val;
    }

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable tilt calculation.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tilt_en in reg CTRL10_C
  */
PT_THREAD(lsm6ds3tr_c_wrist_tilt_sens_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl10_c->wrist_tilt_en;

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt latency register (r/w).
  *         Absolute wrist tilt latency parameters.
  *         1 LSB = 40 ms. Default value: 0Fh (600 ms).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_tilt_latency_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_LAT, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt latency register (r/w).
  *         Absolute wrist tilt latency parameters.
  *         1 LSB = 40 ms. Default value: 0Fh (600 ms).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_tilt_latency_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_LAT, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt threshold register(r/w).
  *         Absolute wrist tilt threshold parameters.
  *         1 LSB = 15.625 mg.Default value: 20h (500 mg).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_tilt_threshold_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_THS, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt threshold register(r/w).
  *         Absolute wrist tilt threshold parameters.
  *         1 LSB = 15.625 mg.Default value: 20h (500 mg).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_tilt_threshold_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_THS, buff, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt mask register (r/w).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Registers A_WRIST_TILT_MASK
  */
PT_THREAD(lsm6ds3tr_c_tilt_src_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_a_wrist_tilt_mask_t *val))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_MASK, (uint8_t *)val, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Absolute Wrist Tilt mask register (r/w).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Registers A_WRIST_TILT_MASK
  */
PT_THREAD(lsm6ds3tr_c_tilt_src_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_a_wrist_tilt_mask_t *val))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_B));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_A_WRIST_TILT_MASK, (uint8_t *)val, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);

}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_ magnetometer_sensor
  * @brief       This section groups all the functions that manage additional
  *              magnetometer sensor.
  * @{
  *
  */

/**
  * @brief  Enable soft-iron correction algorithm for magnetometer.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of soft_en in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_mag_soft_iron_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl9_xl->soft_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable soft-iron correction algorithm for magnetometer.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of soft_en in reg CTRL9_XL
  */
PT_THREAD(lsm6ds3tr_c_mag_soft_iron_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_ctrl9_xl_t *ctrl9_xl = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL9_XL, (uint8_t *)ctrl9_xl, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = ctrl9_xl->soft_en;

    PT_END(pt);
}

/**
  * @brief  Enable hard-iron correction algorithm for magnetometer.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of iron_en in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_mag_hard_iron_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[1];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->iron_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    if (val != 0x00U) {
        ctrl10_c->func_en = val;
    }
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Enable hard-iron correction algorithm for magnetometer.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of iron_en in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_mag_hard_iron_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = master_config->iron_en;

    PT_END(pt);
}

/**
  * @brief  Soft iron 3x3 matrix. Value are expressed in sign-module format.
  *         (Es. SVVVVVVVb where S is the sign 0/+1/- and V is the value).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_mag_soft_iron_mat_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MAG_SI_XX, buff, 9);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Soft iron 3x3 matrix. Value are expressed in sign-module format.
  *         (Es. SVVVVVVVb where S is the sign 0/+1/- and V is the value).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_mag_soft_iron_mat_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MAG_SI_XX, buff, 9);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Offset for hard-iron compensation register (r/w). The value is
  *         expressed as a 16-bit word in two’s complement.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  */
PT_THREAD(lsm6ds3tr_c_mag_offset_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MAG_OFFX_L, buff, 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Offset for hard-iron compensation register(r/w).
  *         The value is expressed as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  */
PT_THREAD(lsm6ds3tr_c_mag_offset_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *buff))
{
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MAG_OFFX_L, buff, 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3TR_C_Sensor_hub
  * @brief       This section groups all the functions that manage the sensor
  *              hub functionality.
  * @{
  *
  */

/**
  * @brief  Enable function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values func_en
  */
PT_THREAD(lsm6ds3tr_c_func_en_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_ctrl10_c_t *ctrl10_c = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    ctrl10_c->func_en = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_CTRL10_C, (uint8_t *)ctrl10_c, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Sensor synchronization time frame with the step of 500 ms and
  *         full range of 5s. Unsigned 8-bit.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tph in reg SENSOR_SYNC_TIME_FRAME
  */
PT_THREAD(lsm6ds3tr_c_sh_sync_sens_frame_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_sensor_sync_time_frame_t *sensor_sync_time_frame = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_TIME_FRAME, (uint8_t *)sensor_sync_time_frame, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    sensor_sync_time_frame->tph = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_TIME_FRAME, (uint8_t *)sensor_sync_time_frame, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Sensor synchronization time frame with the step of 500 ms and
  *         full range of 5s. Unsigned 8-bit.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of tph in reg  SENSOR_SYNC_TIME_FRAME
  */
PT_THREAD(lsm6ds3tr_c_sh_sync_sens_frame_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_sensor_sync_time_frame_t *sensor_sync_time_frame = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_TIME_FRAME, (uint8_t *)sensor_sync_time_frame, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = sensor_sync_time_frame->tph;

    PT_END(pt);
}

/**
  * @brief  Resolution ratio of error code for sensor synchronization.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of rr in reg  SENSOR_SYNC_RES_RATIO
  */
PT_THREAD(lsm6ds3tr_c_sh_sync_sens_ratio_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_rr_t val))
{
    lsm6ds3tr_c_sensor_sync_res_ratio_t *sensor_sync_res_ratio = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_RES_RATIO, (uint8_t *)sensor_sync_res_ratio, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    sensor_sync_res_ratio->rr = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_RES_RATIO, (uint8_t *)sensor_sync_res_ratio, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Resolution ratio of error code for sensor synchronization.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of rr in reg  SENSOR_SYNC_RES_RATIO
  */
PT_THREAD(lsm6ds3tr_c_sh_sync_sens_ratio_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_rr_t *val))
{
    lsm6ds3tr_c_sensor_sync_res_ratio_t *sensor_sync_res_ratio = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSOR_SYNC_RES_RATIO, (uint8_t *)sensor_sync_res_ratio, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);


    switch (sensor_sync_res_ratio->rr) {
        case LSM6DS3TR_C_RES_RATIO_2_11:
            *val = LSM6DS3TR_C_RES_RATIO_2_11;
            break;
        case LSM6DS3TR_C_RES_RATIO_2_12:
            *val = LSM6DS3TR_C_RES_RATIO_2_12;
            break;
        case LSM6DS3TR_C_RES_RATIO_2_13:
            *val = LSM6DS3TR_C_RES_RATIO_2_13;
            break;
        case LSM6DS3TR_C_RES_RATIO_2_14:
            *val = LSM6DS3TR_C_RES_RATIO_2_14;
            break;
        default:
            *val = LSM6DS3TR_C_RES_RATIO_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Sensor hub I2C master enable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of master_on in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_master_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->master_on = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Sensor hub I2C master enable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of master_on in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_master_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = master_config->master_on;

    PT_END(pt);
}

/**
  * @brief  I2C interface pass-through.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pass_through_mode in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_pass_through_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->pass_through_mode = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  I2C interface pass-through.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pass_through_mode in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_pass_through_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = master_config->pass_through_mode;

    PT_END(pt);
}

/**
  * @brief  Master I2C pull-up enable/disable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pull_up_en in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_pin_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pull_up_en_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->pull_up_en = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Master I2C pull-up enable/disable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of pull_up_en in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_pin_mode_get(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_pull_up_en_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (master_config->pull_up_en) {
        case LSM6DS3TR_C_EXT_PULL_UP:
            *val = LSM6DS3TR_C_EXT_PULL_UP;
            break;
        case LSM6DS3TR_C_INTERNAL_PULL_UP:
            *val = LSM6DS3TR_C_INTERNAL_PULL_UP;
            break;
        default:
            *val = LSM6DS3TR_C_SH_PIN_MODE;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Sensor hub trigger signal selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of start_config in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_syncro_mode_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_start_config_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->start_config = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Sensor hub trigger signal selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of start_config in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_syncro_mode_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_start_config_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (master_config->start_config) {
        case LSM6DS3TR_C_XL_GY_DRDY:
            *val = LSM6DS3TR_C_XL_GY_DRDY;
            break;
        case LSM6DS3TR_C_EXT_ON_INT2_PIN:
            *val = LSM6DS3TR_C_EXT_ON_INT2_PIN;
            break;
        default:
            *val = LSM6DS3TR_C_SH_SYNCRO_ND;
            break;
    }

    PT_END(pt);
}

/**
  * @brief  Manage the Master DRDY signal on INT1 pad.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy_on_int1 in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_drdy_on_int1_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_config->drdy_on_int1 = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Manage the Master DRDY signal on INT1 pad.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy_on_int1 in reg MASTER_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_drdy_on_int1_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_master_config_t *master_config = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CONFIG, (uint8_t *)master_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = master_config->drdy_on_int1;

    PT_END(pt);
}

/**
  * @brief  Sensor hub output registers.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure of registers from SENSORHUB1_REG
  */
PT_THREAD(lsm6ds3tr_c_sh_read_data_raw_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_emb_sh_read_t *val))
{
    PT_BEGIN(pt);
    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSORHUB1_REG, (uint8_t *)&(val->sh_byte_1), 12);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENSORHUB13_REG, (uint8_t *)&(val->sh_byte_13), 6);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Master command code used for stamping for sensor sync.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of master_cmd_code in
  *                reg MASTER_CMD_CODE
  */
PT_THREAD(lsm6ds3tr_c_sh_cmd_sens_sync_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_master_cmd_code_t *master_cmd_code = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CMD_CODE, (uint8_t *)master_cmd_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    master_cmd_code->master_cmd_code = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_MASTER_CMD_CODE, (uint8_t *)master_cmd_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Master command code used for stamping for sensor sync.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of master_cmd_code in
  *                reg MASTER_CMD_CODE
  */
PT_THREAD(lsm6ds3tr_c_sh_cmd_sens_sync_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_master_cmd_code_t *master_cmd_code = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_MASTER_CMD_CODE, (uint8_t *)master_cmd_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = master_cmd_code->master_cmd_code;

    PT_END(pt);
}

/**
  * @brief  Error code used for sensor synchronization.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of error_code in
  *                reg sens_sync_spi_error_code->
  */
PT_THREAD(lsm6ds3tr_c_sh_spi_sync_error_set(struct pt *pt, stmdev_ctx_t *ctx, uint8_t val))
{
    lsm6ds3tr_c_sens_sync_spi_error_code_t *sens_sync_spi_error_code = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENS_SYNC_SPI_ERROR_CODE, (uint8_t *)sens_sync_spi_error_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    sens_sync_spi_error_code->error_code = val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SENS_SYNC_SPI_ERROR_CODE, (uint8_t *)sens_sync_spi_error_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_END(pt);
}

/**
  * @brief  Error code used for sensor synchronization.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of error_code in
  *                reg sens_sync_spi_error_code->
  */
PT_THREAD(lsm6ds3tr_c_sh_spi_sync_error_get(struct pt *pt, stmdev_ctx_t *ctx, uint8_t *val))
{
    lsm6ds3tr_c_sens_sync_spi_error_code_t *sens_sync_spi_error_code = (void *)&lsm6ds3tr_data_buffer[0];

    PT_BEGIN(pt);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SENS_SYNC_SPI_ERROR_CODE, (uint8_t *)sens_sync_spi_error_code, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    *val = sens_sync_spi_error_code->error_code;

    PT_END(pt);
}

/**
  * @brief   Number of external sensors to be read by the sensor hub.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of aux_sens_on in reg slave0_config->
  */
PT_THREAD(lsm6ds3tr_c_sh_num_of_dev_connected_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_aux_sens_on_t val))
{
    lsm6ds3tr_c_slave0_config_t *slave0_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave0_config->aux_sens_on = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief   Number of external sensors to be read by the sensor hub.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of aux_sens_on in reg slave0_config->
  */
PT_THREAD(lsm6ds3tr_c_sh_num_of_dev_connected_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_aux_sens_on_t *val))
{
    lsm6ds3tr_c_slave0_config_t *slave0_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave0_config->aux_sens_on) {
        case LSM6DS3TR_C_SLV_0:
            *val = LSM6DS3TR_C_SLV_0;
            break;
        case LSM6DS3TR_C_SLV_0_1:
            *val = LSM6DS3TR_C_SLV_0_1;
            break;
        case LSM6DS3TR_C_SLV_0_1_2:
            *val = LSM6DS3TR_C_SLV_0_1_2;
            break;
        case LSM6DS3TR_C_SLV_0_1_2_3:
            *val = LSM6DS3TR_C_SLV_0_1_2_3;
            break;
        default:
            *val = LSM6DS3TR_C_SLV_EN_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Configure slave 0 for perform a write.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure that contain:
  *                  - uint8_t slv_add;    8 bit i2c device address
  *                  - uint8_t slv_subadd; 8 bit register device address
  *                  - uint8_t slv_data;   8 bit data to write
  */
PT_THREAD(lsm6ds3tr_c_sh_cfg_write(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_sh_cfg_write_t *val))
{
    lsm6ds3tr_c_slv0_add_t *slv0_add = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    slv0_add->slave0_add = val->slv0_add;
    slv0_add->rw_0 = 0;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV0_ADD, (uint8_t *)slv0_add, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV0_SUBADD, &(val->slv0_subadd), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_DATAWRITE_SRC_MODE_SUB_SLV0, &(val->slv0_data), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Configure slave 0 for perform a read.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure that contain:
  *                  - uint8_t slv_add;    8 bit i2c device address
  *                  - uint8_t slv_subadd; 8 bit register device address
  *                  - uint8_t slv_len;    num of bit to read
  */
PT_THREAD(lsm6ds3tr_c_sh_slv0_cfg_read(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_sh_cfg_read_t *val))
{
    lsm6ds3tr_c_slave0_config_t *slave0_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_slv0_add_t *slv0_add = (void *)&lsm6ds3tr_data_buffer[1];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    slv0_add->slave0_add = val->slv_add;
    slv0_add->rw_0 = 1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV0_ADD, (uint8_t *)slv0_add, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV0_SUBADD, &(val->slv_subadd), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave0_config->slave0_numop = val->slv_len;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Configure slave 1 for perform a read.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure that contain:
  *                  - uint8_t slv_add;    8 bit i2c device address
  *                  - uint8_t slv_subadd; 8 bit register device address
  *                  - uint8_t slv_len;    num of bit to read
  */
PT_THREAD(lsm6ds3tr_c_sh_slv1_cfg_read(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_sh_cfg_read_t *val))
{
    lsm6ds3tr_c_slave1_config_t *slave1_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_slv1_add_t *slv1_add = (void *)&lsm6ds3tr_data_buffer[1];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    slv1_add->slave1_add = val->slv_add;
    slv1_add->r_1 = 1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV1_ADD, (uint8_t *)slv1_add, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV1_SUBADD, &(val->slv_subadd), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave1_config->slave1_numop = val->slv_len;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Configure slave 2 for perform a read.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure that contain:
  *                  - uint8_t slv_add;    8 bit i2c device address
  *                  - uint8_t slv_subadd; 8 bit register device address
  *                  - uint8_t slv_len;    num of bit to read
  */
PT_THREAD(lsm6ds3tr_c_sh_slv2_cfg_read(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_sh_cfg_read_t *val))
{
    lsm6ds3tr_c_slv2_add_t *slv2_add = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_slave2_config_t *slave2_config = (void *)&lsm6ds3tr_data_buffer[1];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    slv2_add->slave2_add = val->slv_add;
    slv2_add->r_2 = 1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV2_ADD, (uint8_t *)slv2_add, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV2_SUBADD, &(val->slv_subadd), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE2_CONFIG, (uint8_t *)slave2_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave2_config->slave2_numop = val->slv_len;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE2_CONFIG, (uint8_t *)slave2_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Configure slave 3 for perform a read.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure that contain:
  *                  - uint8_t slv_add;    8 bit i2c device address
  *                  - uint8_t slv_subadd; 8 bit register device address
  *                  - uint8_t slv_len;    num of bit to read
  */
PT_THREAD(lsm6ds3tr_c_sh_slv3_cfg_read(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_sh_cfg_read_t *val))
{
    lsm6ds3tr_c_slave3_config_t *slave3_config = (void *)&lsm6ds3tr_data_buffer[0];
    lsm6ds3tr_c_slv3_add_t *slv3_add = (void *)&lsm6ds3tr_data_buffer[1];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    slv3_add->slave3_add = val->slv_add;
    slv3_add->r_3 = 1;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV3_ADD, (uint8_t *)slv3_add, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLV3_SUBADD, (uint8_t *)&(val->slv_subadd), 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE3_CONFIG, (uint8_t *)slave3_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave3_config->slave3_numop = val->slv_len;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE3_CONFIG, (uint8_t *)slave3_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 0 starting from the
  *         sensor hub trigger.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of slave0_rate in reg SLAVE0_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_0_dec_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave0_rate_t val))
{
    lsm6ds3tr_c_slave0_config_t *slave0_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave0_config->slave0_rate = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 0 starting from the
  *         sensor hub trigger.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of slave0_rate in reg SLAVE0_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_0_dec_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave0_rate_t *val))
{
    lsm6ds3tr_c_slave0_config_t *slave0_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE0_CONFIG, (uint8_t *)slave0_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave0_config->slave0_rate) {
        case LSM6DS3TR_C_SL0_NO_DEC:
            *val = LSM6DS3TR_C_SL0_NO_DEC;
            break;
        case LSM6DS3TR_C_SL0_DEC_2:
            *val = LSM6DS3TR_C_SL0_DEC_2;
            break;
        case LSM6DS3TR_C_SL0_DEC_4:
            *val = LSM6DS3TR_C_SL0_DEC_4;
            break;
        case LSM6DS3TR_C_SL0_DEC_8:
            *val = LSM6DS3TR_C_SL0_DEC_8;
            break;
        default:
            *val = LSM6DS3TR_C_SL0_DEC_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Slave 0 write operation is performed only at the first sensor
  *         hub cycle.
  *         This is effective if the Aux_sens_on[1:0] field in
  *         SLAVE0_CONFIG(04h) is set to a value other than 00.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of write_once in reg SLAVE1_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_write_mode_set(struct pt *pt, stmdev_ctx_t *ctx, lsm6ds3tr_c_write_once_t val))
{
    lsm6ds3tr_c_slave1_config_t *slave1_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave1_config->write_once = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Slave 0 write operation is performed only at the first sensor
  *         hub cycle.
  *         This is effective if the Aux_sens_on[1:0] field in
  *         SLAVE0_CONFIG(04h) is set to a value other than 00.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of write_once in reg SLAVE1_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_write_mode_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_write_once_t *val))
{
    lsm6ds3tr_c_slave1_config_t *slave1_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave1_config->write_once) {
        case LSM6DS3TR_C_EACH_SH_CYCLE:
            *val = LSM6DS3TR_C_EACH_SH_CYCLE;
            break;
        case LSM6DS3TR_C_ONLY_FIRST_CYCLE:
            *val = LSM6DS3TR_C_ONLY_FIRST_CYCLE;
            break;
        default:
            *val = LSM6DS3TR_C_SH_WR_MODE_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 1 starting from the
  *         sensor hub trigger.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of slave1_rate in reg SLAVE1_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_1_dec_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave1_rate_t val))
{
    lsm6ds3tr_c_slave1_config_t *slave1_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave1_config->slave1_rate = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 1 starting from the
  *         sensor hub trigger.[get]
  *
  * @param  ctx    Read / write interface definitions reg SLAVE1_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_1_dec_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave1_rate_t *val))
{
    lsm6ds3tr_c_slave1_config_t *slave1_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE1_CONFIG, (uint8_t *)slave1_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave1_config->slave1_rate) {
        case LSM6DS3TR_C_SL1_NO_DEC:
            *val = LSM6DS3TR_C_SL1_NO_DEC;
            break;
        case LSM6DS3TR_C_SL1_DEC_2:
            *val = LSM6DS3TR_C_SL1_DEC_2;
            break;
        case LSM6DS3TR_C_SL1_DEC_4:
            *val = LSM6DS3TR_C_SL1_DEC_4;
            break;
        case LSM6DS3TR_C_SL1_DEC_8:
            *val = LSM6DS3TR_C_SL1_DEC_8;
            break;
        default:
            *val = LSM6DS3TR_C_SL1_DEC_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 2 starting from the
  *         sensor hub trigger.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of slave2_rate in reg SLAVE2_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_2_dec_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave2_rate_t val))
{
    lsm6ds3tr_c_slave2_config_t *slave2_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE2_CONFIG, (uint8_t *)slave2_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave2_config->slave2_rate = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE2_CONFIG, (uint8_t *)slave2_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 2 starting from the
  *         sensor hub trigger.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of slave2_rate in reg SLAVE2_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_2_dec_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave2_rate_t *val))
{
    lsm6ds3tr_c_slave2_config_t *slave2_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE2_CONFIG, (uint8_t *)slave2_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave2_config->slave2_rate) {
        case LSM6DS3TR_C_SL2_NO_DEC:
            *val = LSM6DS3TR_C_SL2_NO_DEC;
            break;
        case LSM6DS3TR_C_SL2_DEC_2:
            *val = LSM6DS3TR_C_SL2_DEC_2;
            break;
        case LSM6DS3TR_C_SL2_DEC_4:
            *val = LSM6DS3TR_C_SL2_DEC_4;
            break;
        case LSM6DS3TR_C_SL2_DEC_8:
            *val = LSM6DS3TR_C_SL2_DEC_8;
            break;
        default:
            *val = LSM6DS3TR_C_SL2_DEC_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 3 starting from the
  *         sensor hub trigger.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of slave3_rate in reg SLAVE3_CONFIG
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_3_dec_set(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave3_rate_t val))
{
    lsm6ds3tr_c_slave3_config_t *slave3_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE3_CONFIG, (uint8_t *)slave3_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    slave3_config->slave3_rate = (uint8_t)val;
    lsm6ds3tr_c_write_reg(ctx, LSM6DS3TR_C_SLAVE3_CONFIG, (uint8_t *)slave3_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @brief  Decimation of read operation on Slave 3 starting from the
  *         sensor hub trigger.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of slave3_rate in reg slave3_config->
  */
PT_THREAD(lsm6ds3tr_c_sh_slave_3_dec_get(struct pt *pt, stmdev_ctx_t *ctx,
    lsm6ds3tr_c_slave3_rate_t *val))
{
    lsm6ds3tr_c_slave3_config_t *slave3_config = (void *)&lsm6ds3tr_data_buffer[0];

    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_BANK_A));

    lsm6ds3tr_c_read_reg(ctx, LSM6DS3TR_C_SLAVE3_CONFIG, (uint8_t *)slave3_config, 1);
    PT_WAIT_UNTIL(pt, ctx->read_write_done);

    switch (slave3_config->slave3_rate) {
        case LSM6DS3TR_C_SL3_NO_DEC:
            *val = LSM6DS3TR_C_SL3_NO_DEC;
            break;
        case LSM6DS3TR_C_SL3_DEC_2:
            *val = LSM6DS3TR_C_SL3_DEC_2;
            break;
        case LSM6DS3TR_C_SL3_DEC_4:
            *val = LSM6DS3TR_C_SL3_DEC_4;
            break;
        case LSM6DS3TR_C_SL3_DEC_8:
            *val = LSM6DS3TR_C_SL3_DEC_8;
            break;
        default:
            *val = LSM6DS3TR_C_SL3_DEC_ND;
            break;
    }

    PT_SPAWN(pt, &child, lsm6ds3tr_c_mem_bank_set(&child, ctx, LSM6DS3TR_C_USER_BANK));

    PT_END(pt);
}

/**
  * @}
  *
  */

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
