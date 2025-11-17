// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// UART driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <lwrb/lwrb.h>

#include <pbdrv/cache.h>
#include <pbdrv/uart.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include "./uart_ev3.h"
#include "./uart_ev3_pru.h"

#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/edma.h>
#include <tiam1808/hw/hw_edma3cc.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/psc.h>
#include <tiam1808/uart.h>

/**
 * This file contains two almost entirely separate implementations of UART
 * drivers. These could in principle be split into separate modules with
 * appropriate priv_data structures, but this requires generalizing the UART
 * drivers on all platforms as well. For now, we keep them in the same file
 * and use the "type" field in pdata to distinguish which read/write/init/irq
 * functions to use.
 */

#define RX_DATA_SIZE 64 // must be power of 2 for ring buffer!

struct _pbdrv_uart_dev_t {
    /** Platform-specific data */
    const pbdrv_uart_ev3_platform_data_t *pdata;
    /** Circular buffer for caching received bytes. */
    lwrb_t rx_buf;
    /** Timer for read timeout. */
    pbio_os_timer_t read_timer;
    /** Timer for write timeout. */
    pbio_os_timer_t write_timer;
    /** The buffer passed to the read function. */
    uint8_t *read_buf;
    /** The length of read_buf in bytes. */
    uint32_t read_length;
    /** The current position in read_buf. */
    uint32_t read_pos;
    /** The buffer passed to the write function. */
    const uint8_t *write_buf;
    /** The length of write_buf in bytes. */
    uint32_t write_length;
    /** The current position in write_buf. */
    volatile uint32_t write_pos;
};

static pbdrv_uart_dev_t uart_devs[PBDRV_CONFIG_UART_EV3_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_EV3_NUM_UART][RX_DATA_SIZE];

