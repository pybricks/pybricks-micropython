// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// UART driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki.h>
#include <contiki-lib.h>

#include <pbdrv/uart.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "../core.h"
#include "./uart_ev3.h"
#include "./uart_ev3_pru.h"

#include <tiam1808/uart.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

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
    struct ringbuf rx_buf;
    /** Timer for read timeout. */
    struct etimer read_timer;
    /** Timer for write timeout. */
    struct etimer write_timer;
    /** The buffer passed to the read function. */
    uint8_t *read_buf;
    /** The length of read_buf in bytes. */
    uint8_t read_length;
    /** The current position in read_buf. */
    uint8_t read_pos;
    /** The buffer passed to the write function. */
    uint8_t *write_buf;
    /** The length of write_buf in bytes. */
    uint8_t write_length;
    /** The current position in write_buf. */
    volatile uint8_t write_pos;
    /**
     * Parent process that handles incoming data.
     *
     * All protothreads in this module run within that process.
     */
    struct process *parent_process;
};

static pbdrv_uart_dev_t uart_devs[PBDRV_CONFIG_UART_EV3_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_EV3_NUM_UART][RX_DATA_SIZE];

pbio_error_t pbdrv_uart_get_instance(uint8_t id, struct process *parent_process, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_EV3_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_uart_dev_t *dev = &uart_devs[id];
    if (!dev->pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    dev->parent_process = parent_process;
    *uart_dev = dev;
    return PBIO_SUCCESS;
}

int32_t pbdrv_uart_get_char(pbdrv_uart_dev_t *uart) {
    return ringbuf_get(&uart->rx_buf);
}

PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    PT_BEGIN(pt);

    if (uart->read_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    if (timeout) {
        etimer_set(&uart->read_timer, timeout);
    }

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, ({
        // On every re-entry to the async read, drain the ring buffer
        // into the current read buffer. This ensures that we use
        // all available data if there have been multiple polls since our last
        // re-entry. If there is already enough data in the buffer, this
        // protothread completes right away without yielding once first.
        while (uart->read_pos < uart->read_length) {
            int c = ringbuf_get(&uart->rx_buf);
            if (c == -1) {
                break;
            }
            uart->read_buf[uart->read_pos++] = c;
        }
        uart->read_pos == uart->read_length || (timeout && etimer_expired(&uart->read_timer));
    }));

    // Set exit status based on completion condition.
    if ((timeout && etimer_expired(&uart->read_timer))) {
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->read_timer);
        *err = PBIO_SUCCESS;
    }
    uart->read_buf = NULL;

    PT_END(pt);
}

