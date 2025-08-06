// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// I2C driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/pruss.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <pbdrv/cache.h>
#include <pbdrv/i2c.h>

#include "../drv/rproc/rproc.h"
#include "../rproc/rproc_ev3.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

// Max 255 bytes write, 255 bytes read
// Rounded up to a nice power of 2 and multiple of cache lines
#define PRU_I2C_MAX_BYTES_PER_TXN   512

static uint8_t pbdrv_i2c_buffers[PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES][PRU_I2C_MAX_BYTES_PER_TXN] PBDRV_DMA_BUF;

struct _pbdrv_i2c_dev_t {
    uint8_t *buffer;
    volatile bool is_busy;
    bool is_initialized;
    uint8_t pru_i2c_idx;
};

static pbdrv_i2c_dev_t i2c_devs[PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES];

enum {
    PRU_I2C_PORT1_EVT = 42,
    PRU_I2C_PORT2_EVT = 44,
    PRU_I2C_PORT3_EVT = 46,
    PRU_I2C_PORT4_EVT = 48,
};

pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    if (id >= PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_i2c_dev_t *dev = &i2c_devs[id];
    if (!dev->is_initialized) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    *i2c_dev = dev;
    return PBIO_SUCCESS;
}

static void pbdrv_i2c_irq_0(void) {
    IntSystemStatusClear(SYS_INT_EVTOUT4);
    HWREG(INTC_PHYS_BASE + PRU_INTC_SICR_REG) = PRU_I2C_PORT1_EVT;
    i2c_devs[0].is_busy = false;
    pbio_os_request_poll();
}
static void pbdrv_i2c_irq_1(void) {
    IntSystemStatusClear(SYS_INT_EVTOUT5);
    HWREG(INTC_PHYS_BASE + PRU_INTC_SICR_REG) = PRU_I2C_PORT2_EVT;
    i2c_devs[1].is_busy = false;
    pbio_os_request_poll();
}
static void pbdrv_i2c_irq_2(void) {
    IntSystemStatusClear(SYS_INT_EVTOUT6);
    HWREG(INTC_PHYS_BASE + PRU_INTC_SICR_REG) = PRU_I2C_PORT3_EVT;
    i2c_devs[2].is_busy = false;
    pbio_os_request_poll();
}
static void pbdrv_i2c_irq_3(void) {
    IntSystemStatusClear(SYS_INT_EVTOUT7);
    HWREG(INTC_PHYS_BASE + PRU_INTC_SICR_REG) = PRU_I2C_PORT4_EVT;
    i2c_devs[3].is_busy = false;
    pbio_os_request_poll();
}

