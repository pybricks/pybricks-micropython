// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TIAM1808

#include <pbdrv/pwm.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>

#include "../drv/pwm/pwm.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_ev3.h"
#include "../../drv/gpio/gpio_ev3.h"

static pbio_error_t pbdrv_pwm_tiam1808_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    // Blue not available.
    if (ch == PBDRV_LED_PWM_CHANNEL_INVALID) {
        return PBIO_SUCCESS;
    }

    pbdrv_rproc_ev3_pru1_shared_ram.pwm_duty_cycle[ch] = value;
    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_tiam1808_funcs = {
    .set_duty = pbdrv_pwm_tiam1808_set_duty,
};

#define PINMUX_ALT_PRU1     4

void pbdrv_pwm_tiam1808_init(pbdrv_pwm_dev_t *devs) {
    devs[0].funcs = &pbdrv_pwm_tiam1808_funcs;

    // Set GPIO alt modes for the PRU
    for (int j = 0; j < PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS; j++) {
        pbdrv_gpio_alt(&pbdrv_pwm_tiam1808_platform_data.gpios[j], PINMUX_ALT_PRU1);
    }
}

#endif // PBDRV_CONFIG_PWM_TIAM1808