static PT_THREAD(pbdrv_uart_ev3_hw_write(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {
    PT_BEGIN(pt);

    // Can only write one thing at once.
    if (uart->write_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    etimer_set(&uart->write_timer, timeout);

    UARTIntEnable(uart->pdata->base_address, UART_INT_TX_EMPTY);

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, uart->write_pos == uart->write_length || etimer_expired(&uart->write_timer));

    uart->write_buf = NULL;

    if (etimer_expired(&uart->write_timer)) {
        UARTIntDisable(uart->pdata->base_address, UART_INT_TX_EMPTY);
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->write_timer);
        *err = PBIO_SUCCESS;
    }

    PT_END(pt);
}

static PT_THREAD(pbdrv_uart_ev3_pru_write(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {
    PT_BEGIN(pt);

    // Can only write one thing at once.
    if (uart->write_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    if (timeout) {
        etimer_set(&uart->write_timer, timeout);
    }

    // Write one byte at a time until all bytes are written.
    // REVISIT: The PRU API supports sending longer messages. Need to figure
    // out how to set up completion interrupts for longer messages.
    PT_WAIT_UNTIL(pt, ({
        // Try to write one byte if any are remaining.
        if (uart->write_pos < uart->write_length) {
            if (pbdrv_uart_ev3_pru_write_bytes(uart->pdata->peripheral_id, &uart->write_buf[uart->write_pos], 1)) {
                uart->write_pos++;
            }
        }

        // Completion on transmission of whole message and finishing writing or timeout.
        (pbdrv_uart_ev3_pru_can_write(uart->pdata->peripheral_id) && uart->write_pos == uart->write_length) || (timeout && etimer_expired(&uart->write_timer));
    }));
    uart->write_buf = NULL;

    if ((timeout && etimer_expired(&uart->write_timer))) {
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->write_timer);
        *err = PBIO_SUCCESS;
    }

    PT_END(pt);
}

PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {
    if (uart->pdata->uart_kind == EV3_UART_HW) {
        return pbdrv_uart_ev3_hw_write(pt, uart, msg, length, timeout, err);
    } else {
        return pbdrv_uart_ev3_pru_write(pt, uart, msg, length, timeout, err);
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
    while (ringbuf_get(&uart->rx_buf) != -1) {
        ;
    }
}

/**
 * Handle an interrupt from the hardware UART.
 */
void pbdrv_uart_ev3_hw_handle_irq(pbdrv_uart_dev_t *uart) {

    // This determines the cause of UART0 interrupt.
    unsigned int int_id = UARTIntStatus(uart->pdata->base_address);

    // Clears the system interrupt status of UART in AINTC.
    IntSystemStatusClear(uart->pdata->sys_int_uart_int_id);

    // Check if the cause is receiver data condition.
    if (UART_INTID_RX_DATA == (int_id & UART_INTID_RX_DATA) || (UART_INTID_CTI == (int_id & UART_INTID_CTI))) {
        int c = UARTCharGetNonBlocking(uart->pdata->base_address);
        if (c != -1) {
            ringbuf_put(&uart->rx_buf, c);
        }
        // Poll parent process for each received byte, since the IRQ handler
        // has no awareness of the expected length of the read operation.
        process_poll(uart->parent_process);
    }

    // Check if the cause is receiver line error condition.
    if (UART_INTID_RX_LINE_STAT == (int_id & UART_INTID_RX_LINE_STAT)) {
        while (UARTRxErrorGet(uart->pdata->base_address)) {
            /* Read a byte from the RBR if RBR has data.*/
            UARTCharGetNonBlocking(uart->pdata->base_address);
        }
        process_poll(uart->parent_process);
    }

    // Check if the cause is transmitter empty condition.
    if (UART_INTID_TX_EMPTY == (int_id & UART_INTID_TX_EMPTY) && uart->write_buf) {
        // Write the next byte.
        HWREG(uart->pdata->base_address + UART_THR) = uart->write_buf[uart->write_pos++];
        // No need to poll on each byte; only on completion.
        if (uart->write_pos == uart->write_length) {
            UARTIntDisable(uart->pdata->base_address, UART_INT_TX_EMPTY);
            process_poll(uart->parent_process);
        }
    }
}

void pbdrv_uart_ev3_pru_handle_irq(pbdrv_uart_dev_t *uart) {

    IntSystemStatusClear(uart->pdata->sys_int_uart_int_id);

    // This calls what is mostly equivalent the original LEGO/ev3dev IRQ
    // handler. This is using its own ring buffer that we pull data from below.
    // REVISIT: Pass in our ringbuffer so it can be filled directly.
    pbdrv_uart_ev3_pru_handle_irq_data(uart->pdata->peripheral_id);

    uint8_t rx;
    while (pbdrv_uart_ev3_pru_read_bytes(uart->pdata->peripheral_id, &rx, 1)) {
        ringbuf_put(&uart->rx_buf, rx);
    }
    process_poll(uart->parent_process);
}

void pbdrv_uart_ev3_handle_irq(uint8_t id) {
    pbdrv_uart_dev_t *uart = &uart_devs[id];
    if (uart->pdata->uart_kind == EV3_UART_HW) {
        pbdrv_uart_ev3_hw_handle_irq(uart);
    } else {
        pbdrv_uart_ev3_pru_handle_irq(uart);
    }
}

static void pbdrv_uart_init_hw(pbdrv_uart_dev_t *uart) {
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    /* Enabling the PSC for given UART.*/
    PSCModuleControl(SOC_PSC_1_REGS, pdata->peripheral_id, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    /* Enabling the transmitter and receiver*/
    UARTEnable(pdata->base_address);

    /* Configuring the UART clock and baud parameters*/
    pbdrv_uart_set_baud_rate(uart, BAUD_115200);

    /* Enabling the FIFO and flushing the Tx and Rx FIFOs.*/
    UARTFIFOEnable(pdata->base_address);

    /* Setting the UART Receiver Trigger Level*/
    UARTFIFOLevelSet(pdata->base_address, UART_RX_TRIG_LEVEL_1);

    /* Registers the UARTIsr in the Interrupt Vector Table of AINTC. */
    IntRegister(pdata->sys_int_uart_int_id, pdata->isr_handler);

    /* Map the channel number 2 of AINTC to UART system interrupt. */
    IntChannelSet(pdata->sys_int_uart_int_id, 2);

    IntSystemEnable(pdata->sys_int_uart_int_id);

    /* Enable the Interrupts in UART.*/
    UARTIntEnable(pdata->base_address, UART_INT_LINE_STAT | UART_INT_RXDATA_CTI);
}


void pbdrv_uart_init_pru(pbdrv_uart_dev_t *uart) {
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    pbdrv_uart_ev3_pru_activate(pdata->peripheral_id);
    pbdrv_uart_set_baud_rate(uart, BAUD_115200);

    // Registers the UARTIsr in the Interrupt Vector Table of AINTC.
    IntRegister(pdata->sys_int_uart_int_id, pdata->isr_handler);

    // Map the channel number 2 of AINTC to UART system interrupt.
    IntChannelSet(pdata->sys_int_uart_int_id, 2);

    IntSystemEnable(pdata->sys_int_uart_int_id);
}

void pbdrv_uart_init(void) {

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_PRU, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    extern uint8_t _pru0_start;
    extern uint8_t _pru0_end;
    uint32_t fw_size = &_pru0_end - &_pru0_start;
    uint8_t *fw_data = &_pru0_start;
    pbdrv_uart_ev3_pru_load_firmware(fw_data, fw_size);

    for (int i = 0; i < PBDRV_CONFIG_UART_EV3_NUM_UART; i++) {
        const pbdrv_uart_ev3_platform_data_t *pdata = &pbdrv_uart_ev3_platform_data[i];
        uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_dev_t *uart = &uart_devs[i];
        uart->pdata = pdata;
        ringbuf_init(&uart->rx_buf, rx_data, RX_DATA_SIZE);

        // Initialize the peripheral depending on the uart kind.
        if (pdata->uart_kind == EV3_UART_HW) {
            pbdrv_uart_init_hw(uart);
        } else if (pdata->uart_kind == EV3_UART_PRU) {
            pbdrv_uart_init_pru(uart);
        }
    }
}

#endif // PBDRV_CONFIG_UART_EV3
