// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB

#include "py/runtime.h"

#include <pbdrv/gpio.h>
#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pbio/int_math.h>
#include <pbio/geometry.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

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
static void motion_spi_wait(void) {
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

static void motion_get_acceleration_raw(int8_t *data) {
    motion_spi_read(OUT_Y_H, (uint8_t *)&data[0]);
    motion_spi_read(OUT_Z_H, (uint8_t *)&data[2]);
    motion_spi_read(OUT_X_H, (uint8_t *)&data[1]);
    data[1] = -data[1];
}

static void motion_get_acceleration_rotated(int8_t *accel, uint8_t *index, int8_t *sign) {
    int8_t raw[3];
    motion_get_acceleration_raw(raw);

    // Based on given orientation, shift and flip the data.
    for (uint8_t i = 0; i < 3; i++) {
        accel[index[i]] = raw[i] * sign[i];
    }
}

typedef struct {
    mp_obj_base_t base;
    uint8_t transform_index[3];
    int8_t transform_sign[3];
} hubs_MoveHub_IMU_obj_t;

// This is an integer version of pybricks._common.IMU.up
static mp_obj_t hubs_MoveHub_IMU_up(mp_obj_t self_in) {

    int8_t values[3];
    motion_get_acceleration_raw(values);

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

static mp_obj_t hubs_MoveHub_IMU_acceleration(mp_obj_t self_in) {

    hubs_MoveHub_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Gets signed acceleration data.
    int8_t data[3];
    motion_get_acceleration_rotated(data, self->transform_index, self->transform_sign);

    // Convert to appropriate units and return as tuple.
    mp_obj_t values[3];
    for (uint8_t i = 0; i < 3; i++) {
        values[i] = MP_OBJ_NEW_SMALL_INT(data[i] * 158);
    }
    return mp_obj_new_tuple(3, values);
}
static MP_DEFINE_CONST_FUN_OBJ_1(hubs_MoveHub_IMU_acceleration_obj, hubs_MoveHub_IMU_acceleration);

// pybricks._common.SimpleAccelerometer.tilt
static mp_obj_t hubs_MoveHub_IMU_tilt(mp_obj_t self_in) {

    hubs_MoveHub_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Get acceleration data in arbitrary units.
    int8_t accel[3];
    motion_get_acceleration_rotated(accel, self->transform_index, self->transform_sign);

    // Now we can compute the tilt values.
    mp_obj_t tilt[2];

    // Pitch
    tilt[0] = mp_obj_new_int(pbio_int_math_atan2(-accel[0], pbio_int_math_sqrt(accel[2] * accel[2] + accel[1] * accel[1])));

    // Roll
    tilt[1] = mp_obj_new_int(pbio_int_math_atan2(accel[1], accel[2]));

    // Return as tuple
    return mp_obj_new_tuple(2, tilt);
}
MP_DEFINE_CONST_FUN_OBJ_1(hubs_MoveHub_IMU_tilt_obj, hubs_MoveHub_IMU_tilt);

static const mp_rom_map_elem_t hubs_MoveHub_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&hubs_MoveHub_IMU_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_up),           MP_ROM_PTR(&hubs_MoveHub_IMU_up_obj)           },
    { MP_ROM_QSTR(MP_QSTR_tilt),         MP_ROM_PTR(&hubs_MoveHub_IMU_tilt_obj) },
};
static MP_DEFINE_CONST_DICT(hubs_MoveHub_IMU_locals_dict, hubs_MoveHub_IMU_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(hubs_MoveHub_IMU_type,
    MP_QSTRnull,
    MP_TYPE_FLAG_NONE,
    locals_dict, &hubs_MoveHub_IMU_locals_dict);

static mp_obj_t hubs_MoveHub_IMU_make_new(mp_obj_t top_side_in, mp_obj_t front_side_in) {
    hubs_MoveHub_IMU_obj_t *self = mp_obj_malloc(hubs_MoveHub_IMU_obj_t, &hubs_MoveHub_IMU_type);

    // Save base orientation.
    int8_t top_side_axis = mp_obj_get_int(top_side_in);
    int8_t front_side_axis = mp_obj_get_int(front_side_in);
    if (top_side_axis == front_side_axis ||
        top_side_axis == 0 || front_side_axis == 0 ||
        pbio_int_math_abs(top_side_axis) > 3 ||
        pbio_int_math_abs(front_side_axis) > 3) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get the local X and Z axes corresponding to given sides.
    self->transform_index[0] = pbio_int_math_abs(front_side_axis) - 1,
    self->transform_index[2] = pbio_int_math_abs(top_side_axis) - 1,
    self->transform_sign[0] = pbio_int_math_sign(front_side_axis),
    self->transform_sign[2] = pbio_int_math_sign(top_side_axis),

    // Get the remaining Y axis to complete the coordinate system.
    pbio_geometry_get_complementary_axis(self->transform_index, self->transform_sign);

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
    #if PYBRICKS_PY_COMMON_BLE
    mp_obj_t ble;
    #endif
    mp_obj_t button;
    mp_obj_t imu;
    mp_obj_t light;
    mp_obj_t system;
} hubs_MoveHub_obj_t;


static mp_obj_t hubs_MoveHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_INT(top_side, pb_type_Axis_Z_int_enum),
        PB_ARG_DEFAULT_INT(front_side, pb_type_Axis_X_int_enum)
        #if PYBRICKS_PY_COMMON_BLE
        , PB_ARG_DEFAULT_INT(broadcast_channel, 0)
        , PB_ARG_DEFAULT_OBJ(observe_channels, mp_const_empty_tuple_obj)
        #endif
        );

    hubs_MoveHub_obj_t *self = mp_obj_malloc(hubs_MoveHub_obj_t, type);
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    #if PYBRICKS_PY_COMMON_BLE
    self->ble = pb_type_BLE_new(broadcast_channel_in, observe_channels_in);
    #endif
    self->button = pb_type_Keypad_obj_new(pb_type_button_pressed_hub_single_button);
    self->imu = hubs_MoveHub_IMU_make_new(top_side_in, front_side_in);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t hubs_MoveHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_MoveHub_obj_t, battery),
    #if PYBRICKS_PY_COMMON_BLE
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_ble, hubs_MoveHub_obj_t, ble),
    #endif
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_MoveHub_obj_t, button),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_MoveHub_obj_t, button), // backwards compatibility
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_imu, hubs_MoveHub_obj_t, imu),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_MoveHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_MoveHub_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    PYBRICKS_HUB_CLASS_NAME,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_MoveHub_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_MoveHub_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB
