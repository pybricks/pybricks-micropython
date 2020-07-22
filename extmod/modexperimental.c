// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// C implementation of Pybricks experimental module.
// Also see pybricks/experimental.py.

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/obj.h"
#include "py/runtime.h"

#if PYBRICKS_HUB_CPLUSHUB

#include <lsm6ds3tr_c_reg.h>
#include <stm32l4xx_hal.h>
#include <stm32l4xx_ll_i2c.h>

typedef struct {
    mp_obj_base_t base;
    stmdev_ctx_t ctx;
} mod_experimental_IMU_obj_t;

STATIC I2C_HandleTypeDef hi2c;

void mod_experimental_IMU_handle_i2c_er_irq() {
    HAL_I2C_ER_IRQHandler(&hi2c);
}

void mod_experimental_IMU_handle_i2c_ev_irq() {
    HAL_I2C_EV_IRQHandler(&hi2c);
}

STATIC int32_t mod_experimental_IMU_write_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Mem_Write(&hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len, 500);

    return ret;
}

STATIC int32_t mod_experimental_IMU_read_reg(void *handle, uint8_t reg, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Mem_Read(&hi2c, LSM6DS3TR_C_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, data, len, 500);

    return ret;
}

STATIC mp_obj_t mod_experimental_IMU_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mod_experimental_IMU_obj_t *self = m_new_obj(mod_experimental_IMU_obj_t);

    self->base.type = (mp_obj_type_t *)otype;
    self->ctx.write_reg = mod_experimental_IMU_write_reg;
    self->ctx.read_reg = mod_experimental_IMU_read_reg;

    if (hi2c.Instance == NULL) {
        hi2c.Instance = I2C1;
        // Clock is 5MHz, so these timing come out to 1 usec. When combined with
        // internal delays, this is slightly slower than 400kHz
        hi2c.Init.Timing = __LL_I2C_CONVERT_TIMINGS(0, 0, 0, 4, 4);
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

    uint8_t id;
    if (lsm6ds3tr_c_device_id_get(&self->ctx, &id) != 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to get device id"));
    }

    if (id != LSM6DS3TR_C_ID) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("incorrect device id"));
    }

    // Init based on data polling example

    /*
     *  Restore default configuration
     */
    uint8_t rst;
    lsm6ds3tr_c_reset_set(&self->ctx, PROPERTY_ENABLE);
    do {
        lsm6ds3tr_c_reset_get(&self->ctx, &rst);
    } while (rst);
    /*
     *  Enable Block Data Update
     */
    lsm6ds3tr_c_block_data_update_set(&self->ctx, PROPERTY_ENABLE);
    /*
     * Set Output Data Rate
     */
    lsm6ds3tr_c_xl_data_rate_set(&self->ctx, LSM6DS3TR_C_XL_ODR_12Hz5);
    lsm6ds3tr_c_gy_data_rate_set(&self->ctx, LSM6DS3TR_C_GY_ODR_833Hz);
    /*
     * Set full scale
     */
    lsm6ds3tr_c_xl_full_scale_set(&self->ctx, LSM6DS3TR_C_2g);
    lsm6ds3tr_c_gy_full_scale_set(&self->ctx, LSM6DS3TR_C_250dps);

    /*
     * Configure filtering chain(No aux interface)
     */
    /* Accelerometer - analog filter */
    lsm6ds3tr_c_xl_filter_analog_set(&self->ctx, LSM6DS3TR_C_XL_ANA_BW_400Hz);

    /* Accelerometer - LPF1 path ( LPF2 not used )*/
    // lsm6ds3tr_c_xl_lp1_bandwidth_set(&self->ctx, LSM6DS3TR_C_XL_LP1_ODR_DIV_4);

    /* Accelerometer - LPF1 + LPF2 path */
    lsm6ds3tr_c_xl_lp2_bandwidth_set(&self->ctx, LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);

    /* Accelerometer - High Pass / Slope path */
    // lsm6ds3tr_c_xl_reference_mode_set(&self->ctx, PROPERTY_DISABLE);
    // lsm6ds3tr_c_xl_hp_bandwidth_set(&self->ctx, LSM6DS3TR_C_XL_HP_ODR_DIV_100);

    /* Gyroscope - filtering chain */
    lsm6ds3tr_c_gy_band_pass_set(&self->ctx, LSM6DS3TR_C_HP_16mHz_LP1_LIGHT);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t mod_experimental_IMU_accel(mp_obj_t self_in) {
    mod_experimental_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t data[3];

    if (lsm6ds3tr_c_acceleration_raw_get(&self->ctx, (uint8_t *)data) != 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to read data"));
    }

    mp_obj_t values[3];
    values[0] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[0]) / 1000.0f);
    values[1] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[1]) / 1000.0f);
    values[2] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[2]) / 1000.0f);

    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_experimental_IMU_accel_obj, mod_experimental_IMU_accel);

