// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pbsys/sys.h>

#include <pybricks/experimental.h>
#include <pybricks/robotics.h>

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
            MICROPY_EVENT_POLL_HOOK;
        }
    } while (SPI1->SR & SPI_SR_BSY);

    while (SPI1->SR & SPI_SR_RXNE) {
        dummy = BYTE_ACCESS(SPI1->DR);
        (void)dummy;
        MICROPY_EVENT_POLL_HOOK;
    }
    BYTE_ACCESS(SPI1->DR) = 0;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
            MICROPY_EVENT_POLL_HOOK
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
            MICROPY_EVENT_POLL_HOOK;
        }
    } while (SPI1->SR & SPI_SR_BSY);

    BYTE_ACCESS(SPI1->DR) = value;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
            MICROPY_EVENT_POLL_HOOK;
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

#if PYBRICKS_HUB_EV3BRICK
#if !MICROPY_MODULE_BUILTIN_INIT
#error "pybricks.experimental module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

#include <signal.h>

#include "py/mpthread.h"

STATIC void sighandler(int signum) {
    // we just want the signal to interrupt system calls
}

STATIC mp_obj_t mod_experimental___init__(void) {
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
#endif // PYBRICKS_HUB_EV3BRICK

STATIC mp_obj_t experimental_getchar(void) {
    uint8_t c;
    pbio_error_t err = pbsys_stdin_get_char(&c);
    if (err == PBIO_SUCCESS) {
        return MP_OBJ_NEW_SMALL_INT(c);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(experimental_getchar_obj, experimental_getchar);

STATIC const mp_rom_map_elem_t experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental_c) },
    { MP_ROM_QSTR(MP_QSTR_getchar),  MP_ROM_PTR(&experimental_getchar_obj)},
    #if PYBRICKS_HUB_MOVEHUB
    { MP_ROM_QSTR(MP_QSTR_Motion), MP_ROM_PTR(&mod_experimental_Motion_type) },
    #endif // PYBRICKS_HUB_MOVEHUB
    #if PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
    #endif // PYBRICKS_HUB_EV3BRICK
};
STATIC MP_DEFINE_CONST_DICT(pb_module_experimental_globals, experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_experimental_globals,
};

#endif // PYBRICKS_PY_EXPERIMENTAL
