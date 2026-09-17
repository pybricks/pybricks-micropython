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

#include "rproc_nxt.h"
#include "rproc_nxt_twi.h"

#define DEBUG 1

#if DEBUG
#include <pbdrv/clock.h>
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Milliseconds of processing time given to the AVR between transfers. The AVR
 * gives up on the ARM after two seconds, so this only needs to be short enough
 * to leave room for a few recovery attempts within that.
 */
#define AVR_GRACE_PERIOD_MS 2

/**
 * Milliseconds before a transfer that never completes is considered stuck. The
 * longest transfer, the handshake, takes just over a millisecond at 400 kHz.
 */
#define AVR_TRANSFER_TIMEOUT_MS 20

/** Corrupt responses tolerated before the link is rebuilt from scratch. */
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
    uint16_t sensor_adc[NXT_N_SENSORS];
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

#if DEBUG
static void pbdrv_rproc_nxt_debug_fault(const char *phase) {
    uint32_t fault_status;
    uint32_t bytes_remaining;
    pbdrv_rproc_nxt_twi_get_fault_info(&fault_status, &bytes_remaining);
    DEBUG_PRINT("avr %s: %s, sr=%08lx left=%lu\n", phase,
        pbdrv_rproc_nxt_twi_get_status() == PBDRV_RPROC_NXT_TWI_STATUS_ERROR ? "bus error" : "timeout",
        (unsigned long)fault_status, (unsigned long)bytes_remaining);
}
#else
#define pbdrv_rproc_nxt_debug_fault(phase)
#endif

/**
 * Waits for the transfer started just above to end, one way or another, and
 * records in @p ok whether it got there without the bus going wrong.
 */
#define AWAIT_TRANSFER(state, timer, ok)                                           \
    do {                                                                           \
        pbio_os_timer_set((timer), AVR_TRANSFER_TIMEOUT_MS);                       \
        PBIO_OS_AWAIT_UNTIL((state), pbdrv_rproc_nxt_twi_get_status() != PBDRV_RPROC_NXT_TWI_STATUS_BUSY || pbio_os_timer_is_expired((timer))); \
        (ok) = pbdrv_rproc_nxt_twi_get_status() == PBDRV_RPROC_NXT_TWI_STATUS_READY; \
    } while (0)

