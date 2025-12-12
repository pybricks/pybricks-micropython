// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2007 the NxOS developers
// See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
// Copyright (c) 2025 The Pybricks Authors

// Manages NXT AVR coprocessor

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC_NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>

#include <pbdrv/compiler.h>
#include <pbdrv/reset.h>

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbio/int_math.h>

#include "nxos/nxt.h"
#include "nxos/util.h"
#include "nxos/assert.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/_twi.h"

#include "rproc_nxt.h"

#define AVR_ADDRESS 1
#define AVR_MAX_FAILED_CHECKSUMS 3

/**
 * Commands that are periodically sent to the AVR.
 */
static struct PBDRV_PACKED {
    /** Set if the AVR should perform power management. */
    uint8_t power_mode;
    /** Motor PWM frequency. If power mode set above, this is its payload */
    uint8_t motor_pwm_frequency;
    /** Signed duty cycle percentage for the motors. */
    int8_t motor_duty_cycle[4];
    /** Motor bridge decay mode (1=slow, 0=fast). Motor A is LSB. */
    uint8_t motor_decay_mode;
    /** Sensor power settings. */
    uint8_t sensor_power;
    /** Data has changed. The copy that goes over the wire has the checksum here. */
    uint8_t changed;
} pbdrv_rproc_nxt_send_data = {
    .power_mode = 0,
    .motor_pwm_frequency = 8,
    .changed = true,
};

/**
 * Data periodically received from the AVR.
 */
static struct PBDRV_PACKED {
    /** Reading of the ADC on pin 1 */
    uint16_t sensor_adc_pc[NXT_N_SENSORS];
    /** Button value. This is only decoded when needed. */
    uint16_t button_adc;
    /** Version and battery info. */
    uint16_t battery_and_version_info;
    /** Checksum. */
    uint8_t checksum;
} pbdrv_rproc_nxt_received_data = {
    // Suitable initial value (Li-ion, 7311 mV) to avoid low battery-shutdown
    // if we poll this before the first real reading is available.
    .battery_and_version_info = 42512,
};

/**
 * Gets the checksum used for sent and received data buffers.
 *
 * All data including the checksum should add up to 0xFF.
 */
static uint8_t pbdrv_rproc_nxt_get_checksum(uint8_t *data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return ~checksum;
}

static pbio_os_process_t pbdrv_rproc_nxt_link_process;

static pbio_error_t pbdrv_rproc_nxt_link_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    static uint32_t failed_checksums = 0;

    PBIO_OS_ASYNC_BEGIN(state)

    for (;;) {
        failed_checksums = 0;

        // ARM-AVR link is not initialized. We need to send the hello string to
        // tell the AVR that we are alive. This will (among other things) stop the
        // "clicking brick" sound, and avoid having the brick powered down after a
        // few minutes by an AVR that doesn't see us coming up.
        static char avr_init_handshake[] = "\xCC" "Let's samba nxt arm in arm, (c)LEGO System A/S";
        nx__twi_write_async(AVR_ADDRESS, (uint8_t *)avr_init_handshake, sizeof(avr_init_handshake) - 1);
        PBIO_OS_AWAIT_UNTIL(state, nx__twi_ready());

        // Give the AVR some grace time between messages to run its own code.
        // Note that the TWI driver is not polling the event loop, so we are in
        // effect only checking nx__twi_ready once per millisecond
        // This is fine because of the grace period we add in anyway.
        pbio_os_timer_set(&timer, 2);

        while (failed_checksums < AVR_MAX_FAILED_CHECKSUMS) {

            // Allow processing on AVR.
            PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));
            pbio_os_timer_extend(&timer);

            // Double buffer command to send to AVR.
            static uint8_t send_buf[sizeof(pbdrv_rproc_nxt_send_data)];

            // Copy data and set checksum if changed.
            if (pbdrv_rproc_nxt_send_data.changed) {
                pbdrv_rproc_nxt_send_data.changed = false;

                memcpy(send_buf, &pbdrv_rproc_nxt_send_data, sizeof(send_buf) - 1);
                send_buf[sizeof(send_buf) - 1] = pbdrv_rproc_nxt_get_checksum(send_buf, sizeof(send_buf) - 1);
            }

            nx__twi_write_async(AVR_ADDRESS, send_buf, sizeof(send_buf));
            PBIO_OS_AWAIT_UNTIL(state, nx__twi_ready());

            // Allow processing on AVR.
            PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));
            pbio_os_timer_extend(&timer);

            // Get state data from the AVR.
            static uint8_t recv_buf[sizeof(pbdrv_rproc_nxt_received_data)];
            nx__twi_read_async(AVR_ADDRESS, recv_buf, sizeof(recv_buf));
            PBIO_OS_AWAIT_UNTIL(state, nx__twi_ready());
            if (pbdrv_rproc_nxt_get_checksum(recv_buf, sizeof(recv_buf)) == 0) {
                memcpy(&pbdrv_rproc_nxt_received_data, recv_buf, sizeof(recv_buf));
            } else {
                // Start over if we fail too many times.
                failed_checksums++;
            }
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

