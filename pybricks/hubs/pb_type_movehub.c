// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB

#include "py/runtime.h"

#include <pbdrv/gpio.h>
#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_obj_helper.h>

// LIS3DH motion sensor

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

// Wait for spi operation to complete
static void motion_spi_wait() {
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) {
            MICROPY_EVENT_POLL_HOOK;
        }
        MICROPY_EVENT_POLL_HOOK;
    } while (SPI1->SR & SPI_SR_BSY);
}

static void motion_spi_read(uint8_t reg, uint8_t *value) {
    // enable chip select
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = READ_FLAG | reg;

    // busy wait
    motion_spi_wait();

    while (SPI1->SR & SPI_SR_RXNE) {
        uint8_t dummy = BYTE_ACCESS(SPI1->DR);
        (void)dummy;
        MICROPY_EVENT_POLL_HOOK;
    }
    BYTE_ACCESS(SPI1->DR) = 0;

    // busy wait
    motion_spi_wait();

    *value = BYTE_ACCESS(SPI1->DR);

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

static void motion_spi_write(uint8_t reg, uint8_t value) {
    // enable chip select
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = reg;

    // busy wait
    motion_spi_wait();

    BYTE_ACCESS(SPI1->DR) = value;

    // busy wait
    motion_spi_wait();

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

static void motion_get_acceleration(int8_t *data) {
    motion_spi_read(OUT_Y_H, (uint8_t *)&data[0]);
    motion_spi_read(OUT_Z_H, (uint8_t *)&data[2]);
    motion_spi_read(OUT_X_H, (uint8_t *)&data[1]);
    data[1] = -data[1];
}

typedef struct {
    mp_obj_base_t base;
} hubs_MoveHub_IMU_obj_t;

// This is an integer version of pybricks._common.IMU.up
STATIC mp_obj_t hubs_MoveHub_IMU_up(mp_obj_t self_in) {

    int8_t values[3];
    motion_get_acceleration(values);

    // Find index and sign of maximum component
    int8_t abs_max = 0;
    uint8_t axis = 0;
    bool positive = false;
    for (uint8_t i = 0; i < 3; i++) {
        if (values[i] > abs_max) {
            abs_max = values[i];
            positive = true;
            axis = i;
        } else if (-values[i] > abs_max) {
            abs_max = -values[i];
            positive = false;
            axis = i;
        }
    }

    // Convert axis and sign to side
    if (axis == 0 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_FRONT_obj);
    }
    if (axis == 0 && !positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_BACK_obj);
    }
    if (axis == 1 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_LEFT_obj);
    }
    if (axis == 1 && !positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_RIGHT_obj);
    }
    if (axis == 2 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_TOP_obj);
    } else {
        return MP_OBJ_FROM_PTR(&pb_Side_BOTTOM_obj);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(hubs_MoveHub_IMU_up_obj, hubs_MoveHub_IMU_up);

STATIC mp_obj_t hubs_MoveHub_IMU_acceleration(mp_obj_t self_in) {

    // Gets signed acceleration data
    int8_t data[3];
    motion_get_acceleration(data);

    // Convert to appropriate units and return as tuple
    mp_obj_t values[3];
    for (uint8_t i = 0; i < 3; i++) {
        values[i] = MP_OBJ_NEW_SMALL_INT((data[i] * 158));
    }
    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hubs_MoveHub_IMU_acceleration_obj, hubs_MoveHub_IMU_acceleration);

STATIC const mp_rom_map_elem_t hubs_MoveHub_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&hubs_MoveHub_IMU_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_up),           MP_ROM_PTR(&hubs_MoveHub_IMU_up_obj)           },
};
STATIC MP_DEFINE_CONST_DICT(hubs_MoveHub_IMU_locals_dict, hubs_MoveHub_IMU_locals_dict_table);

STATIC const mp_obj_type_t hubs_MoveHub_IMU_type = {
    { &mp_type_type },
    .name = MP_QSTR_Motion,
    .attr = pb_attribute_handler,
    .locals_dict = (mp_obj_dict_t *)&hubs_MoveHub_IMU_locals_dict,
};

STATIC mp_obj_t hubs_MoveHub_IMU_make_new(void) {
    hubs_MoveHub_IMU_obj_t *self = m_new_obj(hubs_MoveHub_IMU_obj_t);

    self->base.type = &hubs_MoveHub_IMU_type;

    // PA4 gpio output - used for CS
    pbdrv_gpio_t gpio = { .bank = GPIOA, .pin = 4 };
    pbdrv_gpio_out_high(&gpio);

    // PA5, PA6, PA7 muxed as SPI1 pins
    for (gpio.pin = 5; gpio.pin <= 7; gpio.pin++) {
        pbdrv_gpio_alt(&gpio, 0);
    }

    // configure SPI1
    #define SPI_CR1_BR_DIV8 (SPI_CR1_BR_1)
    SPI1->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_BR_DIV8;
    #define SPI_CR2_DS_8BIT (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
    SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;

    uint8_t id;
    motion_spi_read(WHO_AM_I, &id);

    if (id != LIS3DH_ID) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("bad id"));
    }

    // Init

    motion_spi_write(CTRL_REG1, CTRL_REG1_XEN | CTRL_REG1_YEN | CTRL_REG1_ZEN | CTRL_REG1_ODR_5376HZ);
    motion_spi_write(CLICK_CFG, CLICK_CFG_XS | CLICK_CFG_XD | CLICK_CFG_YS | CLICK_CFG_YD | CLICK_CFG_ZS | CLICK_CFG_ZD);
    motion_spi_write(CLICK_THS, CLICK_THS_THS(127));
    motion_spi_write(TIME_LIMIT, TIME_LIMIT_TLI(127));
    motion_spi_write(TIME_LATENCY, TIME_LATENCY_TLA(127));

    return MP_OBJ_FROM_PTR(self);
}

typedef struct _hubs_MoveHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    mp_obj_t button;
    mp_obj_t imu;
    mp_obj_t light;
    mp_obj_t system;
} hubs_MoveHub_obj_t;

static const pb_obj_enum_member_t *movehub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_MoveHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_MoveHub_obj_t *self = m_new_obj(hubs_MoveHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    self->button = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(movehub_buttons), movehub_buttons, pbio_button_is_pressed);
    self->imu = hubs_MoveHub_IMU_make_new();
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_MoveHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_MoveHub_obj_t, battery),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_MoveHub_obj_t, button),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_imu, hubs_MoveHub_obj_t, imu),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_MoveHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_MoveHub_obj_t, system),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = PYBRICKS_HUB_CLASS_NAME,
        .make_new = hubs_MoveHub_make_new,
        .attr = pb_attribute_handler,
    },
    .attr_dict = hubs_MoveHub_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_MoveHub_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB
