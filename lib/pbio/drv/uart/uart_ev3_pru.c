/*
 * TI OMAPL PRU SUART Emulation device driver
 * Author: subhasish@mistralsolutions.com
 *
 * This driver supports TI's PRU SUART Emulation and the
 * specs for the same is available at <http://www.ti.com>
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed as is WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
//
// Original license given above. Modifications are licensed as follows:
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors
//
// Based on the original LEGO EV3 and ev3dev sources, with non-Linux inspired
// by liyixiao from EV3RT. All Pybricks modifications are licensed as above.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_EV3_PRU

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <errno.h>

#include "./uart_ev3_pru_lib/omapl_suart_board.h"
#include "./uart_ev3_pru_lib/suart_api.h"
#include "./uart_ev3_pru_lib/suart_utils.h"
#include "./uart_ev3_pru_lib/suart_err.h"
#include "./uart_ev3_pru_lib/pru.h"

struct circ_buf {
    char *buf;
    int32_t head;
    int32_t tail;
};

/* Return count in buffer.  */
#define CIRC_CNT(head, tail, size) (((head) - (tail)) & ((size) - 1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head, tail, size) CIRC_CNT((tail), ((head) + 1), (size))

#define ENOTSUPP    524 /* Operation is not supported */
#define TIOCSER_TEMT    0x01    /* Transmitter physically empty */
#define IO_PHYS            (0x01c00000)
#define DA8XX_SYSCFG0_BASE      (IO_PHYS + 0x14000)
#define DA8XX_PSC0_BASE         0x01c10000
#define DA8XX_PSC1_BASE         0x01e27000
#define OMAPL138_PRU_MEM_BASE         ((void *)0x01C30000)
#define DAVINCI_DA8XX_MCASP0_REG_BASE ((void *)0x01D00000)
#define DA8XX_SHARED_RAM_BASE         ((void *)0x80000000)
#define DMA_PHYS_ADDRESS ((uint32_t)DA8XX_SHARED_RAM_BASE)
#define DMA_VADDRESS_BUFF (DA8XX_SHARED_RAM_BASE)

#define OVERSAMPLE_RATE (SUART_DEFAULT_OVRSMPL)

// FIFO timeout when this number of bits *should* have been received, but
// nothing has.
#define FIFO_TIMEOUT_SYMBOLS (32)
// The number of symbols the PRU SUART sees each time it reads the McASP
// register.
#define MCASP_SYMBOLS_PER_INTERRUPT (4 * 8)

// TODO: Align with global config
#define CORE_CLK_MHZ (300)
#define OSCIN_MHZ (24)

#define CLK_FREQ_PRU (CORE_CLK_MHZ * 1000000 / 2)

#define uart_circ_empty(circ)       ((circ)->head == (circ)->tail)
#define uart_circ_clear(circ)       ((circ)->head = (circ)->tail = 0)

#define dev_err(dev, fmt, ...)

#define NR_SUART (2)
#define SUART_CNTX_SZ 512
#define BUFFER_SIZE (1024) /* Needs to be a 2 size */
#define BUFFER_MASK (BUFFER_SIZE - 1)

#define __suart_err(fmt, args ...)

static inline int suart_oversampling_rate_to_num(int x) {
    switch (x) {
    case SUART_8X_OVRSMPL:
        return 8;
    default:
        return 16;
    }
}

struct uart_icount {
    uint32_t rx;
    uint32_t tx;
    uint32_t frame;
    uint32_t overrun;
    uint32_t brk;
};

struct suart_dma {
    void *dma_vaddr_buff_tx;
    void *dma_vaddr_buff_rx;
    uint32_t dma_phys_addr_tx;
    uint32_t dma_phys_addr_rx;
};

typedef struct {
    struct uart_icount icount;
    suart_struct_handle suart_hdl;
    struct suart_dma suart_dma_addr;
    uint32_t break_rcvt;
    struct circ_buf read_buf;
    struct circ_buf write_buf;
    uint8_t read_data[BUFFER_SIZE];
    uint8_t write_data[BUFFER_SIZE];
    uint32_t baud;
    volatile bool write_busy;
} omapl_pru_suart_t;

static omapl_pru_suart_t suartdevs[NR_SUART];

static void pru_suart_stop_rx(omapl_pru_suart_t *suart);

static uint32_t suart_get_duplex(omapl_pru_suart_t *suart) {
    return suart->suart_hdl.uartType;
}