STATIC mp_obj_t mod_experimental_IMU_gyro(mp_obj_t self_in) {
    mod_experimental_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t data[3];

    if (lsm6ds3tr_c_angular_rate_raw_get(&self->ctx, (uint8_t *)data) != 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to read data"));
    }

    mp_obj_t values[3];
    values[0] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[0]) / 1000.0f);
    values[1] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[1]) / 1000.0f);
    values[2] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[2]) / 1000.0f);

    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_experimental_IMU_gyro_obj, mod_experimental_IMU_gyro);

STATIC const mp_rom_map_elem_t mod_experimental_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_accel), MP_ROM_PTR(&mod_experimental_IMU_accel_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro), MP_ROM_PTR(&mod_experimental_IMU_gyro_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mod_experimental_IMU_locals_dict, mod_experimental_IMU_locals_dict_table);

STATIC const mp_obj_type_t mod_experimental_IMU_type = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .make_new = mod_experimental_IMU_make_new,
    .locals_dict = (mp_obj_dict_t *)&mod_experimental_IMU_locals_dict,
};

#endif // PYBRICKS_HUB_CPLUSHUB

#if PYBRICKS_HUB_MOVEHUB

// Move Hub has internal STMicroelectronics LIS3DH motion sensor

#include "stm32f070xb.h"

#define LIS3DH_ID                   0x33

#define WHO_AM_I                    0x0f

#define CTRL_REG1                   0x20
#define CTRL_REG1_ODR_POWER_DOWN    (0 << 4)
#define CTRL_REG1_ODR_1HZ           (1 << 4)
#define CTRL_REG1_ODR_10HZ          (2 << 4)
#define CTRL_REG1_ODR_25HZ          (3 << 4)
#define CTRL_REG1_ODR_50HZ          (4 << 4)
#define CTRL_REG1_ODR_100HZ         (5 << 4)
#define CTRL_REG1_ODR_200HZ         (6 << 4)
#define CTRL_REG1_ODR_400HZ         (7 << 4)
#define CTRL_REG1_ODR_1620HZ        (8 << 4)
#define CTRL_REG1_ODR_5376HZ        (9 << 4)
#define CTRL_REG1_LPEN              (1 << 3)
#define CTRL_REG1_ZEN               (1 << 2)
#define CTRL_REG1_YEN               (1 << 1)
#define CTRL_REG1_XEN               (1 << 0)

#define OUT_X_L                     0x28
#define OUT_X_H                     0x29
#define OUT_Y_L                     0x2A
#define OUT_Y_H                     0x2B
#define OUT_Z_L                     0x2C
#define OUT_Z_H                     0x2D

#define CLICK_CFG                   0x38
#define CLICK_CFG_ZD                (1 << 5)
#define CLICK_CFG_ZS                (1 << 4)
#define CLICK_CFG_YD                (1 << 3)
#define CLICK_CFG_YS                (1 << 2)
#define CLICK_CFG_XD                (1 << 1)
#define CLICK_CFG_XS                (1 << 0)

#define CLICK_THS                   0x3a
#define CLICK_THS_LIR_CLICK         (1 << 7)
#define CLICK_THS_THS(n)            ((n) & 0x7f)

#define TIME_LIMIT                  0x3b
#define TIME_LIMIT_TLI(n)           ((n) & 0x7f)

#define TIME_LATENCY                0x3c
#define TIME_LATENCY_TLA(n)         ((n) & 0x7f)

#define READ_FLAG                   0x80

#define BYTE_ACCESS(r) (*(volatile uint8_t *)&(r))

typedef struct {
    mp_obj_base_t base;
} mod_experimental_Motion_obj_t;

static void motion_spi_read(uint8_t reg, uint8_t *value) {
    uint8_t dummy;

    // set chip select low
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = READ_FLAG | reg;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
        }
    } while (SPI1->SR & SPI_SR_BSY);

    while (SPI1->SR & SPI_SR_RXNE) {
        dummy = BYTE_ACCESS(SPI1->DR);
        (void)dummy;
    }
    BYTE_ACCESS(SPI1->DR) = 0;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
        }
    } while (SPI1->SR & SPI_SR_BSY);

    *value = BYTE_ACCESS(SPI1->DR);

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

static void motion_spi_write(uint8_t reg, uint8_t value) {
    // set chip select low
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = reg;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
        }
    } while (SPI1->SR & SPI_SR_BSY);

    BYTE_ACCESS(SPI1->DR) = value;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
        }
    } while (SPI1->SR & SPI_SR_BSY);

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

