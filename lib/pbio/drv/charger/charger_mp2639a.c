// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

// driver for MPS MP2639A USB battery charger chip

// NOTE: The datasheet uses CHG and CHGOK interchangeably. We are using CHG
// here since it is shorter. Also, the datasheet is ambiguous as to whether
// low means the CHG signal is low or the pin measured with an oscilloscope
// is logic low. Refer to the pages in the datasheet with oscilloscope captures
// to see what is really going on. When the (not inverted) CHG signal is "on"
// it means "charging", not "charging complete".

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CHARGER_MP2639A

#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/adc.h>
#include <pbdrv/charger.h>
#include <pbdrv/gpio.h>
#if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM | PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM
#include <pbdrv/pwm.h>
#endif
#if PBDRV_CONFIG_CHARGER_MP2639A_CHG_RESISTOR_LADDER
#include <pbdrv/resistor_ladder.h>
#endif
#include <pbio/error.h>
#include <pbio/util.h>

#include "../core.h"
#include "charger_mp2639a.h"

#define platform pbdrv_charger_mp2639a_platform_data

PROCESS(pbdrv_charger_mp2639a_process, "MP2639A");

#if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
static pbdrv_pwm_dev_t *mode_pwm;
#endif
#if PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM
static pbdrv_pwm_dev_t *iset_pwm;
#endif

static pbdrv_charger_status_t pbdrv_charger_status;
static bool mode_pin_is_low;

void pbdrv_charger_init(void) {
    pbdrv_init_busy_up();
    process_start(&pbdrv_charger_mp2639a_process);
}

pbio_error_t pbdrv_charger_get_current_now(uint16_t *current) {
    pbio_error_t err = pbdrv_adc_get_ch(platform.ib_adc_ch, current);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Scaling from raw to mA determined emperically by measuring USB current
    // on a few hubs.
    *current = (*current * 35116 >> 16) - 123;

    return PBIO_SUCCESS;
}

pbdrv_charger_status_t pbdrv_charger_get_status(void) {
    return pbdrv_charger_status;
}

void pbdrv_charger_enable(bool enable, pbdrv_charger_limit_t limit) {
    #if PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM

    // Set the current limit (ISET) based on the type of charger attached.
    // The duty cycle values are hard-coded for SPIKE Prime hardware.

    uint32_t duty_cycle;
    switch (limit) {
        case PBDRV_CHARGER_LIMIT_CHARGING:
            duty_cycle = 100; // max duty cycle
            break;
        case PBDRV_CHARGER_LIMIT_STD_MAX:
            duty_cycle = 15; // 500 mA
            break;
        case PBDRV_CHARGER_LIMIT_STD_MIN:
            duty_cycle = 2; // 100 mA
            break;
        default:
            duty_cycle = 0;
            break;
    }

    pbdrv_pwm_set_duty(iset_pwm, platform.iset_pwm_ch, duty_cycle);

    #endif // PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM

    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM

    // REVISIT: only known use has max duty cycle of UINT16_MAX
    pbdrv_pwm_set_duty(mode_pwm, platform.mode_pwm_ch, enable ? 0 : UINT16_MAX);

    #else // PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM

    if (enable) {
        pbdrv_gpio_out_low(&platform.mode_gpio);
    } else {
        pbdrv_gpio_out_high(&platform.mode_gpio);
    }

    #endif // PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM

    // Need to keep track of MODE pin state for charging logic since /ACOK pin
    // is not wired up.
    mode_pin_is_low = enable;
}

/**
 * Gets the current CHG signal status (inverted compared to /CHG pin state).
 */
static bool read_chg(void) {
    #if PBDRV_CONFIG_CHARGER_MP2639A_CHG_RESISTOR_LADDER
    pbdrv_resistor_ladder_ch_flags_t flags;
    pbio_error_t err = pbdrv_resistor_ladder_get(platform.chg_resistor_ladder_id, &flags);
    if (err != PBIO_SUCCESS) {
        return false;
    }
    // Resistor ladder driver sets flag when /CHG pin is logic low so we don't
    // need to invert here.
    return flags & platform.chg_resistor_ladder_ch;
    #else
    // /CHG pin is active low.
    return !pbdrv_gpio_input(&platform.chg_gpio);
    #endif
}

PROCESS_THREAD(pbdrv_charger_mp2639a_process, ev, data) {
    PROCESS_BEGIN();

    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
    while (pbdrv_pwm_get_dev(platform.mode_pwm_id, &mode_pwm) != PBIO_SUCCESS) {
        PROCESS_PAUSE();
    }
    #endif

    #if PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM
    while (pbdrv_pwm_get_dev(platform.iset_pwm_id, &iset_pwm) != PBIO_SUCCESS) {
        PROCESS_PAUSE();
    }
    #endif

    pbdrv_charger_enable(false, PBDRV_CHARGER_LIMIT_NONE);

    #if !PBDRV_CONFIG_CHARGER_MP2639A_CHG_RESISTOR_LADDER
    // /CHG pin is pulled low or open drain.
    pbdrv_gpio_set_pull(&platform.chg_gpio, PBDRV_GPIO_PULL_UP);
    pbdrv_gpio_input(&platform.chg_gpio);
    #endif

    pbdrv_init_busy_down();

    // When there is a fault the /CHG pin will toggle on and off at 1Hz, so we
    // have to try to detect that to get 3 possible states out of a digital input.

    static bool chg_samples[7];
    static uint8_t chg_index = 0;
    static struct etimer timer;

    // sample at 4Hz
    etimer_set(&timer, 250);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        etimer_restart(&timer);

        chg_samples[chg_index] = read_chg();

        if (mode_pin_is_low) {
            // Count number of transitions seen during sampling window.
            int transitions = chg_samples[0] != chg_samples[PBIO_ARRAY_SIZE(chg_samples) - 1];
            for (size_t i = 1; i < PBIO_ARRAY_SIZE(chg_samples); i++) {
                transitions += chg_samples[i] != chg_samples[i - 1];
            }

            // A single physical transition will be counted as 2 due to the
            // circular buffer. More than 2 means the signal is cycling.
            if (transitions > 2) {
                // CHG blinking indicates a fault.
                pbdrv_charger_status = PBDRV_CHARGER_STATUS_FAULT;
            } else {
                // Otherwise, we are constant, rising, or falling, so
                // we need only the current (latest) state.
                if (chg_samples[chg_index]) {
                    // CHG signal is on (/CHG pin is logic low).
                    pbdrv_charger_status = PBDRV_CHARGER_STATUS_CHARGE;
                } else {
                    // CHG signal is off (/CHG pin is logic high).
                    pbdrv_charger_status = PBDRV_CHARGER_STATUS_COMPLETE;
                }
            }
        } else {
            // This means the battery is discharging. Note that the MP2639A is
            // NOT in discharge mode. That mode (used to charge external
            // devices) requires a momentary pulse on the /PB pin, which is
            // not wired up.
            pbdrv_charger_status = PBDRV_CHARGER_STATUS_DISCHARGE;
        }

        // Increment sampling index with wrap around.
        if (++chg_index >= PBIO_ARRAY_SIZE(chg_samples)) {
            chg_index = 0;
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_CHARGER_MP2639A