static void pru_suart_stop_tx(omapl_pru_suart_t *suart) {
    uint16_t txready;
    uint32_t i;

    /* Check if any TX in progress */
    for (i = 0, txready = 1; (i < 10000) && txready; i++) {
        txready =
            (pru_softuart_getTxStatus(&suart->suart_hdl)
                & CHN_TXRX_STATUS_RDY);
    }
    /* To stop tx, disable the TX interrupt */
    uint16_t uartNum = suart->suart_hdl.uartNum;
    suart_intr_clrmask(uartNum, PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
    pru_softuart_clrTxStatus(&suart->suart_hdl);
}

static void omapl_pru_tx_chars(omapl_pru_suart_t *suart) {
    struct circ_buf *xmit = &suart->write_buf;
    int32_t count = 0;

    if (!(suart_get_duplex(suart) & ePRU_SUART_HALF_TX)) {
        return;
    }

    if (uart_circ_empty(xmit) /*|| uart_tx_stopped(&suart->port)*/) {
        pru_suart_stop_tx(suart);
        return;
    }

    for (count = 0; (uint32_t)count <= SUART_FIFO_LEN; count++) {
        *((char *)suart->suart_dma_addr.dma_vaddr_buff_tx + count) = xmit->buf[xmit->tail];
        xmit->tail = (xmit->tail + 1) & (BUFFER_MASK);          /* limit to PAGE_SIZE */
        suart->icount.tx++;
        if (uart_circ_empty(xmit)) {
            break;
        }
    }

    if (count == (SUART_FIFO_LEN + 1)) {
        count = SUART_FIFO_LEN;
    }

    /* Write the character to the data port */
    if (SUART_SUCCESS != pru_softuart_write(&suart->suart_hdl,
        (uint32_t *)&suart->suart_dma_addr.dma_phys_addr_tx, count)) {
        __suart_err("failed to tx data\n");
    }
}

static void omapl_pru_rx_chars(omapl_pru_suart_t *suart) {
    uint16_t rx_status, data_len = SUART_FIFO_LEN;
    uint32_t data_len_read = 0;
    uint8_t suart_data[SUART_FIFO_LEN + 1];
    struct circ_buf *buf;
    uint32_t index, space;

    if (!(suart_get_duplex(suart) & ePRU_SUART_HALF_RX)) {
        return;
    }
    /* read the status */
    rx_status = pru_softuart_getRxStatus(&suart->suart_hdl);
    pru_softuart_read_data(&suart->suart_hdl, suart_data,
        data_len + 1, &data_len_read);

    /* check for errors */
    if (rx_status & CHN_TXRX_STATUS_ERR) {
        // NB: Originally, here the driver used to call pru_suart_stop_rx if
        // the "sensor was inited" but it is not clear why.

        if (rx_status & CHN_TXRX_STATUS_FE) {
            suart->icount.frame++;
        }

        if (rx_status & CHN_TXRX_STATUS_OVRNERR) {
            suart->icount.overrun++;
        }

        if (rx_status & CHN_TXRX_STATUS_BI) {
            suart->icount.brk++;
            suart->break_rcvt = 1;
        }

    } else {
        buf = &suart->read_buf;

        // Receive data into ring buffer
        space = CIRC_SPACE(buf->head, buf->tail, BUFFER_SIZE);

        if (space < data_len_read) {
            data_len_read = space;
            pru_suart_stop_rx(suart);
        }

        for (index = 0; index < data_len_read; index++)
        {
            buf->buf[buf->head] = suart_data[index];
            buf->head = (buf->head + 1) & BUFFER_MASK;
        }
    }

    pru_softuart_clrRxStatus(&suart->suart_hdl);

}

void pbdrv_uart_ev3_pru_handle_irq_data(uint8_t id) {
    omapl_pru_suart_t *suart = &suartdevs[id];
    uint16_t txrx_flag;
    uint32_t ret;
    uint16_t uartNum = suart->suart_hdl.uartNum;

    do {
        ret = pru_softuart_get_isrstatus(uartNum, &txrx_flag);
        if (PRU_SUART_SUCCESS != ret) {
            __suart_err
                ("suart: failed to get interrupt, ret: 0x%X txrx_flag 0x%X\n", ret, txrx_flag);
            return;
        }
        if ((PRU_RX_INTR & txrx_flag) == PRU_RX_INTR) {
            pru_intr_clr_isrstatus(uartNum, PRU_RX_INTR);
            omapl_pru_rx_chars(suart);
        }

        if ((PRU_TX_INTR & txrx_flag) == PRU_TX_INTR) {
            pru_intr_clr_isrstatus(uartNum, PRU_TX_INTR);
            pru_softuart_clrTxStatus(&suart->suart_hdl);
            omapl_pru_tx_chars(suart);
            suart->write_busy = false;
        }
    } while (txrx_flag & (PRU_RX_INTR | PRU_TX_INTR));
}

bool pbdrv_uart_ev3_pru_can_write(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    return !suart->write_busy;
}

void pru_suart_stop_rx(omapl_pru_suart_t *suart) {

    // Stop PRU reception of data and clear FIFO
    pru_softuart_stopReceive(&suart->suart_hdl);

    /* disable rx interrupt */
    suart_intr_clrmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
        | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
        | CHN_TXRX_IE_MASK_TIMEOUT);

    pru_softuart_clrRxFifo(&suart->suart_hdl);
    pru_softuart_clrRxStatus(&suart->suart_hdl);

}