STATIC mp_obj_t mod_experimental_Motion_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mod_experimental_Motion_obj_t *self = m_new_obj(mod_experimental_Motion_obj_t);

    self->base.type = (mp_obj_type_t *)otype;

    // PA4 gpio output - used for CS
    GPIOA->BSRR = GPIO_BSRR_BS_4;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER4_Msk) | (1 << GPIO_MODER_MODER4_Pos);

    // PA5, PA5, PA7 muxed as SPI1 pins
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER5_Msk) | (2 << GPIO_MODER_MODER5_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER6_Msk) | (2 << GPIO_MODER_MODER6_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL6;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL7;

    // configure SPI1
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    #define SPI_CR1_BR_DIV8 (SPI_CR1_BR_1)
    SPI1->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_BR_DIV8;
    #define SPI_CR2_DS_8BIT (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
    SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;

    uint8_t id;
    motion_spi_read(WHO_AM_I, &id);

    if (id != LIS3DH_ID) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("incorrect device id"));
    }

    // Init

    motion_spi_write(CTRL_REG1, CTRL_REG1_XEN | CTRL_REG1_YEN | CTRL_REG1_ZEN | CTRL_REG1_ODR_5376HZ);
    motion_spi_write(CLICK_CFG, CLICK_CFG_XS | CLICK_CFG_XD | CLICK_CFG_YS | CLICK_CFG_YD | CLICK_CFG_ZS | CLICK_CFG_ZD);
    motion_spi_write(CLICK_THS, CLICK_THS_THS(127));
    motion_spi_write(TIME_LIMIT, TIME_LIMIT_TLI(127));
    motion_spi_write(TIME_LATENCY, TIME_LATENCY_TLA(127));

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t mod_experimental_Motion_accel(mp_obj_t self_in) {
    mod_experimental_Motion_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;

    uint8_t data[3];
    motion_spi_read(OUT_X_H, &data[0]);
    motion_spi_read(OUT_Y_H, &data[1]);
    motion_spi_read(OUT_Z_H, &data[2]);

    mp_obj_t values[3];
    values[0] = mp_obj_new_int((int8_t)data[0]);
    values[1] = mp_obj_new_int((int8_t)data[1]);
    values[2] = mp_obj_new_int((int8_t)data[2]);

    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_experimental_Motion_accel_obj, mod_experimental_Motion_accel);

STATIC const mp_rom_map_elem_t mod_experimental_Motion_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_accel), MP_ROM_PTR(&mod_experimental_Motion_accel_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mod_experimental_Motion_locals_dict, mod_experimental_Motion_locals_dict_table);

STATIC const mp_obj_type_t mod_experimental_Motion_type = {
    { &mp_type_type },
    .name = MP_QSTR_Motion,
    .make_new = mod_experimental_Motion_make_new,
    .locals_dict = (mp_obj_dict_t *)&mod_experimental_Motion_locals_dict,
};

#endif // PYBRICKS_HUB_MOVEHUB

#if PYBRICKS_HUB_EV3
#if !MICROPY_MODULE_BUILTIN_INIT
#error "pybricks.experimental module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

#include <signal.h>

#include "py/mpthread.h"

STATIC void sighandler() {
    // we just want the signal to interrupt system calls
}

STATIC mp_obj_t mod_experimental___init__() {
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_experimental___init___obj, mod_experimental___init__);

STATIC mp_obj_t mod_experimental_pthread_raise(mp_obj_t thread_id_in, mp_obj_t ex_in) {
    mp_uint_t thread_id = mp_obj_int_get_truncated(thread_id_in);
    if (ex_in != mp_const_none && !mp_obj_is_exception_instance(ex_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("must be an exception or None"));
    }
    return mp_obj_new_int(mp_thread_schedule_exception(thread_id, ex_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_experimental_pthread_raise_obj, mod_experimental_pthread_raise);
#endif // PYBRICKS_HUB_EV3

STATIC const mp_rom_map_elem_t mod_experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental_c) },
    #if PYBRICKS_HUB_CPLUSHUB
    { MP_ROM_QSTR(MP_QSTR_IMU), MP_ROM_PTR(&mod_experimental_IMU_type) },
    #endif // PYBRICKS_HUB_CPLUSHUB
    #if PYBRICKS_HUB_MOVEHUB
    { MP_ROM_QSTR(MP_QSTR_Motion), MP_ROM_PTR(&mod_experimental_Motion_type) },
    #endif // PYBRICKS_HUB_MOVEHUB
    #if PYBRICKS_HUB_EV3
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
    #endif // PYBRICKS_HUB_EV3
};
STATIC MP_DEFINE_CONST_DICT(mod_experimental_globals, mod_experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mod_experimental_globals,
};

#endif // PYBRICKS_PY_EXPERIMENTAL
