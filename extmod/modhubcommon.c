#include <stdio.h>

#include <pbdrv/adc.h>
#include <pbdrv/battery.h>
#include <pbio/light.h>
#include <pbsys/sys.h>

#include "py/obj.h"

#include "mpconfigbrick.h"
#include "modmotor.h"
#include "modhubcommon.h"
#include "pberror.h"
#include "pbobj.h"
#include "gpio.h"


/* Color enum */

STATIC const mp_rom_map_elem_t pup_Color_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_none),      MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_NONE)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_purple),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_PURPLE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_blue),      MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_BLUE)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_green),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_GREEN)  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_yellow),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_YELLOW) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_orange),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_ORANGE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_red),       MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_RED)    },
    { MP_OBJ_NEW_QSTR(MP_QSTR_white),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_WHITE)  },
};
PB_DEFINE_CONST_ENUM(pup_Color_enum, pup_Color_enum_table);

STATIC mp_obj_t hub_shutdown(void) {
    pbsys_power_off();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_shutdown_obj, hub_shutdown);

STATIC mp_obj_t hub_reboot() {
    pbsys_reboot(0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_reboot_obj, hub_reboot);

STATIC mp_obj_t hub_update() {
    pbsys_reboot(1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_update_obj, hub_update);

STATIC mp_obj_t hub_set_light(mp_obj_t color) {
    pbio_light_color_t color_id = mp_obj_get_int(color);
    if (color_id < PBIO_LIGHT_COLOR_NONE || color_id > PBIO_LIGHT_COLOR_PURPLE) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    if (color_id == PBIO_LIGHT_COLOR_NONE) {
        pbio_light_off(PBIO_PORT_SELF);
    }
    else {
        pbio_light_on(PBIO_PORT_SELF, color_id);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);

#if PYBRICKS_ENABLE_HARDWARE_DEBUG
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

MP_DEFINE_CONST_FUN_OBJ_3(hub_gpios_obj, hub_gpios);

STATIC mp_obj_t hub_read_adc(mp_obj_t ch) {
    uint16_t value;

    pb_assert(pbdrv_adc_get_ch(mp_obj_get_int(ch), &value));

    return mp_obj_new_int(value);
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_read_adc_obj, hub_read_adc);
#endif //PYBRICKS_ENABLE_HARDWARE_DEBUG