void pru_suart_start_tx(omapl_pru_suart_t *suart) {
    suart_intr_setmask(suart->suart_hdl.uartNum, PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
    omapl_pru_tx_chars(suart);
}

uint32_t pru_suart_tx_empty(omapl_pru_suart_t *suart) {
    return (pru_softuart_getTxStatus(&suart->suart_hdl) & CHN_TXRX_STATUS_RDY) ? 0 : TIOCSER_TEMT;
}

int32_t pru_suart_startup(omapl_pru_suart_t *suart) {
    int32_t retval = 0;

    /*
     * Disable interrupts from this port
     */
    suart_intr_clrmask(suart->suart_hdl.uartNum,
        PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
    suart_intr_clrmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
        | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
        | CHN_TXRX_IE_MASK_TIMEOUT);

    /*
     * enable interrupts from this port
     */
    suart_intr_setmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, SUART_GBL_INTR_ERR_MASK);

    suart_intr_setmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, CHN_TXRX_IE_MASK_CMPLT | CHN_TXRX_IE_MASK_TIMEOUT);

    suart_intr_setmask(suart->suart_hdl.uartNum,
        PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);

    suart_intr_setmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, CHN_RX_IE_MASK_OVRN);

    if ((suart_get_duplex(suart) & ePRU_SUART_HALF_TX)
        == ePRU_SUART_HALF_TX) {
        suart_pru_to_host_intr_enable(suart->suart_hdl.uartNum,
            PRU_TX_INTR, true);
    }
    /* Seed RX if port is half-rx or full-duplex */
    if ((suart_get_duplex(suart) & ePRU_SUART_HALF_RX) == ePRU_SUART_HALF_RX) {
        suart_pru_to_host_intr_enable(suart->suart_hdl.uartNum, PRU_RX_INTR, true);
        // Note: the final argument is effectively the trigger threshold of the UART RX FIFO.
        // We chose 8 to give ourselves some breathing room in case we are slow to process the
        // FIFO interrupt.
        pru_softuart_read(&suart->suart_hdl, (uint32_t *)&suart->suart_dma_addr.dma_phys_addr_rx, 8);
    }
    return retval;
}

void pru_suart_shutdown(omapl_pru_suart_t *suart) {
    /*
     * Disable interrupts from this port
     */
    /* Disable BI and FE intr */
    suart_intr_clrmask(suart->suart_hdl.uartNum, PRU_TX_INTR, CHN_TXRX_IE_MASK_CMPLT);
    suart_intr_clrmask(suart->suart_hdl.uartNum,
        PRU_RX_INTR, CHN_TXRX_IE_MASK_BI
        | CHN_TXRX_IE_MASK_FE | CHN_TXRX_IE_MASK_CMPLT
        | CHN_TXRX_IE_MASK_TIMEOUT | CHN_RX_IE_MASK_OVRN);
}

void pru_suart_release_port(omapl_pru_suart_t *suart) {
    if (pru_softuart_close(&suart->suart_hdl) != SUART_SUCCESS) {
        dev_err(&pdev->dev, "failed to close suart\n");
    }
}

