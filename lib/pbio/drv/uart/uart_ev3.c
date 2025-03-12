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

#include <tiam1808/uart.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>


#define RX_DATA_SIZE 64 // must be power of 2 for ring buffer!

typedef struct {
    /** Public UART device handle. */
    pbdrv_uart_dev_t uart_dev;
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
    /** Callback to call on read or write completion events */
    pbdrv_uart_poll_callback_t poll_callback;
    /** Context for callback caller */
    void *poll_callback_context;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_EV3_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_EV3_NUM_UART][RX_DATA_SIZE];

void pbdrv_uart_set_poll_callback(pbdrv_uart_dev_t *uart_dev, pbdrv_uart_poll_callback_t callback, void *context) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    uart->poll_callback = callback;
    uart->poll_callback_context = context;
}

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_EV3_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbdrv_uart[id].pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

// Revisit: Following two functions not currently part of public API, but
// would be useful addition to async read.
int32_t pbdrv_uart_char_get(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    return ringbuf_get(&uart->rx_buf);
}

int32_t pbdrv_uart_char_available(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    return ringbuf_elements(&uart->rx_buf);
}

PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    PT_BEGIN(pt);

    // Wait while other read operation already in progress.
    PT_WAIT_WHILE(pt, uart->read_buf);

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    // etimer_set(&uart->read_timer, timeout);

    // If read_pos is less that read_length then we have not read everything yet
    PT_WAIT_WHILE(pt, uart->read_pos < uart->read_length /*&& !etimer_expired(&uart->read_timer)*/);
    if (0 /*etimer_expired(&uart->read_timer)*/) {
        uart->read_buf = NULL;
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        // etimer_stop(&uart->read_timer);
        *err = PBIO_SUCCESS;
    }

    PT_END(pt);
}

PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    PT_BEGIN(pt);

    // Can only write one thing at once.
    if (uart->write_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    if (length == 0) {
        *err = PBIO_SUCCESS;
        PT_EXIT(pt);
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    etimer_set(&uart->write_timer, timeout);

    PT_WAIT_UNTIL(pt, ({
        // Try to write a byte.
        if (UARTCharPutNonBlocking(pdata->base_address, uart->write_buf[uart->write_pos])) {
            uart->write_pos++;
        }
        // Always callback, which effectively enforces a poll loop for writing.
        // This works around the unreliable TX_EMPTY interrupt. Since UART writes
        // on EV3 are limited to small messages like sensor mode changes, this
        // is acceptable.
        if (uart->poll_callback && uart->write_pos < uart->write_length) {
            uart->poll_callback(uart->poll_callback_context);
        }
        // Completion condition.
        uart->write_pos == uart->write_length;
    }));

    uart->write_buf = NULL;

    if (etimer_expired(&uart->write_timer)) {
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->write_timer);
        *err = PBIO_SUCCESS;
    }

    PT_END(pt);
}

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    UARTConfigSetExpClk(uart->pdata->base_address, SOC_UART_0_MODULE_FREQ, baud, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
}

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
}

/**
 * Updates the state of the awaitable read operation when a new byte is received.
 */
static void pbdrv_uart_ev3_update_read_status(pbdrv_uart_t *uart) {
    // if receive is pending and we have not received all bytes yet
    while (uart->read_buf && uart->read_pos < uart->read_length) {
        int c = ringbuf_get(&uart->rx_buf);
        if (c == -1) {
            break;
        }
        uart->read_buf[uart->read_pos++] = c;
    }

    // poll callback when read_buf is full
    if (uart->read_buf && uart->read_pos == uart->read_length) {
        // clearing read_buf to prevent multiple polls
        uart->read_buf = NULL;
        if (uart->poll_callback) {
            uart->poll_callback(uart->poll_callback_context);
            ;
        }
    }
}

void pbdrv_uart_ev3_handle_irq(uint8_t id) {
    pbdrv_uart_t *uart = &pbdrv_uart[id];
    const pbdrv_uart_ev3_platform_data_t *pdata = uart->pdata;

    /* This determines the cause of UART0 interrupt.*/
    unsigned int int_id = UARTIntStatus(pdata->base_address);

    /* Clears the system interupt status of UART0 in AINTC. */
    IntSystemStatusClear(pdata->sys_int_uart_int_id);

    /* Check if the cause is receiver data condition.*/
    if (UART_INTID_RX_DATA == (int_id & UART_INTID_RX_DATA) || (UART_INTID_CTI == (int_id & UART_INTID_CTI))) {
        int c = UARTCharGetNonBlocking(pdata->base_address);
        if (c != -1) {
            ringbuf_put(&uart->rx_buf, c);
            pbdrv_uart_ev3_update_read_status(uart);
        }
    }

    /* Check if the cause is receiver line error condition.*/
    if (UART_INTID_RX_LINE_STAT == (int_id & UART_INTID_RX_LINE_STAT)) {
        while (UARTRxErrorGet(pdata->base_address)) {
            /* Read a byte from the RBR if RBR has data.*/
            UARTCharGetNonBlocking(pdata->base_address);
        }
    }

}

void pbdrv_uart_init(void) {

    for (int i = 0; i < PBDRV_CONFIG_UART_EV3_NUM_UART; i++) {
        const pbdrv_uart_ev3_platform_data_t *pdata = &pbdrv_uart_ev3_platform_data[i];
        uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_t *uart = &pbdrv_uart[i];
        uart->pdata = pdata;
        ringbuf_init(&uart->rx_buf, rx_data, RX_DATA_SIZE);

        /* Enabling the PSC for given UART.*/
        PSCModuleControl(SOC_PSC_1_REGS, pdata->psc_peripheral_id, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

        // Set the GPIO pins to UART mode.
        pbdrv_gpio_alt(&pdata->pin_rx, pdata->pin_rx_mux);
        pbdrv_gpio_alt(&pdata->pin_tx, pdata->pin_tx_mux);

        /* Enabling the transmitter and receiver*/
        UARTEnable(pdata->base_address);

        /* Configuring the UART clock and baud parameters*/
        pbdrv_uart_set_baud_rate(&uart->uart_dev, BAUD_115200);

        /* Enabling the FIFO and flushing the Tx and Rx FIFOs.*/
        UARTFIFOEnable(pdata->base_address);

        /* Setting the UART Receiver Trigger Level*/
        UARTFIFOLevelSet(pdata->base_address, UART_RX_TRIG_LEVEL_1);

        /* Registers the UARTIsr in the Interrupt Vector Table of AINTC. */
        IntRegister(pdata->sys_int_uart_int_id, pdata->isr_handler);

        /* Map the channel number 2 of AINTC to UART0 system interrupt. */
        IntChannelSet(pdata->sys_int_uart_int_id, 2);

        IntSystemEnable(pdata->sys_int_uart_int_id);

        /* Enable the Interrupts in UART.*/
        UARTIntEnable(pdata->base_address, UART_INT_LINE_STAT | UART_INT_RXDATA_CTI);
    }
}



#endif // PBDRV_CONFIG_UART_EV3