/**
 * Sets the duty cycle for a motor.
 *
 * @param index               Motor index (0-2).
 * @param duty_cycle_percent  Duty cycle percentage (-100 to 100).
 * @param slow_decay          True for slow decay mode, false for fast decay.
 *
 * @return ::PBIO_SUCCESS on success.
 *         ::PBIO_ERROR_INVALID_ARG if the index is out of range.
 */
pbio_error_t pbdrv_rproc_nxt_set_duty_cycle(uint8_t index, int32_t duty_cycle_percent, bool slow_decay) {
    if (index >= NXT_N_MOTORS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    pbdrv_rproc_nxt_send_data.motor_duty_cycle[index] = pbio_int_math_clamp(duty_cycle_percent, 100);

    if (slow_decay) {
        pbdrv_rproc_nxt_send_data.motor_decay_mode |= (1 << index);
    } else {
        pbdrv_rproc_nxt_send_data.motor_decay_mode &= ~(1 << index);
    }

    pbdrv_rproc_nxt_send_data.changed = true;
    return PBIO_SUCCESS;
}

/**
 * Sets the power pin for a sensor.
 *
 * @param index               Sensor index (0-3).
 * @param set                 True to turn on sensor power, false to turn it off.
 *
 * @return ::PBIO_SUCCESS on success.
 *         ::PBIO_ERROR_INVALID_ARG if the index is out of range.
 */
pbio_error_t pbdrv_rproc_nxt_set_sensor_power(uint8_t index, pbdrv_rproc_nxt_sensor_power_t power_type) {
    if (index >= NXT_N_SENSORS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Clear the two bits for this input (first port has bits 0 and 4).
    pbdrv_rproc_nxt_send_data.sensor_power &= ~((0x11) << index);

    // Set the new value.
    pbdrv_rproc_nxt_send_data.sensor_power |= (power_type << index);

    pbdrv_rproc_nxt_send_data.changed = true;
    return PBIO_SUCCESS;
}

/**
 * Gets the currently pressed buttons by reading the ADC value from the AVR.
 *
 * Can be the center button at at most one of the other buttons.
 *
 * @return Button flag.
 */
pbio_button_flags_t pbdrv_rproc_nxt_get_button_pressed(void) {

    uint16_t adc = pbdrv_rproc_nxt_received_data.button_adc;

    // Center button is a digital pin.
    pbio_button_flags_t buttons = 0;
    if (adc >= 0x07FF) {
        buttons = PBIO_BUTTON_CENTER;
        adc -= 0x07FF;
    }

    // Other buttons are on a resistor ladder.
    if (adc > 720) {
        buttons |= PBIO_BUTTON_DOWN;
    } else if (adc > 270) {
        buttons |= PBIO_BUTTON_RIGHT;
    } else if (adc > 60) {
        buttons |= PBIO_BUTTON_LEFT;
    }
    return buttons;
}

/**
 * Gets battery information from the AVR.
 *
 * @param [in] voltage Pointer to store the battery voltage in millivolts.
 * @return True if the button in the battery compartment is pressed, else false.
 */
bool pbdrv_rproc_nxt_get_battery_info(uint16_t *voltage) {
    uint16_t data = pbdrv_rproc_nxt_received_data.battery_and_version_info;

    // This data also contains:
    // avr.version.major = (data >> 13) & 0x3;
    // avr.version.minor = (data >> 10) & 0x7;

    // The data contains the voltage value, in units of 13.848mV. The
    // multiplication by 13.848 is approximated by a multiplication by 3545
    // followed by a division by 256.
    *voltage = ((data & 0x3ff) * 3545) >> 8;

    // Bit 15 represents the battery compartment button, pressed if high.
    return data & 0x8000;
}

void pbdrv_rproc_init(void) {
    // Set up the TWI driver to turn on the i2c bus, and kickstart the state
    // machine to start transmitting.
    nx__twi_init();

    pbio_os_process_start(&pbdrv_rproc_nxt_link_process, pbdrv_rproc_nxt_link_process_thread, NULL);
}

/**
 * Resets the host using the AVR coprocessor.
 *
 * @param action The type of reset to perform. Supports only
 *      ::PBDRV_RESET_ACTION_POWER_OFF and
 *      ::PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE.
 */
void pbdrv_rproc_nxt_reset_host(pbdrv_reset_action_t action) {
    switch (action) {
        case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
            pbdrv_rproc_nxt_send_data.power_mode = 0xA5;
            pbdrv_rproc_nxt_send_data.motor_pwm_frequency = 0x5A;
            break;
        case PBDRV_RESET_ACTION_POWER_OFF:
        default:
            pbdrv_rproc_nxt_send_data.power_mode = 0x5A;
            pbdrv_rproc_nxt_send_data.motor_pwm_frequency = 0;
            break;
    }
    pbdrv_rproc_nxt_send_data.changed = true;

    // The main event loop is no longer running, but we do want communication
    // with the AVR to keep going to transmit this command.
    for (;;) {
        pbdrv_rproc_nxt_link_process_thread(&pbdrv_rproc_nxt_link_process.state, NULL);
    }
}

#endif // PBDRV_CONFIG_RPROC_NXT