int32_t pru_suart_request_port(omapl_pru_suart_t *suart) {
    suart_config pru_suart_config;
    uint32_t err = 0;

    err = pru_softuart_open(&suart->suart_hdl);
    if (PRU_SUART_SUCCESS != err) {
        dev_err(&pdev->dev, "failed to open suart: %d\n", err);
        err = -ENODEV;
        goto exit;
    }

    // We want the PRU to interrupt us any time there is data in the FIFO
    // and we haven't seen any new data for a period of FIFO_TIMEOUT_SYMBOLS
    // symbols. For example, if FIFO_TIMEOUT_SYMBOLS is 32, the we will get an
    // interrupt if the line is silent for the time it would ordinarily take
    // to send four bytes.
    pru_set_fifo_timeout(FIFO_TIMEOUT_SYMBOLS * suart_oversampling_rate_to_num(OVERSAMPLE_RATE) / MCASP_SYMBOLS_PER_INTERRUPT);

    if (suart->suart_hdl.uartNum == PRU_SUART_UART1) {
        pru_suart_config.TXSerializer = PRU_SUART1_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART1_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART2) {
        pru_suart_config.TXSerializer = PRU_SUART2_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART2_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART3) {
        pru_suart_config.TXSerializer = PRU_SUART3_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART3_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART4) {
        pru_suart_config.TXSerializer = PRU_SUART4_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART4_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART5) {
        pru_suart_config.TXSerializer = PRU_SUART5_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART5_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART6) {
        pru_suart_config.TXSerializer = PRU_SUART6_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART6_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART7) {
        pru_suart_config.TXSerializer = PRU_SUART7_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART7_CONFIG_RX_SER;
    } else if (suart->suart_hdl.uartNum == PRU_SUART_UART8) {
        pru_suart_config.TXSerializer = PRU_SUART8_CONFIG_TX_SER;
        pru_suart_config.RXSerializer = PRU_SUART8_CONFIG_RX_SER;
    } else {
        return -ENOTSUPP;
    }

    /* Some defaults to startup. reconfigured by terimos later */
    pru_suart_config.txClkDivisor = 1;
    pru_suart_config.rxClkDivisor = 1;
    pru_suart_config.txBitsPerChar = ePRU_SUART_DATA_BITS8;
    pru_suart_config.rxBitsPerChar = ePRU_SUART_DATA_BITS8;     /* including start and stop bit 8 + 2 */
    pru_suart_config.Oversampling = OVERSAMPLE_RATE;

    if (PRU_SUART_SUCCESS !=
        pru_softuart_setconfig(&suart->suart_hdl,
            &pru_suart_config)) {
        dev_err(&pdev->dev,
            "pru_softuart_setconfig: failed to set config: %X\n",
            err);
    }
exit:
    return err;
}

int32_t pbdrv_uart_ev3_pru_activate(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];

    pru_suart_request_port(suart);

    int32_t retval = 0;
    struct circ_buf *buf;
    buf = &suart->read_buf;

    retval = pru_suart_startup(suart);
    suart->break_rcvt = 0;

    // When activating port flush buffer
    pru_softuart_clrRxFifo(&suart->suart_hdl);
    buf->head = 0;
    buf->tail = 0;

    return retval;
}

void pbdrv_uart_ev3_pru_deactivate(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    pru_suart_stop_tx(suart);
    while (pru_suart_tx_empty(suart) == 0) {
        ;
    }
    pru_suart_stop_rx(suart);
    pru_suart_shutdown(suart);
}

void pbdrv_uart_ev3_pru_exit(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    // Simulate calls from TTY implementation
    pru_suart_stop_tx(suart);
    while (pru_suart_tx_empty(suart) == 0) {
        ;
    }
    pru_suart_stop_rx(suart);
    while (pru_suart_tx_empty(suart) == 0) {
        ;
    }
    pru_suart_shutdown(suart);
}

void pbdrv_uart_ev3_pru_set_baudrate(uint8_t line, uint32_t baud) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    if (baud == suart->baud) {
        return;
    }
    suart->baud = baud;
    pru_softuart_setdatabits(&suart->suart_hdl, ePRU_SUART_DATA_BITS8, ePRU_SUART_DATA_BITS8);
    pru_softuart_setbaud(&suart->suart_hdl, SUART_DEFAULT_BAUD / baud, SUART_DEFAULT_BAUD / baud);
}

