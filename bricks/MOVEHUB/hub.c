#include <stdio.h>

#include "stm32f070xb.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "button.h"
#include "gpio.h"
#include "led.h"

// Bootloader reads this address to know if firmware loader should run
#define HUB_BOOTLOADER_MAGIC_ADDR   (*(uint32_t *)0x20000100)
#define HUB_BOOTLOADER_MAGIC_VALUE  0xAAAAAAAA

STATIC mp_obj_t hub_get_button(void) {
    return mp_obj_new_bool(button_get_state());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_get_button_obj, hub_get_button);

STATIC mp_obj_t hub_gpios(mp_obj_t value) {
    mp_uint_t action = (mp_obj_get_int(value) & 0xF00) >> 8;
    mp_uint_t port = (mp_obj_get_int(value) & 0x0F0) >> 4;
    mp_uint_t pin = (mp_obj_get_int(value) & 0x00F);
    mp_uint_t retval = 2;

    GPIO_TypeDef *gpio;

    switch(port){
        case 0:
            gpio = GPIOA;
            // printf("PORT: A\n");
            break;
        case 1:
            gpio = GPIOB;
            // printf("PORT: B\n");
            break;
        case 2:
            gpio = GPIOC;
            // printf("PORT: C\n");
            break;
        case 3:
            gpio = GPIOD;
            // printf("PORT: D\n");
            break;
        case 5:
            gpio = GPIOF;
            // printf("PORT: F\n");
            break;
        default:
            printf("Unknown port\n");
            return mp_obj_new_int_from_uint(3);
    }

    switch(action){
        case 0: // Init IN UP
            gpio_init(gpio, pin, GPIO_MODE_IN, GPIO_PULL_UP, 0);
            printf("Init PIN %d as INput pull up\n", pin);
            break;
        case 1: // Init OUT
            gpio_init(gpio, pin, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
            printf("Init PIN %d as OUTput\n", pin);
            break;
        case 2: // Read
            retval = gpio_get(gpio, pin);
            // printf("Read PIN %d\n", pin);
            break;
        case 3: // SET LOW
            gpio_low(gpio, pin);
            printf("Set PIN %d low\n", pin);
            break;
        case 4: // SET HIGH
            gpio_high(gpio, pin);
            printf("Make PIN %d high\n", pin);
            break;
        case 5: // Init IN DOWN
            gpio_init(gpio, pin, GPIO_MODE_IN, GPIO_PULL_DOWN, 0);
            printf("Init PIN %d as INput pull down\n", pin);
            break;
        default:
            printf("Unknown Action \n");
            return mp_obj_new_int_from_uint(4);
    }
    return mp_obj_new_int_from_uint(retval);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_gpios_obj, hub_gpios);

STATIC mp_obj_t hub_power_off(void) {
    led_deinit();

    // setting PB11 low cuts the power
    GPIOB->BRR = GPIO_BRR_BR_11;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_power_off_obj, hub_power_off);

STATIC mp_obj_t hub_reboot(mp_obj_t bootloader) {
    if (mp_obj_is_true(bootloader)) {
        HUB_BOOTLOADER_MAGIC_ADDR = HUB_BOOTLOADER_MAGIC_VALUE;
    }
    // this function never returns
    NVIC_SystemReset();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_reboot_obj, hub_reboot);

STATIC mp_obj_t hub_set_light(mp_obj_t state) {
    mp_obj_t len;

    if (MP_OBJ_IS_TYPE(state, &mp_type_NoneType)) {
        led_set_rgb(0, 0, 0);
        return mp_const_none;
    }
    
    if (MP_OBJ_IS_STR(state)) {
        qstr color = mp_obj_str_get_qstr(state);

        // TODO: adjust the colors so they look right
        switch (color) {
        case MP_QSTR_red:
            led_set_rgb(255, 0, 0);
            break;
        case MP_QSTR_green:
            led_set_rgb(0, 255, 0);
            break;
        case MP_QSTR_blue:
            led_set_rgb(0, 0, 255);
            break;
        case MP_QSTR_cyan:
            led_set_rgb(0, 255, 255);
            break;
        case MP_QSTR_magenta:
            led_set_rgb(255, 0, 255);
            break;
        case MP_QSTR_yellow:
            led_set_rgb(255, 255, 0);
            break;
        case MP_QSTR_white:
            led_set_rgb(255, 255, 255);
            break;
        default:
            mp_raise_ValueError("Unknown color");
        }

        return mp_const_none;
    }

    len = mp_obj_len_maybe(state);
    if (len != MP_OBJ_NULL && mp_obj_get_int(len) >= 3) {
        mp_int_t r, g, b;

        // TODO: do we want to scale these so they look right too?
        r = mp_obj_get_int(mp_obj_subscr(state, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_SENTINEL));
        g = mp_obj_get_int(mp_obj_subscr(state, MP_OBJ_NEW_SMALL_INT(1), MP_OBJ_SENTINEL));
        b = mp_obj_get_int(mp_obj_subscr(state, MP_OBJ_NEW_SMALL_INT(2), MP_OBJ_SENTINEL));

        led_set_rgb(r, g, b);

        return mp_const_none;
    }

    mp_raise_TypeError("Must be string or RGB value");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);

STATIC const mp_map_elem_t hub_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_hub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_button), (mp_obj_t)&hub_get_button_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&hub_gpios_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_off), (mp_obj_t)&hub_power_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reboot), (mp_obj_t)&hub_reboot_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_light), (mp_obj_t)&hub_set_light_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_hub_globals,
    hub_globals_table
);

const mp_obj_module_t mp_module_hub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_hub_globals,
};
