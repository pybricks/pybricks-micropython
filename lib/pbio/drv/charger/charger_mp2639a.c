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
#include <pbdrv/pwm.h>
#include <pbdrv/resistor_ladder.h>
#include <pbdrv/usb.h>
#include <pbio/error.h>
#include <pbio/util.h>
#include <pbio/os.h>

#include "../core.h"
#include "charger_mp2639a.h"

#define platform pbdrv_charger_mp2639a_platform_data

#if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
static pbdrv_pwm_dev_t *mode_pwm;
#endif
#if PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM
static pbdrv_pwm_dev_t *iset_pwm;
#endif

static pbdrv_charger_status_t pbdrv_charger_status;
static bool mode_pin_is_low;

pbio_error_t pbdrv_charger_get_current_now(uint16_t *current) {
    pbio_error_t err = pbdrv_adc_get_ch(platform.ib_adc_ch, current);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Scaling from raw to mA determined empirically by measuring USB current
    // on a few hubs.
    *current = (*current * 35116 >> 16) - 123;

    return PBIO_SUCCESS;
}

pbdrv_charger_status_t pbdrv_charger_get_status(void) {
    return pbdrv_charger_status;
}

/**
 * Enables or disables the charger.
 *
 * @param enable    True to enable the charger, false to disable.
 * @param limit     The current limit to set. Only used on some platforms.
 */
static void pbdrv_charger_enable(bool enable, pbdrv_charger_limit_t limit) {
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
 * Enables the charger if USB is connected, otherwise disables charger.
 */
static void pbdrv_charger_enable_if_usb_connected(void) {
    pbdrv_usb_bcd_t bcd = pbdrv_usb_get_bcd();
    bool enable = bcd != PBDRV_USB_BCD_NONE;
    pbdrv_charger_limit_t limit;

    // This battery charger chip will automatically monitor VBUS and
    // limit the current if the VBUS voltage starts to drop, so these limits
    // are a bit looser than they could be.
    switch (bcd) {
        case PBDRV_USB_BCD_NONE:
            limit = PBDRV_CHARGER_LIMIT_NONE;
            break;
        case PBDRV_USB_BCD_STANDARD_DOWNSTREAM:
            limit = PBDRV_CHARGER_LIMIT_STD_MAX;
            break;
        default:
            limit = PBDRV_CHARGER_LIMIT_CHARGING;
            break;
    }

    pbdrv_charger_enable(enable, limit);
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

// Sample CHG signal at 4Hz to capture transitions to detect fault condition.
#define PBDRV_CHARGER_MP2639A_STATUS_SAMPLE_TIME (250)

// After charging for a long time, we disable charging for some time. This
// matches observed behavior with the LEGO Education SPIKE V3.x firmware.
//
// Why? Due to the way the hardware works, the hub cannot be truly turned off
// while USB is plugged in. As a result, the charger is always on. For some
// battery-charger pairs, this causes the battery to stop charging normally in
// hardware when full as intended, automatically starting a new cycle after
// some time. But in some battery-charger pairs, the charger will reach a
// timeout state and not restart charging. When leaving such a combination
// plugged in overnight, it will time out and not be charged in the morning,
// which is not desirable. For this reason, we pause and restart charging
// manually if it has been plugged in for a long time.
#define PBDRV_CHARGER_MP2639A_CHARGE_TIMEOUT_MS (60 * 60 * 1000)
#define PBDRV_CHARGER_MP2639A_CHARGE_PAUSE_MS (30 * 1000)

static pbio_os_process_t pbdrv_charger_mp2639a_process;

pbio_error_t pbdrv_charger_mp2639a_process_thread(pbio_os_state_t *state) {

    static pbio_os_timer_t timer;

    ASYNC_BEGIN(state);

    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
    while (pbdrv_pwm_get_dev(platform.mode_pwm_id, &mode_pwm) != PBIO_SUCCESS) {
        AWAIT_MS(state, &timer, 1);
    }
    #endif

    #if PBDRV_CONFIG_CHARGER_MP2639A_ISET_PWM
    while (pbdrv_pwm_get_dev(platform.iset_pwm_id, &iset_pwm) != PBIO_SUCCESS) {
        AWAIT_MS(state, &timer, 1);
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
    static uint32_t charge_count = 0;

    for (;;) {
        AWAIT_MS(state, &timer, PBDRV_CHARGER_MP2639A_STATUS_SAMPLE_TIME);

        // Enable charger chip based on USB state. We don't need to disable it
        // on charger fault since the chip will automatically disable itself.
        // If we disable it here we can't detect the fault condition.
        pbdrv_charger_enable_if_usb_connected();

        chg_samples[chg_index] = read_chg();

        if (mode_pin_is_low) {

            // Keep track of how long we have been charging.
            charge_count++;

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
                    // CHG signal is off (/CHG pin is logic high). This is only
                    // valid after a few cycles. Otherwise it always briefly
                    // appears as if it is full when just plugged in.
                    pbdrv_charger_status = charge_count > 2 ?
                        PBDRV_CHARGER_STATUS_COMPLETE :
                        PBDRV_CHARGER_STATUS_DISCHARGE;
                }
            }
        } else {
            // This means the battery is discharging. Note that the MP2639A is
            // NOT in discharge mode. That mode (used to charge external
            // devices) requires a momentary pulse on the /PB pin, which is
            // not wired up.
            pbdrv_charger_status = PBDRV_CHARGER_STATUS_DISCHARGE;
            charge_count = 0;
        }

        // Increment sampling index with wrap around.
        if (++chg_index >= PBIO_ARRAY_SIZE(chg_samples)) {
            chg_index = 0;
        }

        // If we have been charging for a long time, pause charging for a while.
        if (charge_count > (PBDRV_CHARGER_MP2639A_CHARGE_TIMEOUT_MS / PBDRV_CHARGER_MP2639A_STATUS_SAMPLE_TIME)) {
            pbdrv_charger_status = PBDRV_CHARGER_STATUS_DISCHARGE;
            pbdrv_charger_enable(false, PBDRV_CHARGER_LIMIT_NONE);
            AWAIT_MS(state, &timer, PBDRV_CHARGER_MP2639A_CHARGE_PAUSE_MS);
            charge_count = 0;
        }
    }

    ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_charger_init(void) {
    pbdrv_init_busy_up();
    pbio_os_start_process(&pbdrv_charger_mp2639a_process, pbdrv_charger_mp2639a_process_thread);
}

#endif // PBDRV_CONFIG_CHARGER_MP2639A
