#include <stdio.h>

#include "stm32f030xc.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "gpio.h"

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
    // setting PB11 low cuts the power
    GPIOB->BRR = GPIO_BRR_BR_11;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hub_power_off_obj, hub_power_off);

STATIC const mp_map_elem_t hub_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_hub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&hub_gpios_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_off), (mp_obj_t)&hub_power_off_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_hub_globals,
    hub_globals_table
);

const mp_obj_module_t mp_module_hub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_hub_globals,
};
