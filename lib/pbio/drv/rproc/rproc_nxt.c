/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Manages NXT AVR coprocessor

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC_NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>

#include <pbdrv/reset.h>

#include <pbio/os.h>

#include "nxos/nxt.h"
#include "nxos/util.h"
#include "nxos/assert.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/_twi.h"

#include "nxos/drivers/_avr.h"

#define AVR_ADDRESS 1
#define AVR_MAX_FAILED_CHECKSUMS 3

const char avr_init_handshake[] =
    "\xCC" "Let's samba nxt arm in arm, (c)LEGO System A/S";

// Contains all the commands that are periodically sent to the AVR.
static volatile struct {
    // Tells the AVR to perform power management:
    enum {
        AVR_RUN = 0, // No power management (normal runtime mode).
        AVR_POWER_OFF, // Power down the brick.
        AVR_RESET_MODE, // Go into SAM-BA reset mode.
    } power_mode;

    // The speed and braking configuration of the motor ports.
    int8_t motor_speed[NXT_N_MOTORS];
    uint8_t motor_brake;

    // TODO: enable controlling of input power. Currently the input
    // stuff is ignored.
} to_avr = {
    AVR_RUN,   // Start in normal power mode.
    { 0, 0, 0 }, // All motors are off...
    0          // And set to coast.
};

// Contains all the status data periodically received from the AVR.
static volatile struct {
    // The analog reading of the analog pin on all active sensors.
    uint16_t adc_value[NXT_N_SENSORS];

    // The state of the NXT's buttons. Given the way that the buttons
    // are handled in hardware, only one button is reported pressed at a
    // time. See the nx_avr_button_t enumeration for values to test for.
    uint8_t buttons;

    // Battery information.
    struct {
        // True if the power supply is rechargeable battery pack or false if AA
        // batteries.
        bool is_accu_pack;
        uint16_t charge; // The remaining battery charge in mV.
    } battery;

    // The version of the AVR firmware. The currently supported version is 1.1.
    struct {
        uint8_t major;
        uint8_t minor;
    } version;
} from_avr;

// The following two arrays hold the data structures above, converted
// into the raw ARM-AVR communication format. Data to send is
// serialized into this buffer prior to sending, and received data is
// received into here before being deserialized into the status
// struct.
static uint8_t raw_from_avr[(2 * NXT_N_SENSORS) + // Sensor A/D value.
                            2 + // Buttons reading.
                            2 + // Battery type, charge and AVR firmware version.
                            1]; // Checksum.
static uint8_t raw_to_avr[1 + // Power mode
                          1 + // PWM frequency
                          4 + // output % for the 4 (?!)  motors
                          1 + // Output modes (brakes)
                          1 + // Input modes (sensor power)
                          1]; // Checksum

// Serialize the to_avr data structure into raw_to_avr, ready for
// sending to the AVR.
static void avr_pack_to_avr(void) {
    uint32_t i;
    uint8_t checksum = 0;

    memset(raw_to_avr, 0, sizeof(raw_to_avr));

    // Marshal the power mode configuration.
    switch (to_avr.power_mode) {
        case AVR_RUN:
            // Normal operating mode. First byte is 0, and the second (PWM frequency is
            // set to 8.
            raw_to_avr[1] = 8;
            break;
        case AVR_POWER_OFF:
            // Tell the AVR to shut us down.
            raw_to_avr[0] = 0x5A;
            raw_to_avr[1] = 0;
            break;
        case AVR_RESET_MODE:
            // Tell the AVR to boot SAM-BA.
            raw_to_avr[0] = 0x5A;
            raw_to_avr[1] = 0xA5;
            break;
    }

    // Marshal the motor speed settings.
    raw_to_avr[2] = to_avr.motor_speed[0];
    raw_to_avr[3] = to_avr.motor_speed[1];
    raw_to_avr[4] = to_avr.motor_speed[2];

    // raw_to_avr[5] is the value for the 4th motor, which doesn't exist. This is
    // probably a bug in the AVR firmware, but it is required. So we just latch
    // the value to zero.

    // Marshal the motor brake settings.
    raw_to_avr[6] = to_avr.motor_brake;

    // Calculate the checksum.
    for (i = 0; i < (sizeof(raw_to_avr) - 1); i++) {
        checksum += raw_to_avr[i];
    }
    raw_to_avr[sizeof(raw_to_avr) - 1] = ~checksum;
}

// Small helper to convert two bytes into an uint16_t.
static inline uint16_t unpack_word(uint8_t *word) {
    return (word[1] << 8) | word[0];
}