pbio_error_t pbdrv_uart_get_instance(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_EV3_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_uart_dev_t *dev = &uart_devs[id];
    if (!dev->pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    *uart_dev = dev;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, uint8_t *msg, uint32_t length, uint32_t timeout) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (uart->read_buf) {
        return PBIO_ERROR_BUSY;
    }

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    if (timeout) {
        pbio_os_timer_set(&uart->read_timer, timeout);
    }

    // Await completion or timeout.
    PBIO_OS_AWAIT_UNTIL(state, ({
        // On every re-entry to the async read, drain the ring buffer
        // into the current read buffer. This ensures that we use
        // all available data if there have been multiple polls since our last
        // re-entry. If there is already enough data in the buffer, this
        // protothread completes right away without yielding once first.
        uart->read_pos += lwrb_read(&uart->rx_buf, &uart->read_buf[uart->read_pos], uart->read_length - uart->read_pos);
        uart->read_pos == uart->read_length || (timeout && pbio_os_timer_is_expired(&uart->read_timer));
    }));

    uart->read_buf = NULL;

    if (timeout && pbio_os_timer_is_expired(&uart->read_timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t pbdrv_uart_write_pru(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, const uint8_t *msg, uint32_t length, uint32_t timeout) {

    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    PBIO_OS_ASYNC_BEGIN(state);

    // Can only write one thing at once.
    if (uart->write_buf) {
        return PBIO_ERROR_BUSY;
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    if (timeout) {
        pbio_os_timer_set(&uart->write_timer, timeout);
    }

    // Write one byte at a time until all bytes are written.
    PBIO_OS_AWAIT_UNTIL(state, ({
        // Try to write one byte if any are remaining.
        if (uart->write_pos < uart->write_length) {
            // Attempt to write one byte with the PRU UART. We could be sending
            // more than one byte at a time, but we keep it consistent with the
            // current PRU API. EV3 sensors don't send much data anyway.
            if (pbdrv_uart_ev3_pru_write_bytes(pdata->peripheral_id, &uart->write_buf[uart->write_pos], 1)) {
                uart->write_pos++;
            }
        }
        // Completion on transmission of whole message and finishing writing, or timeout.
        bool complete = pbdrv_uart_ev3_pru_can_write(pdata->peripheral_id) && uart->write_pos == uart->write_length;
        bool expired = timeout && pbio_os_timer_is_expired(&uart->write_timer);

        // Await until complete or timed out.
        complete || expired;
    }));

    uart->write_buf = NULL;

    if (timeout && pbio_os_timer_is_expired(&uart->write_timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_uart_write_hw(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, const uint8_t *msg, uint32_t length, uint32_t timeout) {

    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    PBIO_OS_ASYNC_BEGIN(state);

    // Can only write one thing at once.
    if (uart->write_buf) {
        return PBIO_ERROR_BUSY;
    }

    uart->write_buf = msg;

    // Write length and pos properties not used in this implementation.
    uart->write_length = 0;
    uart->write_pos = 0;

    if (timeout) {
        pbio_os_timer_set(&uart->write_timer, timeout);
    }

    volatile EDMA3CCPaRAMEntry paramSet = {
        .srcAddr = (uint32_t)uart->write_buf,
        // UART transmit register address.
        .destAddr = pdata->base_address + UART_THR,
        // Number of bytes in an array.
        .aCnt = 1,
        // Number of such arrays to be transferred.
        .bCnt = length,
        // Number of frames of aCnt*bBcnt bytes to be transferred.
        .cCnt = 1,
        // The src index should increment for every byte being transferred.
        .srcBIdx = 1,
        // The dst index should not increment since it is a h/w register.
        .destBIdx = 0,
        // Transfer mode.
        .srcCIdx = 0,
        .destCIdx = 0,
        .linkAddr = 0xFFFF,
        .bCntReload = 0,
        .opt = EDMA3CC_OPT_DAM | ((pdata->sys_int_uart_tx_int_id << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC) | (1 << EDMA3CC_OPT_TCINTEN_SHIFT),
    };

    pbdrv_cache_prepare_before_dma(uart->write_buf, length);

    // Save configuration and start transfer.
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, pdata->sys_int_uart_tx_int_id, (EDMA3CCPaRAMEntry *)&paramSet);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, pdata->sys_int_uart_tx_int_id, EDMA3_TRIG_MODE_EVENT);
    UARTDMAEnable(pdata->base_address, UART_RX_TRIG_LEVEL_1 | UART_DMAMODE | UART_FIFO_MODE);

    // Await until all bytes are written or timeout reached.
    PBIO_OS_AWAIT_UNTIL(state, !uart->write_buf || (timeout && pbio_os_timer_is_expired(&uart->write_timer)));

    uart->write_buf = NULL;

    if (timeout && pbio_os_timer_is_expired(&uart->write_timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_uart_write(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, const uint8_t *msg, uint32_t length, uint32_t timeout) {
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    if (pdata->uart_kind == EV3_UART_HW) {
        return pbdrv_uart_write_hw(state, uart, msg, length, timeout);
    } else {
        return pbdrv_uart_write_pru(state, uart, msg, length, timeout);
    }
}

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud) {
    if (uart->pdata->uart_kind == EV3_UART_HW) {
        UARTConfigSetExpClk(uart->pdata->base_address, SOC_UART_0_MODULE_FREQ, baud, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
    } else {
        pbdrv_uart_ev3_pru_set_baudrate(uart->pdata->peripheral_id, baud);
    }
}


void pbdrv_uart_flush(pbdrv_uart_dev_t *uart) {
    // If a process was exited while an operation was in progress this is
    // normally an error, and the process may call flush when it is restarted
    // to clear the state.
    uart->write_buf = NULL;
    uart->write_length = 0;
    uart->write_pos = 0;
    uart->read_buf = NULL;
    uart->read_length = 0;
    uart->read_pos = 0;
    // Discard all received bytes.
    lwrb_reset(&uart->rx_buf);
}

/**
 * Handles RX interrupts for the hardware UART.
 *
 * @param [in] uart The UART device.
 */
void pbdrv_uart_ev3_hw_handle_irq(pbdrv_uart_dev_t *uart) {
    /* Clears the system interrupt status of UART in AINTC. */
    IntSystemStatusClear(uart->pdata->sys_int_uart_rx_int_id);

    // Repeatedly attempt to handle all the data which we might have
    while ((HWREG(uart->pdata->base_address + UART_IIR) & UART_IIR_IPEND) == 0) {
        // We always have *a* character in the FIFO, even if it might be an error
        int c = UARTCharGetNonBlocking(uart->pdata->base_address);
        unsigned int err = UARTRxErrorGet(uart->pdata->base_address);

        // If there is an overrun, the data we do have is nonetheless valid.
        // We don't report overruns, so just ignore the flag
        // (reading the LSR register automatically clears it in the hardware side).
        err &= ~UART_OVERRUN_ERROR;

        if (c != -1 && !err) {
            // Push valid characters into the ring buffer
            uint8_t rx = c;
            lwrb_write(&uart->rx_buf, &rx, 1);
        }
    }

    // Poll parent process for each received byte, since the IRQ handler
    // has no awareness of the expected length of the read operation. This is
    // done outside of the if statements above. We can do that since write IRQs
    // are not handled here.
    pbio_os_request_poll();

}

void pbdrv_uart_ev3_pru_handle_irq(pbdrv_uart_dev_t *uart) {

    // rx and tx have the same interrupt ID.
    IntSystemStatusClear(uart->pdata->sys_int_uart_rx_int_id);

    // This calls what is mostly equivalent the original LEGO/ev3dev IRQ
    // handler. This is using its own ring buffer that we pull data from below.
    // REVISIT: Pass in our ringbuffer so it can be filled directly.
    pbdrv_uart_ev3_pru_handle_irq_data(uart->pdata->peripheral_id);

    uint8_t rx;
    while (pbdrv_uart_ev3_pru_read_bytes(uart->pdata->peripheral_id, &rx, 1)) {
        lwrb_write(&uart->rx_buf, &rx, 1);
    }
    pbio_os_request_poll();
}

void pbdrv_uart_ev3_handle_irq(uint8_t id) {
    pbdrv_uart_dev_t *uart = &uart_devs[id];
    if (uart->pdata->uart_kind == EV3_UART_HW) {
        pbdrv_uart_ev3_hw_handle_irq(uart);
    } else {
        pbdrv_uart_ev3_pru_handle_irq(uart);
    }
}

void pbdrv_uart_ev3_handle_tx_complete(uint8_t id) {
    pbdrv_uart_dev_t *uart = &uart_devs[id];
    UARTDMADisable(uart->pdata->base_address, (UART_RX_TRIG_LEVEL_1 | UART_FIFO_MODE));
    uart->write_buf = NULL;
    pbio_os_request_poll();
}

static void pbdrv_uart_init_hw(pbdrv_uart_dev_t *uart) {
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    // Enabling the transmitter and receiver.
    UARTEnable(pdata->base_address);

    // Configuring the UART clock and baud parameters.
    pbdrv_uart_set_baud_rate(uart, BAUD_115200);

    // Enabling the FIFO and flushing the Tx and Rx FIFOs.
    UARTFIFOEnable(pdata->base_address);

    // Setting the UART Receiver Trigger Level.
    UARTFIFOLevelSet(pdata->base_address, UART_RX_TRIG_LEVEL_1);

    // Registers the UARTIsr for reading in the Interrupt Vector Table of AINTC.
    // Map the channel number 2 of AINTC to UART system interrupt.
    // Interrupts for reading stay enabled, unlike writing.
    IntRegister(pdata->sys_int_uart_rx_int_id, pdata->isr_handler);
    IntChannelSet(pdata->sys_int_uart_rx_int_id, 2);
    IntSystemEnable(pdata->sys_int_uart_rx_int_id);
    UARTIntEnable(pdata->base_address, UART_INT_LINE_STAT | UART_INT_RXDATA_CTI);

    // Request DMA Channel for UART Transmit at event queue 0.
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, pdata->sys_int_uart_tx_int_id, pdata->sys_int_uart_tx_int_id, 0);
}


void pbdrv_uart_init_pru(pbdrv_uart_dev_t *uart) {
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    pbdrv_uart_ev3_pru_activate(pdata->peripheral_id);
    pbdrv_uart_set_baud_rate(uart, BAUD_115200);

    // Registers the UARTIsr in the Interrupt Vector Table of AINTC.
    // Map the channel number 2 of AINTC to UART system interrupt.
    // One handler for both RX and TX interrupts with same ID.
    IntRegister(pdata->sys_int_uart_tx_int_id, pdata->isr_handler);
    IntChannelSet(pdata->sys_int_uart_tx_int_id, 2);
    IntSystemEnable(pdata->sys_int_uart_tx_int_id);
}

void pbdrv_uart_init(void) {
    // Enabling the PSC for all UARTs
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_UART0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_UART1, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    for (int i = 0; i < PBDRV_CONFIG_UART_EV3_NUM_UART; i++) {
        const pbdrv_uart_ev3_platform_data_t *pdata = &pbdrv_uart_ev3_platform_data[i];
        uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_dev_t *uart = &uart_devs[i];
        uart->pdata = pdata;
        lwrb_init(&uart->rx_buf, rx_data, RX_DATA_SIZE);

        // Initialize the peripheral depending on the uart kind.
        if (pdata->uart_kind == EV3_UART_HW) {
            pbdrv_uart_init_hw(uart);
        } else if (pdata->uart_kind == EV3_UART_PRU) {
            pbdrv_uart_init_pru(uart);
        }
    }
}

#endif // PBDRV_CONFIG_UART_EV3