static pbio_error_t pbdrv_rproc_nxt_link_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t grace_timer;
    static pbio_os_timer_t transfer_timer;

    static uint32_t failed_checksums;
    static bool transfer_ok;

    #if DEBUG
    static uint32_t link_starts;
    static uint32_t cycles;
    #endif

    PBIO_OS_ASYNC_BEGIN(state)

    for (;;) {
        // Either this is the first attempt or a previous one went wrong, so
        // start from a known state: reset the bus, then tell the AVR that we
        // are alive. Without the handshake the AVR assumes the ARM is dead and
        // cuts the power a few seconds later, which is the clicking brick.
        pbdrv_rproc_nxt_twi_init();

        failed_checksums = 0;

        #if DEBUG
        DEBUG_PRINT("avr: link start %lu at %lu ms\n",
            (unsigned long)++link_starts, (unsigned long)pbdrv_clock_get_ms());
        cycles = 0;
        #endif

        static const char avr_init_handshake[] = "\xCC" "Let's samba nxt arm in arm, (c)LEGO System A/S";
        pbdrv_rproc_nxt_twi_write((const uint8_t *)avr_init_handshake, sizeof(avr_init_handshake) - 1);
        AWAIT_TRANSFER(state, &transfer_timer, transfer_ok);
        if (!transfer_ok) {
            pbdrv_rproc_nxt_debug_fault("handshake");
            continue;
        }
        DEBUG_PRINT("avr: handshake sent\n");

        pbio_os_timer_set(&grace_timer, AVR_GRACE_PERIOD_MS);

        while (failed_checksums < AVR_MAX_FAILED_CHECKSUMS) {

            // Allow processing on AVR.
            PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&grace_timer));
            pbio_os_timer_extend(&grace_timer);

            // Double buffer command to send to AVR.
            static uint8_t send_buf[sizeof(pbdrv_rproc_nxt_send_data)];

            // Copy data and set checksum if changed.
            if (pbdrv_rproc_nxt_send_data.changed) {
                pbdrv_rproc_nxt_send_data.changed = false;

                memcpy(send_buf, &pbdrv_rproc_nxt_send_data, sizeof(send_buf) - 1);
                send_buf[sizeof(send_buf) - 1] = pbdrv_rproc_nxt_get_checksum(send_buf, sizeof(send_buf) - 1);
            }

            pbdrv_rproc_nxt_twi_write(send_buf, sizeof(send_buf));
            AWAIT_TRANSFER(state, &transfer_timer, transfer_ok);
            if (!transfer_ok) {
                pbdrv_rproc_nxt_debug_fault("write");
                break;
            }

            // Allow processing on AVR.
            PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&grace_timer));
            pbio_os_timer_extend(&grace_timer);

            // Get state data from the AVR.
            static uint8_t recv_buf[sizeof(pbdrv_rproc_nxt_received_data)];
            pbdrv_rproc_nxt_twi_read(recv_buf, sizeof(recv_buf));
            AWAIT_TRANSFER(state, &transfer_timer, transfer_ok);
            if (!transfer_ok) {
                pbdrv_rproc_nxt_debug_fault("read");
                break;
            }

            if (pbdrv_rproc_nxt_get_checksum(recv_buf, sizeof(recv_buf)) == 0) {
                failed_checksums = 0;
                memcpy(&pbdrv_rproc_nxt_received_data, recv_buf, sizeof(recv_buf));
                #if DEBUG
                // One line per second or so, to show the link is alive and
                // that the numbers coming back are plausible.
                if (++cycles % 250 == 0) {
                    DEBUG_PRINT("avr: %lu ok, batt=%u btn=%u adc=%u %u %u %u\n",
                        (unsigned long)cycles,
                        pbdrv_rproc_nxt_received_data.battery_and_version_info,
                        pbdrv_rproc_nxt_received_data.button_adc,
                        pbdrv_rproc_nxt_received_data.sensor_adc[0],
                        pbdrv_rproc_nxt_received_data.sensor_adc[1],
                        pbdrv_rproc_nxt_received_data.sensor_adc[2],
                        pbdrv_rproc_nxt_received_data.sensor_adc[3]);
                }
                #endif
            } else {
                failed_checksums++;
                DEBUG_PRINT("avr: bad checksum %lu, %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    (unsigned long)failed_checksums,
                    recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3], recv_buf[4], recv_buf[5],
                    recv_buf[6], recv_buf[7], recv_buf[8], recv_buf[9], recv_buf[10], recv_buf[11], recv_buf[12]);
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
 * Gets the analog value of a sensor ADC pin.
 */
pbio_error_t pbdrv_rproc_nxt_get_sensor_adc(uint8_t index, uint16_t *value) {
    if (index >= NXT_N_SENSORS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *value = pbdrv_rproc_nxt_received_data.sensor_adc[index];
    return PBIO_SUCCESS;
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

/**
 * Tests whether the link to the AVR is between transfers.
 *
 * Drivers that have to disable interrupts for more than about a millisecond,
 * such as when programming flash, should wait for this first. Aborting a
 * transfer in progress breaks the link, and with it the power supply that the
 * AVR controls.
 *
 * @return True if no transfer is in progress, else false.
 */
bool pbdrv_rproc_nxt_link_is_idle(void) {
    return pbdrv_rproc_nxt_twi_get_status() != PBDRV_RPROC_NXT_TWI_STATUS_BUSY;
}

void pbdrv_rproc_init(void) {
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

    // The AVR acts on the first packet that carries this command and has a
    // valid checksum, but it only looks at it while the link is up. Restart the
    // link process from the top so that a link broken by whatever ran before
    // this, such as flash programming, gets rebuilt instead of never delivering
    // the command.
    PBIO_OS_ASYNC_RESET(&pbdrv_rproc_nxt_link_process.state);

    DEBUG_PRINT("avr: reset host, action %d\n", action);

    // The main event loop is no longer running, but we do want communication
    // with the AVR to keep going to transmit this command.
    for (;;) {
        pbdrv_rproc_nxt_link_process_thread(&pbdrv_rproc_nxt_link_process.state, NULL);
        #if DEBUG
        // Only so that buffered debug output still reaches the host.
        pbio_os_run_processes_once();
        #endif
    }
}

#endif // PBDRV_CONFIG_RPROC_NXT
