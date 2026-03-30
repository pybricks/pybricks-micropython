// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_PICO

#include <pbdrv/pwm.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>

#include "../drv/pwm/pwm.h"
#include "../drv/pwm/pwm_pico.h"
#include "../../drv/led/led_pwm.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"

static pbio_error_t pbdrv_pwm_pico_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    // Blue not available.
    if (ch == PBDRV_LED_PWM_CHANNEL_INVALID) {
        return PBIO_SUCCESS;
    }

    if (ch >= PBDRV_CONFIG_PWM_PICO_NUM_CHANNELS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    const pbdrv_pwm_pico_platform_data_t *pdata = dev->pdata;
    const pbdrv_pwm_pico_channel_t *channel = &pdata->channels[ch];

    pwm_set_gpio_level(channel->gpio, value);

    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_pico_funcs = {
    .set_duty = pbdrv_pwm_pico_set_duty,
};

void pbdrv_pwm_pico_init(pbdrv_pwm_dev_t *devs) {
    const pbdrv_pwm_pico_platform_data_t *pdata = &pbdrv_pwm_pico_platform_data;

    devs[pdata->id].funcs = &pbdrv_pwm_pico_funcs;
    devs[pdata->id].pdata = pdata;

    for (int i = 0; i < PBDRV_CONFIG_PWM_PICO_NUM_CHANNELS; i++) {
        const pbdrv_pwm_pico_channel_t *channel = &pdata->channels[i];

        // Tell the LED pin that the PWM is in charge of its value.
        gpio_set_function(channel->gpio, GPIO_FUNC_PWM);
        // Figure out which slice we just connected to the LED pin
        uint slice_num = pwm_gpio_to_slice_num(channel->gpio);

        // Get some sensible defaults for the slice configuration. By default,
        // the counter is allowed to wrap over its maximum range (0 to 2**16-1).
        pwm_config config = pwm_get_default_config();
        // Set divider, reduces counter clock to sysclock/this value.
        pwm_config_set_clkdiv(&config, 4.f);
        // Load the configuration into our PWM slice, and set it running.
        pwm_init(slice_num, &config, true);
    }
}

#endif // PBDRV_CONFIG_PWM_PICO
