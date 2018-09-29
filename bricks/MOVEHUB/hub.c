#include <stdio.h>

#include <pbdrv/adc.h>
#include <pbio/button.h>
#include <pbio/light.h>
#include <pbsys/sys.h>
#include <modmotor.h>
#include <mpconfigbrick.h>

#include "stm32f070xb.h"

#include "py/builtin.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "gpio.h"

#if PYBRICKS_HW_ENABLE_MOTORS
const mp_obj_type_id_t motor_MovehubMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MovehubMotor,
    .device_id = PBIO_ID_PUP_MOVEHUB_MOTOR,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
#endif //PYBRICKS_ENABLE_MOTORS

STATIC mp_obj_t hub_get_button(void) {
    pbio_button_flags_t btn;

    // should always return success given these parameters
    pbio_button_is_pressed(PBIO_PORT_SELF, &btn);

    return mp_obj_new_bool(btn & PBIO_BUTTON_CENTER);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_get_button_obj, hub_get_button);

mp_obj_t hub_wait_btn_press(void) {
    pbio_button_flags_t btn;

    for (;;) {
        pbio_button_is_pressed(PBIO_PORT_SELF, &btn);
        if (btn & PBIO_BUTTON_CENTER) {
            return mp_const_none;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_wait_btn_press_obj, hub_wait_btn_press);

mp_obj_t hub_wait_btn_release(void) {
    pbio_button_flags_t btn;

    for (;;) {
        pbio_button_is_pressed(PBIO_PORT_SELF, &btn);
        if (!(btn & PBIO_BUTTON_CENTER)) {
            return mp_const_none;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_wait_btn_release_obj, hub_wait_btn_release);

STATIC mp_obj_t hub_gpios(mp_obj_t bank, mp_obj_t pin, mp_obj_t action) {
    GPIO_TypeDef *gpio;
    uint8_t pin_idx;

    switch(mp_obj_get_int(bank)) {
        case 1:
            gpio = GPIOA;
            // printf("PORT: A\n");
            break;
        case 2:
            gpio = GPIOB;
            // printf("PORT: B\n");
            break;
        case 3:
            gpio = GPIOC;
            // printf("PORT: C\n");
            break;
        case 4:
            gpio = GPIOD;
            // printf("PORT: D\n");
            break;
        case 6:
            gpio = GPIOF;
            // printf("PORT: F\n");
            break;
        default:
            mp_raise_ValueError("Unknown bank");
    }

    pin_idx = mp_obj_get_int(pin);
    if (pin_idx < 0 || pin_idx >= 16) {
        mp_raise_ValueError("Pin out of range");
    }

    switch(mp_obj_get_int(action)) {
        case 0: // output, low
            gpio_low(gpio, pin_idx);
            break;
        case 1: // output, high
            gpio_high(gpio, pin_idx);
            break;
        case 2: // input
            gpio_get(gpio, pin_idx);
            break;
        case 3: // read
            return MP_OBJ_NEW_SMALL_INT(gpio_get(gpio, pin_idx));
        default:
            mp_raise_ValueError("Unknown Action");
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(hub_gpios_obj, hub_gpios);

STATIC mp_obj_t hub_power_off(void) {
    pbsys_power_off();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_power_off_obj, hub_power_off);

STATIC mp_obj_t hub_read_adc(mp_obj_t ch) {
    uint16_t value;
    pbio_error_t err;

    err = pbdrv_adc_get_ch(mp_obj_get_int(ch), &value);
    if (err != PBIO_SUCCESS) {
        mp_raise_ValueError("Invalid channel");
    }

    return mp_obj_new_int(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_read_adc_obj, hub_read_adc);

STATIC mp_obj_t hub_reboot(mp_obj_t bootloader) {
    pbsys_reboot(mp_obj_is_true(bootloader));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_reboot_obj, hub_reboot);

STATIC mp_obj_t hub_set_light(mp_obj_t state) {

    if (MP_OBJ_IS_TYPE(state, &mp_type_NoneType)) {
        pbio_light_off(PBIO_PORT_SELF);
        return mp_const_none;
    }

    if (MP_OBJ_IS_STR(state)) {
        qstr color = mp_obj_str_get_qstr(state);

        switch (color) {
        case MP_QSTR_white:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_WHITE);
            break;
        case MP_QSTR_red:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_RED);
            break;
        case MP_QSTR_orange:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_ORANGE);
            break;
        case MP_QSTR_yellow:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_YELLOW);
            break;
        case MP_QSTR_green:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_GREEN);
            break;
        case MP_QSTR_blue:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_BLUE);
            break;
        case MP_QSTR_purple:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_PURPLE);
            break;
        case MP_QSTR_pink:
            pbio_light_on(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_PINK);
            break;
        default:
            mp_raise_ValueError("Unknown color");
        }

        return mp_const_none;
    }

    mp_raise_TypeError("Must be string or None");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);

STATIC const mp_map_elem_t hub_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_hub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_button), (mp_obj_t)&hub_get_button_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_wait_btn_press), (mp_obj_t)&hub_wait_btn_press_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_wait_btn_release), (mp_obj_t)&hub_wait_btn_release_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&hub_gpios_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_off), (mp_obj_t)&hub_power_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_adc), (mp_obj_t)&hub_read_adc_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reboot), (mp_obj_t)&hub_reboot_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_light), (mp_obj_t)&hub_set_light_obj },
#if PYBRICKS_HW_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_MovehubMotor), (mp_obj_t)&motor_MovehubMotor_type},
#endif //PYBRICKS_HW_ENABLE_MOTORS
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_hub_globals,
    hub_globals_table
);

const mp_obj_module_t mp_module_hub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_hub_globals,
};
