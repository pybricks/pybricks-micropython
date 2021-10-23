// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// driver for MPS MP2639A USB battery charger chip

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CHARGER_MP2639A

#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/adc.h>
#include <pbdrv/charger.h>
#include <pbdrv/gpio.h>
#if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
#include <pbdrv/pwm.h>
#endif
#include <pbio/error.h>

#include "../core.h"
#include "charger_mp2639a.h"

#define platform pbdrv_charger_mp2639a_platform_data

PROCESS(pbdrv_charger_mp2639a_process, "MP2639A");

#if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
static pbdrv_pwm_dev_t *mode_pwm;
#endif

void pbdrv_charger_init(void) {
    pbdrv_init_busy_up();
    process_start(&pbdrv_charger_mp2639a_process);
}

pbio_error_t pbdrv_charger_get_current_now(uint16_t *current) {
    pbio_error_t err = pbdrv_adc_get_ch(platform.ib_adc_ch, current);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // TODO: find and apply scaling

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_charger_get_status(pbdrv_charger_status_t *status) {

    // TODO: implement PBDRV_CHARGER_STATUS_FAULT

    #if PBDRV_CONFIG_CHARGER_MP2639A_CHG_ADC
    uint16_t value;
    pbio_error_t err = pbdrv_adc_get_ch(platform.chg_adc_ch, &value);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // TODO: this code should be shared with button_adc since it is tuned
    // specifically for SPIKE Prime

    bool chg = false;

    if (value > 3642) {
        // chg = false;
    } else if (value > 3142) {
        chg = true;
    } else if (value > 2879) {
        // chg = false;
    } else if (value > 2634) {
        chg = true;
    } else if (value > 2449) {
        // chg = false;
    } else if (value > 2209) {
        chg = true;
    } else if (value > 2072) {
        // chg = false;
    } else if (value > 1800) {
        chg = true;
    } else {
        // hardware failure?
        return PBIO_ERROR_IO;
    }

    // CHG pin is active low
    *status = chg ? PBDRV_CHARGER_STATUS_DISCHARGE : PBDRV_CHARGER_STATUS_CHARGE;

    #else
    // CHG pin is active low
    *status = pbdrv_gpio_input(&platform.chg_gpio) ? PBDRV_CHARGER_STATUS_DISCHARGE : PBDRV_CHARGER_STATUS_CHARGE;
    #endif

    return PBIO_SUCCESS;
}

void pbdrv_charger_enable(bool enable) {
    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
    // REVISIT: only known use has max duty cycle of UINT16_MAX
    pbdrv_pwm_set_duty(mode_pwm, platform.mode_pwm_ch, enable ? 0 : UINT16_MAX);
    #else
    if (enable) {
        pbdrv_gpio_out_low(&platform.mode_gpio);
    } else {
        pbdrv_gpio_out_high(&platform.mode_gpio);
    }
    #endif
}

PROCESS_THREAD(pbdrv_charger_mp2639a_process, ev, data) {
    PROCESS_BEGIN();

    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
    while (pbdrv_pwm_get_dev(platform.mode_pwm_id, &mode_pwm) != PBIO_SUCCESS) {
        PROCESS_PAUSE();
    }
    #endif

    pbdrv_charger_enable(false);

    #if !PBDRV_CONFIG_CHARGER_MP2639A_CHG_ADC
    // CHG pin is pulled low or open drain
    pbdrv_gpio_set_pull(&platform.chg_gpio, PBDRV_GPIO_PULL_UP);
    pbdrv_gpio_input(&platform.chg_gpio);
    #endif

    pbdrv_init_busy_down();

    // TODO: monitor CHG pin to detect fault (1 Hz flashing)

    PROCESS_END();
}

#endif // PBDRV_CONFIG_CHARGER_MP2639A
