// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2007 the NxOS developers
// See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
// Copyright (C) leJOS NXJ developers
// Copyright (c) 2025 The Pybricks Authors

// Two Wire Interface driver for the link to the NXT AVR coprocessor.
//
// The AVR is the only device on this bus and no others can be added, so the
// driver is private to the coprocessor driver and hardcodes the address.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC_NXT

#include <stdint.h>

#include <at91sam7s256.h>

#include <pbio/os.h>

#include "nxos/nxt.h"
#include "nxos/interrupts.h"
#include "nxos/drivers/aic.h"
#include "nxos/drivers/systick.h"

#include "rproc_nxt_twi.h"

/** I2C bus address of the AVR coprocessor. */
#define TWI_SLAVE_ADDRESS 1

/** Clock divider for 400 kHz, the rate the AVR is programmed for. */
#define TWI_CLOCK_DIVIDER (((NXT_CLOCK_FREQ / 400000) / 2) - 3)

/**
 * Condition that aborts the transfer in progress.
 *
 * Only a missing acknowledge counts. The controller also raises an underrun
 * once the transmit shifter runs dry, which happens at the end of every write
 * and does not mean anything went wrong.
 */
#define TWI_ERROR_FLAGS (AT91C_TWI_NACK)

static volatile struct {
    /** Status of the current or most recent transfer. */
    pbdrv_rproc_nxt_twi_status_t status;
    /** Next byte to send or receive. */
    uint8_t *ptr;
    /** Bytes still to send or receive. */
    uint32_t len;
    /** Status register bits that the handler should act on right now. */
    uint32_t mask;
    /** Status register as it was when the last transfer was aborted. */
    uint32_t fault_status;
} twi = {
    // Nothing may use the bus until it has been initialized, so report it as
    // occupied rather than free until then.
    .status = PBDRV_RPROC_NXT_TWI_STATUS_BUSY,
};

static void pbdrv_rproc_nxt_twi_isr(void) {

    // Reading the status register acknowledges the interrupt. Masking keeps
    // the handler from acting on bits that belong to a different phase of the
    // transfer, so the phase does not have to be tested here.
    uint32_t sr = *AT91C_TWI_SR;
    uint32_t status = sr & twi.mask;

    if (status & TWI_ERROR_FLAGS) {
        *AT91C_TWI_CR = AT91C_TWI_STOP;
        *AT91C_TWI_IDR = ~0;
        twi.mask = 0;
        twi.fault_status = sr;
        twi.status = PBDRV_RPROC_NXT_TWI_STATUS_ERROR;
        pbio_os_request_poll();
        return;
    }

    if (status & AT91C_TWI_RXRDY) {
        *twi.ptr++ = *AT91C_TWI_RHR;

        // The stop condition has to be requested while the second to last
        // byte is being read, so it goes out with the last one.
        if (--twi.len == 1) {
            *AT91C_TWI_CR = AT91C_TWI_STOP;
        }

        // Unlike a write, this does not wait for the transfer complete flag.
        // The AVR expects the next transfer soon after and finishing slightly
        // early keeps that window as wide as possible.
        if (twi.len == 0) {
            *AT91C_TWI_IDR = ~0;
            twi.mask = 0;
            twi.status = PBDRV_RPROC_NXT_TWI_STATUS_READY;
            pbio_os_request_poll();
        }
        return;
    }

    if (status & AT91C_TWI_TXRDY) {
        if (twi.len) {
            *AT91C_TWI_THR = *twi.ptr++;
            twi.len--;
            return;
        }
        // The last byte has only reached the shift register. The controller
        // sends the stop condition by itself once it has been shifted out, so
        // wait for that instead of declaring the bus free too early.
        *AT91C_TWI_IDR = AT91C_TWI_TXRDY;
        *AT91C_TWI_IER = AT91C_TWI_TXCOMP;
        twi.mask = AT91C_TWI_TXCOMP | TWI_ERROR_FLAGS;
        return;
    }

    if (status & AT91C_TWI_TXCOMP) {
        *AT91C_TWI_IDR = ~0;
        twi.mask = 0;
        twi.status = PBDRV_RPROC_NXT_TWI_STATUS_READY;
        pbio_os_request_poll();
    }
}

/**
 * Resets the bus and the controller.
 *
 * May be called again at any time to recover from a failed transfer.
 */
