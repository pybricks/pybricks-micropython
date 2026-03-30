// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

#include <pbdrv/gpio.h>

#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_pico.h"

#include "hardware/gpio.h"

enum {
    PWM_DEV_0,
};

enum {
    LED_DEV_0_STATUS,
};

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .pin = 17,
};

// LED

static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    .r_factor = 1000,
    .g_factor = 1000,
    .b_factor = 0,
    // Tuned with scale_factor=17 so pure red/green are as close as possible
    // to UINT16_MAX without exceeding it.
    .r_brightness = 83,
    .g_brightness = 83,
    .b_brightness = 0,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_0_STATUS,
        .r_id = PWM_DEV_0,
        .r_ch = 0,
        .g_id = PWM_DEV_0,
        .g_ch = 1,
        // Blue not available.
        .b_id = PWM_DEV_0,
        .b_ch = PBDRV_LED_PWM_CHANNEL_INVALID,
        .scale_factor = 17,
    },
};

// PWM

const pbdrv_pwm_pico_platform_data_t pbdrv_pwm_pico_platform_data = {
    .id = PWM_DEV_0,
    .channels = {
        { .gpio = 14 }, // LED0
        { .gpio = 15 }, // LED1
    }
};

int main(void) {
    extern void pbsys_main(void);
    pbsys_main();
}
