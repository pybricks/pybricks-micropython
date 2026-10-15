// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// I2C driver for NXT
//
// The sensor ports have no I2C peripheral, so the nxos SoftMAC driver bit bangs
// the bus. This translates its asynchronous transaction API to pbdrv.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>

#include <pbdrv/i2c.h>

#include <pbio/error.h>
#include <pbio/os.h>

#include "i2c.h"

#include "nxos/nxt.h"
#include "nxos/drivers/i2c.h"
#include "nxos/drivers/_sensors.h"

/** A bit banged transfer of a few bytes at 9.6 kHz takes a handful of ms. */
#define PBDRV_I2C_NXT_TIMEOUT_MS 500

/**
 * Minimum bus idle time before a START. The ultrasonic sensor doesn't
 * acknowledge its address otherwise (3 ms fails, 5 ms works). The nxos
 * i2c_memory.c helpers got this implicitly by polling every 10 ms.
 */
#define PBDRV_I2C_NXT_IDLE_MS 10

/**
 * ADC channel wired to pin 6 of each sensor port, which is also the I2C data
 * line. Must match the adc_p6 values in the platform data.
 */
static const uint8_t pbdrv_i2c_nxt_pin6_adc_channel[NXT_N_SENSORS] = { 1, 2, 3, 7 };

struct _pbdrv_i2c_dev_t {
    /** Sensor port this bus belongs to. */
    uint8_t port_index;
    /** Whether a device is currently registered on this port. */
    bool registered;
    /** Expires once the bus has been idle for PBDRV_I2C_NXT_IDLE_MS. */
    pbio_os_timer_t idle_timer;
    /** Holds data written to the device. */
    uint8_t write_buf[I2C_MAX_DATA_SIZE];
    /** Holds data read back from the device. */
    uint8_t read_buf[I2C_MAX_DATA_SIZE];
};

static pbdrv_i2c_dev_t i2c_devs[NXT_N_SENSORS];

/** Gives pin 6 back to the ADC after a transfer. */
static void pbdrv_i2c_nxt_release_pins(pbdrv_i2c_dev_t *i2c_dev) {
    *AT91C_ADC_CHER = 1 << pbdrv_i2c_nxt_pin6_adc_channel[i2c_dev->port_index];
}

pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    if (id >= NXT_N_SENSORS) {
        return PBIO_ERROR_INVALID_ARG;
    }
    *i2c_dev = &i2c_devs[id];
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_i2c_write_then_read(
    pbio_os_state_t *state,
    pbdrv_i2c_dev_t *i2c_dev,
    uint8_t dev_addr,
    const uint8_t *wdata,
    size_t wlen,
    uint8_t **rdata,
    size_t rlen,
    bool nxt_quirk) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    if (wlen > I2C_MAX_DATA_SIZE || rlen > I2C_MAX_DATA_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (wlen && !wdata) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Copy the write data, since the caller only provides it on the first call
    // but the SoftMAC driver needs it until the transfer completes.
    memcpy(i2c_dev->write_buf, wdata, wlen);

    // Restart the idle time if SCL isn't driven high (e.g. the pins were
    // released as inputs). Between transfers SCL stays high, so the time since
    // the previous transfer counts.
    {
        const nx__sensors_pins *pins = nx__sensors_get_pins(i2c_dev->port_index);
        if (!(*AT91C_PIOA_OSR & *AT91C_PIOA_ODSR & pins->scl)) {
            pbio_os_timer_set(&i2c_dev->idle_timer, PBDRV_I2C_NXT_IDLE_MS);
        }
    }

    // Re-register for every transfer to reset the port state, or the next
    // transfer returns stale data without touching the bus. Registering
    // asserts the port is free, hence the unregister.
    if (i2c_dev->registered) {
        nx_i2c_unregister(i2c_dev->port_index);
    }
    nx_i2c_register(i2c_dev->port_index, dev_addr, nxt_quirk);
    i2c_dev->registered = true;

    // Take pin 6 from the ADC for the transfer (SDA can't be pulled low while
    // it's an analog input). Turn the internal pull-ups off, as in LEGO's
    // driver (sensors provide them, and with them on, the ultrasonic doesn't
    // acknowledge on port 4).
    {
        const nx__sensors_pins *pins = nx__sensors_get_pins(i2c_dev->port_index);
        *AT91C_ADC_CHDR = 1 << pbdrv_i2c_nxt_pin6_adc_channel[i2c_dev->port_index];
        *AT91C_PIOA_PER = pins->scl | pins->sda;
        *AT91C_PIOA_PPUDR = pins->scl | pins->sda;
    }

    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&i2c_dev->idle_timer));

    pbio_os_timer_set(&timer, PBDRV_I2C_NXT_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, !nx_i2c_busy(i2c_dev->port_index) || pbio_os_timer_is_expired(&timer));
    if (nx_i2c_busy(i2c_dev->port_index)) {
        pbdrv_i2c_nxt_release_pins(i2c_dev);
        return PBIO_ERROR_TIMEDOUT;
    }

    // Zero the receive buffer, since the SoftMAC driver ORs received bits in.
    memset(i2c_dev->read_buf, 0, rlen);

    if (nx_i2c_start_transaction(i2c_dev->port_index, rlen ? TXN_MODE_READ : TXN_MODE_WRITE,
        i2c_dev->write_buf, wlen, i2c_dev->read_buf, rlen) != I2C_ERR_OK) {
        pbdrv_i2c_nxt_release_pins(i2c_dev);
        return PBIO_ERROR_FAILED;
    }

    pbio_os_timer_set(&timer, PBDRV_I2C_NXT_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, !nx_i2c_busy(i2c_dev->port_index) || pbio_os_timer_is_expired(&timer));

    pbio_os_timer_set(&i2c_dev->idle_timer, PBDRV_I2C_NXT_IDLE_MS);
    pbdrv_i2c_nxt_release_pins(i2c_dev);

    if (nx_i2c_busy(i2c_dev->port_index)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    if (nx_i2c_get_txn_status(i2c_dev->port_index) != TXN_STAT_SUCCESS) {
        return PBIO_ERROR_IO;
    }

    if (rlen) {
        *rdata = i2c_dev->read_buf;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_i2c_init(void) {
    for (uint8_t i = 0; i < NXT_N_SENSORS; i++) {
        i2c_devs[i].port_index = i;
    }

    nx_i2c_init();
}

#endif // PBDRV_CONFIG_I2C_NXT