void pbdrv_rproc_nxt_twi_init(void) {

    uint32_t clocks = 9;

    uint32_t state = nx_interrupts_disable();

    // Power up the TWI and PIO controllers.
    *AT91C_PMC_PCER = (1 << AT91C_ID_TWI) | (1 << AT91C_ID_PIOA);

    *AT91C_TWI_IDR = ~0;
    twi.mask = 0;
    twi.len = 0;
    twi.fault_status = 0;
    twi.status = PBDRV_RPROC_NXT_TWI_STATUS_BUSY;

    // The coprocessor may believe it is in the middle of a transaction, in
    // which case it can be holding the data line low. The TWI controller locks
    // up if it is initialized while either line is low, so take the pins over
    // with the PIO controller and clock the slave until it lets go.
    *AT91C_PIOA_MDER = AT91C_PA3_TWD | AT91C_PA4_TWCK;
    *AT91C_PIOA_PER = AT91C_PA3_TWD | AT91C_PA4_TWCK;
    *AT91C_PIOA_ODR = AT91C_PA3_TWD;
    *AT91C_PIOA_OER = AT91C_PA4_TWCK;

    while (clocks > 0 && !(*AT91C_PIOA_PDSR & AT91C_PA3_TWD)) {
        *AT91C_PIOA_CODR = AT91C_PA4_TWCK;
        nx_systick_wait_ns(1500);
        *AT91C_PIOA_SODR = AT91C_PA4_TWCK;
        nx_systick_wait_ns(1500);
        clocks--;
    }

    // Hand the now clean lines back to the TWI controller.
    *AT91C_PIOA_PDR = AT91C_PA3_TWD | AT91C_PA4_TWCK;
    *AT91C_PIOA_ASR = AT91C_PA3_TWD | AT91C_PA4_TWCK;

    *AT91C_TWI_CR = AT91C_TWI_SWRST | AT91C_TWI_MSDIS;
    *AT91C_TWI_CWGR = (TWI_CLOCK_DIVIDER << 8) | TWI_CLOCK_DIVIDER;
    *AT91C_TWI_CR = AT91C_TWI_MSEN;

    // Runs above everything but the system tick: a late handler means a byte
    // is missed, which breaks the link and with it the power supply.
    nx_aic_install_isr(AT91C_ID_TWI, AIC_PRIO_RT, AIC_TRIG_LEVEL, pbdrv_rproc_nxt_twi_isr);

    twi.status = PBDRV_RPROC_NXT_TWI_STATUS_READY;

    nx_interrupts_enable(state);
}

/**
 * Starts sending @p len bytes from @p data to the coprocessor.
 *
 * @param [in]  data  Data to send. Must stay valid until the transfer ends.
 * @param [in]  len   Number of bytes to send. Must be at least two.
 */
void pbdrv_rproc_nxt_twi_write(const uint8_t *data, uint32_t len) {

    uint32_t state = nx_interrupts_disable();

    twi.status = PBDRV_RPROC_NXT_TWI_STATUS_BUSY;
    twi.ptr = (uint8_t *)data;
    twi.len = len;
    twi.mask = AT91C_TWI_TXRDY | TWI_ERROR_FLAGS;

    *AT91C_TWI_MMR = AT91C_TWI_IADRSZ_NO | (TWI_SLAVE_ADDRESS << 16);

    // Writing the holding register is what generates the start condition.
    *AT91C_TWI_THR = *twi.ptr++;
    twi.len--;

    *AT91C_TWI_IER = AT91C_TWI_TXRDY | TWI_ERROR_FLAGS;

    nx_interrupts_enable(state);
}

/**
 * Starts receiving @p len bytes from the coprocessor into @p data.
 *
 * @param [out] data  Buffer to receive into.
 * @param [in]  len   Number of bytes to receive. Must be at least two, since
 *                    a single byte read needs the stop condition to go out
 *                    with the start condition, which is not implemented.
 */
void pbdrv_rproc_nxt_twi_read(uint8_t *data, uint32_t len) {

    uint32_t state = nx_interrupts_disable();

    twi.status = PBDRV_RPROC_NXT_TWI_STATUS_BUSY;
    twi.ptr = data;
    twi.len = len;
    twi.mask = AT91C_TWI_RXRDY | TWI_ERROR_FLAGS;

    *AT91C_TWI_MMR = AT91C_TWI_IADRSZ_NO | AT91C_TWI_MREAD | (TWI_SLAVE_ADDRESS << 16);
    *AT91C_TWI_CR = AT91C_TWI_START;
    *AT91C_TWI_IER = AT91C_TWI_RXRDY | TWI_ERROR_FLAGS;

    nx_interrupts_enable(state);
}

/**
 * Gets the state of the transfer most recently started.
 */
pbdrv_rproc_nxt_twi_status_t pbdrv_rproc_nxt_twi_get_status(void) {
    return twi.status;
}

/**
 * Gets the status register saved when the last transfer was aborted, and how
 * many bytes the current or most recent transfer still has outstanding.
 */
void pbdrv_rproc_nxt_twi_get_fault_info(uint32_t *fault_status, uint32_t *bytes_remaining) {
    *fault_status = twi.fault_status;
    *bytes_remaining = twi.len;
}

#endif // PBDRV_CONFIG_RPROC_NXT