pbio_error_t pbdrv_i2c_write_then_read(
    pbio_os_state_t *state,
    pbdrv_i2c_dev_t *i2c_dev,
    uint8_t dev_addr,
    const uint8_t *wdata,
    size_t wlen,
    uint8_t *rdata,
    size_t rlen,
    bool nxt_quirk) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (wlen && !wdata) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (rlen && !rdata) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (wlen > 0xff || rlen > 0xff) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (i2c_dev->is_busy) {
        return PBIO_ERROR_BUSY;
    }

    // Prepare TX data
    if (wlen) {
        memcpy(i2c_dev->buffer, wdata, wlen);
    }
    i2c_dev->is_busy = true;
    pbdrv_cache_prepare_before_dma(i2c_dev->buffer, PRU_I2C_MAX_BYTES_PER_TXN);

    // Kick off transfer
    pbdrv_rproc_ev3_pru1_shared_ram.i2c[i2c_dev->pru_i2c_idx].flags = PBDRV_RPROC_EV3_PRU1_I2C_PACK_FLAGS(
        dev_addr,
        rlen,
        wlen,
        PBDRV_RPROC_EV3_PRU1_I2C_CMD_START | (nxt_quirk ? PBDRV_RPROC_EV3_PRU1_I2C_CMD_NXT_QUIRK : 0)
        );

    // Wait for transfer to finish
    PBIO_OS_AWAIT_WHILE(state, i2c_dev->is_busy);

    uint32_t flags = pbdrv_rproc_ev3_pru1_shared_ram.i2c[i2c_dev->pru_i2c_idx].flags;
    debug_pr("i2c %d done flags %08x\r\n", i2c_dev->pru_i2c_idx, flags);
    if (!(flags & PBDRV_RPROC_EV3_PRU1_I2C_STAT_DONE)) {
        debug_pr("i2c %d not actually done???\r\n", i2c_dev->pru_i2c_idx);
        return PBIO_ERROR_FAILED;
    }
    switch (flags & PBDRV_RPROC_EV3_PRU1_I2C_STAT_MASK) {
        case PBDRV_RPROC_EV3_PRU1_I2C_STAT_OK:
            break;
        case PBDRV_RPROC_EV3_PRU1_I2C_STAT_TIMEOUT:
            return PBIO_ERROR_TIMEDOUT;
        case PBDRV_RPROC_EV3_PRU1_I2C_STAT_NAK:
            return PBIO_ERROR_IO;
        default:
            debug_pr("i2c %d unknown error occurred???\r\n", i2c_dev->pru_i2c_idx);
            return PBIO_ERROR_FAILED;
    }

    // If we got here, there's no error. Copy RX data.
    pbdrv_cache_prepare_after_dma(i2c_dev->buffer, PRU_I2C_MAX_BYTES_PER_TXN);
    if (rlen) {
        memcpy(rdata, &i2c_dev->buffer[wlen], rlen);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t ev3_i2c_init_process;

pbio_error_t ev3_i2c_init_process_thread(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    // Need rproc to be initialized, because it sets up the PRU INTC
    PBIO_OS_AWAIT_UNTIL(state, pbdrv_rproc_is_ready());

    // Set up the buffer pointers
    for (int i = 0; i < PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES; i++) {
        pbdrv_i2c_dev_t *i2c = &i2c_devs[i];
        pbdrv_rproc_ev3_pru1_shared_ram.i2c[i].buffer = (uintptr_t)i2c->buffer;
    }

    // REVISIT: These event numbers get set up by the SUART library.
    // We should separate them cleanly in the future.
    IntRegister(SYS_INT_EVTOUT4, pbdrv_i2c_irq_0);
    IntChannelSet(SYS_INT_EVTOUT4, 2);
    IntSystemEnable(SYS_INT_EVTOUT4);
    HWREG(INTC_PHYS_BASE + PRU_INTC_EISR_REG) = PRU_I2C_PORT1_EVT;

    IntRegister(SYS_INT_EVTOUT5, pbdrv_i2c_irq_1);
    IntChannelSet(SYS_INT_EVTOUT5, 2);
    IntSystemEnable(SYS_INT_EVTOUT5);
    HWREG(INTC_PHYS_BASE + PRU_INTC_EISR_REG) = PRU_I2C_PORT2_EVT;

    IntRegister(SYS_INT_EVTOUT6, pbdrv_i2c_irq_2);
    IntChannelSet(SYS_INT_EVTOUT6, 2);
    IntSystemEnable(SYS_INT_EVTOUT6);
    HWREG(INTC_PHYS_BASE + PRU_INTC_EISR_REG) = PRU_I2C_PORT3_EVT;

    IntRegister(SYS_INT_EVTOUT7, pbdrv_i2c_irq_3);
    IntChannelSet(SYS_INT_EVTOUT7, 2);
    IntSystemEnable(SYS_INT_EVTOUT7);
    HWREG(INTC_PHYS_BASE + PRU_INTC_EISR_REG) = PRU_I2C_PORT4_EVT;

    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_i2c_init(void) {
    for (int i = 0; i < PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES; i++) {
        pbdrv_i2c_dev_t *i2c = &i2c_devs[i];
        i2c->pru_i2c_idx = i;
        i2c->buffer = pbdrv_i2c_buffers[i];
        i2c->is_initialized = true;
    }

    pbio_busy_count_up();
    pbio_os_process_start(&ev3_i2c_init_process, ev3_i2c_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_I2C_EV3