// Deserialize the AVR data structure in raw_from_avr into the from_avr status
// structure.
static pbio_error_t avr_unpack_from_avr(void) {
    uint8_t checksum = 0;
    uint16_t word;
    uint32_t i;
    uint8_t *p = raw_from_avr;

    // Compute the checksum of the received data. This is done by doing the
    // unsigned sum of all the bytes in the received buffer. They should add up
    // to 0xFF.
    for (i = 0; i < sizeof(raw_from_avr); i++) {
        checksum += raw_from_avr[i];
    }

    if (checksum != 0xff) {
        return PBIO_ERROR_IO;
    }

    // Unpack and store the 4 sensor analog readings.
    for (i = 0; i < NXT_N_SENSORS; i++) {
        from_avr.adc_value[i] = unpack_word(p);
        p += 2;
    }

    // Grab the buttons word (an analog reading), and compute the state of
    // buttons from that.
    word = unpack_word(p);
    p += 2;

    if (word > 1023) {
        from_avr.buttons = BUTTON_OK;
    } else if (word > 720) {
        from_avr.buttons = BUTTON_CANCEL;
    } else if (word > 270) {
        from_avr.buttons = BUTTON_RIGHT;
    } else if (word > 60) {
        from_avr.buttons = BUTTON_LEFT;
    } else {
        from_avr.buttons = BUTTON_NONE;
    }

    // Process the last word, which is a mix and match of many values.
    word = unpack_word(p);

    // Extract the AVR firmware version, as well as the type of power supply
    // connected.
    from_avr.version.major = (word >> 13) & 0x3;
    from_avr.version.minor = (word >> 10) & 0x7;
    from_avr.battery.is_accu_pack = word & 0x8000;

    // The rest of the word is the voltage value, in units of 13.848mV. As the
    // NXT does not have a floating point unit, the multiplication by 13.848 is
    // approximated by a multiplication by 3545 followed by a division by 256.
    from_avr.battery.charge = (((uint32_t)word & 0x3ff) * 3545) >> 8;

    return PBIO_SUCCESS;
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

            // Send commands to AVR.
            avr_pack_to_avr();
            nx__twi_write_async(AVR_ADDRESS, raw_to_avr, sizeof(raw_to_avr));
            PBIO_OS_AWAIT_UNTIL(state, nx__twi_ready());

            // Allow processing on AVR.
            PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));
            pbio_os_timer_extend(&timer);

            // Get state data from the AVR.
            memset(raw_from_avr, 0, sizeof(raw_from_avr));
            nx__twi_read_async(AVR_ADDRESS, raw_from_avr, sizeof(raw_from_avr));
            PBIO_OS_AWAIT_UNTIL(state, nx__twi_ready());

            // Start over if we fail too many times.
            if (avr_unpack_from_avr() != PBIO_SUCCESS) {
                failed_checksums++;
            }
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

uint32_t nx__avr_get_sensor_value(uint32_t n) {
    NX_ASSERT(n < NXT_N_SENSORS);

    return from_avr.adc_value[n];
}

void nx__avr_set_motor(uint32_t motor, int power_percent, bool brake) {
    NX_ASSERT(motor < NXT_N_MOTORS);

    to_avr.motor_speed[motor] = power_percent;
    if (brake) {
        to_avr.motor_brake |= (1 << motor);
    } else {
        to_avr.motor_brake &= ~(1 << motor);
    }
}

nx_avr_button_t nx_avr_get_button(void) {
    return from_avr.buttons;
}

uint32_t nx_avr_get_battery_voltage(void) {
    return from_avr.battery.charge;
}

bool nx_avr_battery_is_accu_pack(void) {
    return from_avr.battery.is_accu_pack;
}

void nx_avr_get_version(uint8_t *major, uint8_t *minor) {
    if (major) {
        *major = from_avr.version.major;
    }
    if (minor) {
        *minor = from_avr.version.minor;
    }
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
            to_avr.power_mode = AVR_RESET_MODE;
            break;
        case PBDRV_RESET_ACTION_POWER_OFF:
        default:
            to_avr.power_mode = AVR_POWER_OFF;
            break;
    }

    // The main event loop is no longer running, but we do want communication
    // with the AVR to keep going to transmit this command.
    for (;;) {
        pbdrv_rproc_nxt_link_process_thread(&pbdrv_rproc_nxt_link_process.state, NULL);
    }
}

#endif // PBDRV_CONFIG_RPROC_NXT