int32_t pbdrv_uart_ev3_pru_get_break_state(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    if (suart->break_rcvt) {
        suart->break_rcvt = 0;
        return 1;
    }
    return 0;
}

int32_t pbdrv_uart_ev3_pru_write_bytes(uint8_t line, const uint8_t *pdata, int32_t size) {
    omapl_pru_suart_t *suart = &suartdevs[line];

    if (suart->write_busy || size == 0) {
        return 0;
    }
    suart->write_busy = true;

    struct circ_buf *buf;
    int32_t space, index;

    buf = &suart->write_buf;

    // Save data into transmit ring buffer
    space = CIRC_SPACE(buf->head, buf->tail, BUFFER_SIZE);

    if (space < size) {
        size = space;
    }

    for (index = 0; index < size; index++)
    {
        buf->buf[buf->head] = pdata[index];
        buf->head = (buf->head + 1) & BUFFER_MASK;
    }

    pru_suart_start_tx(suart);
    return size;
}

bool pbdrv_uart_ev3_pru_tx_empty(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    return pru_suart_tx_empty(suart);
}

int32_t pbdrv_uart_ev3_pru_read_bytes(uint8_t line, uint8_t *pdata, int32_t size) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    struct circ_buf *buf;
    int32_t space, index;

    buf = &suart->read_buf;

    // Deliver data from ring buffer
    space = CIRC_CNT(buf->head, buf->tail, BUFFER_SIZE);

    if (size > space) {
        size = space;
    }

    for (index = 0; index < size; index++)
    {
        pdata[index] = buf->buf[buf->tail];
        buf->tail = (buf->tail + 1) & BUFFER_MASK;
    }

    return size;
}

int32_t pbdrv_uart_ev3_pru_size_data_rx_buffer(uint8_t line) {
    omapl_pru_suart_t *suart = &suartdevs[line];
    struct circ_buf *buf;
    buf = &suart->read_buf;

    return CIRC_CNT(buf->head, buf->tail, BUFFER_SIZE);
}

int32_t pbdrv_uart_ev3_pru_load_firmware(uint8_t *firmware_data, uint32_t firmware_size) {

    static arm_pru_iomap pru_arm_iomap = {0};

    pru_arm_iomap.pru_io_addr = OMAPL138_PRU_MEM_BASE;
    pru_arm_iomap.mcasp_io_addr = DAVINCI_DA8XX_MCASP0_REG_BASE;
    pru_arm_iomap.psc0_io_addr = (void *)DA8XX_PSC0_BASE;
    pru_arm_iomap.psc1_io_addr = (void *)DA8XX_PSC1_BASE;
    pru_arm_iomap.syscfg_io_addr = (void *)DA8XX_SYSCFG0_BASE;

    pru_arm_iomap.pFifoBufferPhysBase = (void *)DMA_PHYS_ADDRESS;
    pru_arm_iomap.pFifoBufferVirtBase = (void *)DMA_VADDRESS_BUFF;
    pru_arm_iomap.pru_clk_freq = (CLK_FREQ_PRU / 1000000);

    pru_softuart_init(SUART_DEFAULT_BAUD, SUART_DEFAULT_BAUD, SUART_DEFAULT_OVRSMPL, firmware_data, firmware_size, &pru_arm_iomap);

    for (uint32_t i = 0; i < NR_SUART; i++) {
        omapl_pru_suart_t *suart = &suartdevs[i];
        suart->suart_hdl.uartNum = i + 1;
        suart->break_rcvt = 0;
        suart->baud = 0;
        suart->read_buf.buf = (char *)&suart->read_data[0];
        suart->write_buf.buf = (char *)&suart->write_data[0];
        suart->suart_dma_addr.dma_vaddr_buff_tx = DMA_VADDRESS_BUFF + (2 * SUART_CNTX_SZ * i);
        suart->suart_dma_addr.dma_vaddr_buff_rx = DMA_VADDRESS_BUFF + ((2 * SUART_CNTX_SZ * i) + SUART_CNTX_SZ);
        suart->suart_dma_addr.dma_phys_addr_tx = DMA_PHYS_ADDRESS + (2 * SUART_CNTX_SZ * i);
        suart->suart_dma_addr.dma_phys_addr_rx = DMA_PHYS_ADDRESS + ((2 * SUART_CNTX_SZ * i) + SUART_CNTX_SZ);
    }

    return 0;
}

#endif // PBDRV_CONFIG_UART_EV3_PRU
