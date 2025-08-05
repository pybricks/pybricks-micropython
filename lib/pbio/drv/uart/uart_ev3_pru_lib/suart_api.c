/*
 * pru/hal/uart/src/suart_api.c
 *
 * Copyright (C) 2010 Texas Instruments Incorporated
 * Author: Jitendra Kumar <jitendra@mistralsolutions.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as  published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 *====================
 * Includes
 *====================
 */

#include "suart_api.h"
#include "suart_pru_regs.h"
#include "pru.h"
#include "omapl_suart_board.h"
#include "suart_utils.h"
#include "suart_err.h"

#include "cslr_mcasp.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static uint8_t gUartStatuTable[8];
static arm_pru_iomap pru_arm_iomap;
static int32_t suart_set_pru_id(uint32_t pru_no);
static void pru_set_rx_tx_mode(uint32_t pru_mode, uint32_t pruNum);
static void pru_set_delay_count(uint32_t pru_freq);

void pru_set_ram_data(arm_pru_iomap *arm_iomap_pru) {

    PRU_SUART_RegsOvly pru_suart_regs = (PRU_SUART_RegsOvly)arm_iomap_pru->pru_io_addr;
    uint32_t *pu32SrCtlAddr = (uint32_t *)((uint32_t)
        arm_iomap_pru->mcasp_io_addr + 0x180);
    pru_suart_tx_cntx_priv *pru_suart_tx_priv = NULL;
    pru_suart_rx_cntx_priv *pru_suart_rx_priv = NULL;
    uint8_t *pu32_pru_ram_base = (uint8_t *)arm_iomap_pru->pru_io_addr;

    /* ***************************** UART 0  **************************************** */

    /* Channel 0 context information is Tx */
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_TX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART1_CONFIG_TX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART1_CONFIG_DUPLEX & PRU_SUART_HALF_TX_DISABLED) == PRU_SUART_HALF_TX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART1_CONFIG_TX_SER)) = MCASP_SRCTL_TX_MODE;
    #endif
    pru_suart_regs->Reserved1 = 1;
    pru_suart_tx_priv = (pru_suart_tx_cntx_priv *)(pu32_pru_ram_base + 0x0B0);  /* SUART1 TX context base addr */
    pru_suart_tx_priv->asp_xsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART1_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->asp_xbuf_base = (uint32_t)(MCASP_XBUF_BASE_ADDR + (PRU_SUART1_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->buff_addr = 0x0090; /* SUART1 TX formatted data base addr */

    /* Channel 1 is Rx context information */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_RX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART1_CONFIG_RX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART1_CONFIG_DUPLEX & PRU_SUART_HALF_RX_DISABLED) == PRU_SUART_HALF_RX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART1_CONFIG_RX_SER)) = MCASP_SRCTL_RX_MODE;
    #endif
    /* RX is active by default, write the dummy received data at PRU RAM addr 0x1FC to avoid memory corruption */
    pru_suart_regs->CH_TXRXData = RX_DEFAULT_DATA_DUMP_ADDR;
    pru_suart_regs->Reserved1 = 0;
    pru_suart_rx_priv = (pru_suart_rx_cntx_priv *)(pu32_pru_ram_base + 0x0C0);  /* SUART1 RX context base addr */
    pru_suart_rx_priv->asp_rbuf_base = (uint32_t)(MCASP_RBUF_BASE_ADDR + (PRU_SUART1_CONFIG_RX_SER << 2));
    pru_suart_rx_priv->asp_rsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART1_CONFIG_RX_SER << 2));


    /* ***************************** UART 1  **************************************** */
    /* Channel 2 context information is Tx */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_TX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART2_CONFIG_TX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART2_CONFIG_DUPLEX & PRU_SUART_HALF_TX_DISABLED) == PRU_SUART_HALF_TX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART2_CONFIG_TX_SER)) = MCASP_SRCTL_TX_MODE;
    #endif
    pru_suart_regs->Reserved1 = 1;
    pru_suart_tx_priv = (pru_suart_tx_cntx_priv *)(pu32_pru_ram_base + 0x100);  /* SUART2 TX context base addr */
    pru_suart_tx_priv->asp_xsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART2_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->asp_xbuf_base = (uint32_t)(MCASP_XBUF_BASE_ADDR + (PRU_SUART2_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->buff_addr = 0x00E0; /* SUART2 TX formatted data base addr */

    /* Channel 3 is Rx context information */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_RX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART2_CONFIG_RX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART2_CONFIG_DUPLEX & PRU_SUART_HALF_RX_DISABLED) == PRU_SUART_HALF_RX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART2_CONFIG_RX_SER)) = MCASP_SRCTL_RX_MODE;
    #endif
    /* RX is active by default, write the dummy received data at PRU RAM addr 0x1FC to avoid memory corruption */
    pru_suart_regs->CH_TXRXData = RX_DEFAULT_DATA_DUMP_ADDR;
    pru_suart_regs->Reserved1 = 0;
    pru_suart_rx_priv = (pru_suart_rx_cntx_priv *)(pu32_pru_ram_base + 0x110);  /* SUART2 RX context base addr */
    pru_suart_rx_priv->asp_rbuf_base = (uint32_t)(MCASP_RBUF_BASE_ADDR + (PRU_SUART2_CONFIG_RX_SER << 2));
    pru_suart_rx_priv->asp_rsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART2_CONFIG_RX_SER << 2));

    /* ***************************** UART 2  **************************************** */
    /* Channel 4 context information is Tx */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_TX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART3_CONFIG_TX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART3_CONFIG_DUPLEX & PRU_SUART_HALF_TX_DISABLED) == PRU_SUART_HALF_TX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART3_CONFIG_TX_SER)) = MCASP_SRCTL_TX_MODE;
    #endif
    pru_suart_regs->Reserved1 = 1;
    pru_suart_tx_priv = (pru_suart_tx_cntx_priv *)(pu32_pru_ram_base + 0x150);  /* SUART3 TX context base addr */
    pru_suart_tx_priv->asp_xsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART3_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->asp_xbuf_base = (uint32_t)(MCASP_XBUF_BASE_ADDR + (PRU_SUART3_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->buff_addr = 0x0130; /* SUART3 TX formatted data base addr */

    /* Channel 5 is Rx context information */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_RX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART3_CONFIG_RX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART3_CONFIG_DUPLEX & PRU_SUART_HALF_RX_DISABLED) == PRU_SUART_HALF_RX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART3_CONFIG_RX_SER)) = MCASP_SRCTL_RX_MODE;
    #endif
    /* RX is active by default, write the dummy received data at PRU RAM addr 0x1FC to avoid memory corruption */
    pru_suart_regs->CH_TXRXData = RX_DEFAULT_DATA_DUMP_ADDR;
    pru_suart_regs->Reserved1 = 0;
    pru_suart_rx_priv = (pru_suart_rx_cntx_priv *)(pu32_pru_ram_base + 0x160);  /* SUART3 RX context base addr */
    pru_suart_rx_priv->asp_rbuf_base = (uint32_t)(MCASP_RBUF_BASE_ADDR + (PRU_SUART3_CONFIG_RX_SER << 2));
    pru_suart_rx_priv->asp_rsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART3_CONFIG_RX_SER << 2));

    /* ***************************** UART 3  **************************************** */
    /* Channel 6 context information is Tx */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_TX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART4_CONFIG_TX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART4_CONFIG_DUPLEX & PRU_SUART_HALF_TX_DISABLED) == PRU_SUART_HALF_TX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART4_CONFIG_TX_SER)) = MCASP_SRCTL_TX_MODE;
    #endif
    pru_suart_regs->Reserved1 = 1;
    pru_suart_tx_priv = (pru_suart_tx_cntx_priv *)(pu32_pru_ram_base + 0x1A0);  /* SUART4 TX context base addr */
    pru_suart_tx_priv->asp_xsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART4_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->asp_xbuf_base = (uint32_t)(MCASP_XBUF_BASE_ADDR + (PRU_SUART4_CONFIG_TX_SER << 2));
    pru_suart_tx_priv->buff_addr = 0x0180; /* SUART4 TX formatted data base addr */

    /* Channel 7 is Rx context information */
    pru_suart_regs++;
    pru_suart_regs->CH_Ctrl_Config1.mode = SUART_CHN_RX;
    pru_suart_regs->CH_Ctrl_Config1.serializer_num = (0xF & PRU_SUART4_CONFIG_RX_SER);
    pru_suart_regs->CH_Ctrl_Config1.over_sampling = SUART_DEFAULT_OVRSMPL;
    pru_suart_regs->CH_Config2_TXRXStatus.bits_per_char = 8;
    #if ((PRU_SUART4_CONFIG_DUPLEX & PRU_SUART_HALF_RX_DISABLED) == PRU_SUART_HALF_RX_DISABLED)
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_DISABLED;
    #else
    pru_suart_regs->CH_Config2_TXRXStatus.chn_state = SUART_CHN_ENABLED;
    *((uint32_t *)(pu32SrCtlAddr + PRU_SUART4_CONFIG_RX_SER)) = MCASP_SRCTL_RX_MODE;
    #endif
    /* RX is active by default, write the dummy received data at PRU RAM addr 0x1FC to avoid memory corruption */
    pru_suart_regs->CH_TXRXData = RX_DEFAULT_DATA_DUMP_ADDR;
    pru_suart_regs->Reserved1 = 0;
    pru_suart_rx_priv = (pru_suart_rx_cntx_priv *)(pu32_pru_ram_base + 0x1B0);  /* SUART4 RX context base addr */
    pru_suart_rx_priv->asp_rbuf_base = (uint32_t)(MCASP_RBUF_BASE_ADDR + (PRU_SUART4_CONFIG_RX_SER << 2));
    pru_suart_rx_priv->asp_rsrctl_base = (uint32_t)(MCASP_SRCTL_BASE_ADDR + (PRU_SUART4_CONFIG_RX_SER << 2));
}

/*
 * suart Initialization routine
 */
int16_t pru_softuart_init(uint32_t txBaudValue,
    uint32_t rxBaudValue,
    uint32_t oversampling,
    uint8_t *pru_suart_emu_code,
    uint32_t fw_size, arm_pru_iomap *arm_iomap_pru) {
    uint32_t omapl_addr;
    uint32_t u32loop;
    int16_t status = PRU_SUART_SUCCESS;
    int16_t idx;
    int16_t retval;

    pru_arm_iomap.pru_io_addr = arm_iomap_pru->pru_io_addr;
    pru_arm_iomap.mcasp_io_addr = arm_iomap_pru->mcasp_io_addr;
    pru_arm_iomap.psc0_io_addr = arm_iomap_pru->psc0_io_addr;
    pru_arm_iomap.psc1_io_addr = arm_iomap_pru->psc1_io_addr;
    pru_arm_iomap.syscfg_io_addr = arm_iomap_pru->syscfg_io_addr;
    pru_arm_iomap.pFifoBufferPhysBase = arm_iomap_pru->pFifoBufferPhysBase;
    pru_arm_iomap.pFifoBufferVirtBase = arm_iomap_pru->pFifoBufferVirtBase;
    pru_arm_iomap.pru_clk_freq = arm_iomap_pru->pru_clk_freq;

    omapl_addr = (uint32_t)arm_iomap_pru->syscfg_io_addr;

    omapl_addr = (uint32_t)arm_iomap_pru->psc1_io_addr;
    suart_mcasp_psc_enable(omapl_addr);

    omapl_addr = (uint32_t)arm_iomap_pru->mcasp_io_addr;
    /* Configure McASP0  */
    suart_mcasp_config(omapl_addr, txBaudValue, rxBaudValue, oversampling,
        arm_iomap_pru);

    pru_enable(0, arm_iomap_pru);

    omapl_addr = (uint32_t)arm_iomap_pru->pru_io_addr;

//	for (u32loop = 0; u32loop < 512; u32loop++)
    for (u32loop = 0; u32loop < 512; u32loop += 4)   // Fixed the alignment fault -- ertl-liyixiao
    {
        *(uint32_t *)(omapl_addr | u32loop) = 0x0;
    }

    pru_load(PRU_NUM0, (uint32_t *)pru_suart_emu_code,
        (fw_size / sizeof(uint32_t)), arm_iomap_pru);

    retval = arm_to_pru_intr_init();
    if (-1 == retval) {
        return status;
    }
    pru_set_delay_count(pru_arm_iomap.pru_clk_freq);

    suart_set_pru_id(0);

    pru_set_rx_tx_mode(PRU0_MODE, PRU_NUM0);

    pru_set_ram_data(arm_iomap_pru);

    pru_run(PRU_NUM0, arm_iomap_pru);

    /* Initialize gUartStatuTable */
    for (idx = 0; idx < 8; idx++) {
        gUartStatuTable[idx] = ePRU_SUART_UART_FREE;
    }

    return status;
}

static void pru_set_rx_tx_mode(uint32_t pru_mode, uint32_t pruNum) {

    uint32_t pruOffset;

    if (pruNum == PRU_NUM0) {
        /* PRU0 */
        pruOffset = PRU_SUART_PRU0_RX_TX_MODE;
    } else {
        return;
    }

    pru_ram_write_data(pruOffset, (uint8_t *)&pru_mode, 1, &pru_arm_iomap);

}


void pru_set_fifo_timeout(uint32_t timeout) {
    /* PRU 0 */
    pru_ram_write_data(PRU_SUART_PRU0_IDLE_TIMEOUT_OFFSET,
        (uint8_t *)&timeout, 2, &pru_arm_iomap);
}

/* Not needed as PRU Soft Uart Firmware is implemented as Mcasp Event Based */
static void pru_set_delay_count(uint32_t pru_freq) {
    uint32_t u32delay_cnt;

    if (pru_freq == 228) {
        u32delay_cnt = 5;
    } else if (pru_freq == 186) {
        u32delay_cnt = 5;
    } else {
        u32delay_cnt = 3;
    }

    /* PRU 0 */
    pru_ram_write_data(PRU_SUART_PRU0_DELAY_OFFSET,
        (uint8_t *)&u32delay_cnt, 1, &pru_arm_iomap);

}

void pru_mcasp_deinit(void) {
    suart_mcasp_reset(&pru_arm_iomap);
}

int16_t pru_softuart_deinit(void) {
    uint32_t offset;
    int16_t s16retval = 0;
    uint32_t u32value = 0;

    pru_disable(&pru_arm_iomap);

    offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATCLRINT1 &
            0xFFFF);
    u32value = 0xFFFFFFFF;
    s16retval =
        pru_ram_write_data_4byte(offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }
    offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATCLRINT0 &
            0xFFFF);
    u32value = 0xFFFFFFFF;
    s16retval =
        pru_ram_write_data_4byte(offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }

    return PRU_SUART_SUCCESS;
}

/*
 * suart Instance open routine
 */
int16_t pru_softuart_open(suart_handle hSuart) {
    int16_t status = PRU_SUART_SUCCESS;

    switch (hSuart->uartNum) {
        /* ************ PRU 0 ************** */
        case PRU_SUART_UART1:
            if (gUartStatuTable[PRU_SUART_UART1 - 1] ==
                ePRU_SUART_UART_IN_USE) {
                status = SUART_UART_IN_USE;
                return status;
            } else {
                hSuart->uartStatus = ePRU_SUART_UART_IN_USE;
                hSuart->uartType = PRU_SUART1_CONFIG_DUPLEX;
                hSuart->uartTxChannel = PRU_SUART1_CONFIG_TX_SER;
                hSuart->uartRxChannel = PRU_SUART1_CONFIG_RX_SER;

                gUartStatuTable[PRU_SUART_UART1 - 1] =
                    ePRU_SUART_UART_IN_USE;
            }

            break;

        case PRU_SUART_UART2:
            if (gUartStatuTable[PRU_SUART_UART2 - 1] ==
                ePRU_SUART_UART_IN_USE) {
                status = SUART_UART_IN_USE;
                return status;
            } else {

                hSuart->uartStatus = ePRU_SUART_UART_IN_USE;
                hSuart->uartType = PRU_SUART2_CONFIG_DUPLEX;
                hSuart->uartTxChannel = PRU_SUART2_CONFIG_TX_SER;
                hSuart->uartRxChannel = PRU_SUART2_CONFIG_RX_SER;

                gUartStatuTable[PRU_SUART_UART2 - 1] =
                    ePRU_SUART_UART_IN_USE;
            }

            break;

        case PRU_SUART_UART3:
            if (gUartStatuTable[PRU_SUART_UART3 - 1] ==
                ePRU_SUART_UART_IN_USE) {
                status = SUART_UART_IN_USE;
                return status;
            } else {

                hSuart->uartStatus = ePRU_SUART_UART_IN_USE;
                hSuart->uartType = PRU_SUART3_CONFIG_DUPLEX;
                hSuart->uartTxChannel = PRU_SUART3_CONFIG_TX_SER;
                hSuart->uartRxChannel = PRU_SUART3_CONFIG_RX_SER;

                gUartStatuTable[PRU_SUART_UART3 - 1] =
                    ePRU_SUART_UART_IN_USE;
            }

            break;

        case PRU_SUART_UART4:
            if (gUartStatuTable[PRU_SUART_UART4 - 1] ==
                ePRU_SUART_UART_IN_USE) {
                status = SUART_UART_IN_USE;
                return status;
            } else {

                hSuart->uartStatus = ePRU_SUART_UART_IN_USE;
                hSuart->uartType = PRU_SUART4_CONFIG_DUPLEX;
                hSuart->uartTxChannel = PRU_SUART4_CONFIG_TX_SER;
                hSuart->uartRxChannel = PRU_SUART4_CONFIG_RX_SER;

                gUartStatuTable[PRU_SUART_UART4 - 1] =
                    ePRU_SUART_UART_IN_USE;
            }
            break;

        default:
            /* return invalid UART */
            status = SUART_INVALID_UART_NUM;
            break;
    }
    return status;
}

/*
 * suart instance close routine
 */
int16_t pru_softuart_close(suart_handle hUart) {
    int16_t status = SUART_SUCCESS;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    } else {
        gUartStatuTable[hUart->uartNum - 1] = ePRU_SUART_UART_FREE;
        /* Reset the Instance to Invalid */
        hUart->uartNum = PRU_SUART_UARTx_INVALID;
        hUart->uartStatus = ePRU_SUART_UART_FREE;
    }
    return status;
}

/*
 * suart routine for setting relative baud rate
 */
int16_t pru_softuart_setbaud
    (suart_handle hUart,
    uint16_t txClkDivisor,
    uint16_t rxClkDivisor) {
    uint32_t offset;
    uint32_t pruOffset;
    int16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint16_t regval = 0;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    /* Set the clock divisor value into the McASP */
    if ((txClkDivisor > 385) || (txClkDivisor == 0)) {
        return SUART_INVALID_CLKDIVISOR;
    }

    if ((rxClkDivisor > 385) || (rxClkDivisor == 0)) {
        return SUART_INVALID_CLKDIVISOR;
    }


    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }




    if (txClkDivisor != 0) {
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= (~0x3FF);
        regval |= txClkDivisor;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }

    if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        chNum++;
    } else {
        return PRU_MODE_INVALID;
    }

    regval = 0;
    if (rxClkDivisor != 0) {
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= (~0x3FF);
        regval |= txClkDivisor;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    return status;
}

/*
 * suart routine for setting number of bits per character for a specific uart
 */
int16_t pru_softuart_setdatabits
    (suart_handle hUart, uint16_t txDataBits, uint16_t rxDataBits) {
    uint32_t offset;
    uint32_t pruOffset;
    int16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint32_t reg_val;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    /*
     * NOTE:
     * The supported data bits are 6, 7, 8, 9, 10, 11 and 12 bits per character
     */

    if ((txDataBits < ePRU_SUART_DATA_BITS6) || (txDataBits > ePRU_SUART_DATA_BITS12)) {
        return PRU_SUART_ERR_PARAMETER_INVALID;
    }

    if ((rxDataBits < ePRU_SUART_DATA_BITS6) || (rxDataBits > ePRU_SUART_DATA_BITS12)) {
        return PRU_SUART_ERR_PARAMETER_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }


    if (txDataBits != 0) {
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG2_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&reg_val, 1,
            &pru_arm_iomap);

        reg_val &= ~(0xF);
        reg_val |= txDataBits;
        pru_ram_write_data(offset, (uint8_t *)&reg_val, 1,
            &pru_arm_iomap);
    }

    if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        chNum++;
    } else {
        return PRU_MODE_INVALID;
    }

    if (rxDataBits != 0) {
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG2_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&reg_val, 1,
            &pru_arm_iomap);

        reg_val &= ~(0xF);
        reg_val |= rxDataBits;

        pru_ram_write_data(offset, (uint8_t *)&rxDataBits, 1,
            &pru_arm_iomap);
    }

    return status;
}

/*
 * suart routine to configure specific uart
 */
int16_t pru_softuart_setconfig(suart_handle hUart, suart_config *configUart) {
    uint32_t offset;
    uint32_t pruOffset;
    int16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint16_t regVal = 0;
    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    /*
     * NOTE:
     * Dependent baud rate for the given UART, the value MUST BE LESS THAN OR
     * EQUAL TO 64, preScalarValue <= 64
     */
    /* Validate the value of relative buad rate */
    if ((configUart->txClkDivisor > 384) || (configUart->rxClkDivisor > 384)) {
        return SUART_INVALID_CLKDIVISOR;
    }
    /* Validate the bits per character */
    if ((configUart->txBitsPerChar < 8) || (configUart->txBitsPerChar > 14)) {
        return PRU_SUART_ERR_PARAMETER_INVALID;
    }

    if ((configUart->rxBitsPerChar < 8) || (configUart->rxBitsPerChar > 14)) {
        return PRU_SUART_ERR_PARAMETER_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Configuring the Transmit part of the given UART */
    if (configUart->TXSerializer != PRU_SUART_SERIALIZER_NONE) {
        /* Serializer has been as TX in mcasp config, by writing 1 in bits corresponding to tx serializer
           in PFUNC regsiter i.e. already set to GPIO mode PRU code will set then back to MCASP mode once
           TX request for that serializer is posted. It is required because at this point Mcasp is accessed
           by both PRU and DSP have lower priority for Mcasp in comparison to PRU and DPS keeps on looping
           there only.
        */

        /*
          suart_mcasp_tx_serialzier_set (configUart->TXSerializer, &pru_arm_iomap);
        */

        /* Configuring TX serializer  */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CTRL_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);

        regVal = (configUart->TXSerializer <<
                PRU_SUART_CH_CTRL_SR_SHIFT);

        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        /* Configuring the Transmit part of the given UART */
        /* Configuring TX prescalar value */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        regVal =
            regVal | (configUart->txClkDivisor <<
                    PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);
        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        /* Configuring TX bits per character value */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG2_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        regVal =
            regVal | (configUart->txBitsPerChar <<
                    PRU_SUART_CH_CONFIG2_BITPERCHAR_SHIFT);
        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
    }

    if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        chNum++;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Configuring the Transmit part of the given UART */
    if (configUart->RXSerializer != PRU_SUART_SERIALIZER_NONE) {
        /* Configuring RX serializer  */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CTRL_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);

        regVal = (configUart->RXSerializer <<
                PRU_SUART_CH_CTRL_SR_SHIFT);
        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);

        /* Configuring RX prescalar value and Oversampling */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        regVal =
            regVal | (configUart->rxClkDivisor <<
                    PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT) |
            (configUart->Oversampling <<
                    PRU_SUART_CH_CONFIG1_OVS_SHIFT);
        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);

        /* Configuring RX bits per character value */
        offset =
            pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG2_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
        regVal =
            regVal | (configUart->rxBitsPerChar <<
                    PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);
        pru_ram_write_data(offset, (uint8_t *)&regVal, 2,
            &pru_arm_iomap);
    }
    return status;
}

/*
 * suart routine for getting the number of bytes transfered
 */
int16_t pru_softuart_getTxDataLen(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t chNum;
    uint16_t u16ReadValue = 0;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Transmit channel number is (UartNum * 2) - 2  */

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u16ReadValue, 2, &pru_arm_iomap);

    u16ReadValue = ((u16ReadValue & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
        PRU_SUART_CH_CONFIG2_DATALEN_SHIFT);
    return u16ReadValue;
}

/*
 * suart routine for getting the number of bytes received
 */
int16_t pru_softuart_getRxDataLen(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t chNum;
    uint16_t u16ReadValue = 0;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }

        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u16ReadValue, 2, &pru_arm_iomap);

    u16ReadValue = ((u16ReadValue & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
        PRU_SUART_CH_CONFIG2_DATALEN_SHIFT);

    return u16ReadValue;
}

/*
 * suart routine to get the configuration information from a specific uart
 */
int16_t pru_softuart_getconfig(suart_handle hUart, suart_config *configUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t chNum;
    uint16_t regVal = 0;
    int16_t status = SUART_SUCCESS;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    /*
     * NOTE:
     * Dependent baud rate for the given UART, the value MUST BE LESS THAN OR
     * EQUAL TO 64, preScalarValue <= 64
     */

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;


        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Configuring the Transmit part of the given UART */
    /* Configuring TX serializer  */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->TXSerializer =
        ((regVal & PRU_SUART_CH_CTRL_SR_MASK) >>
            PRU_SUART_CH_CTRL_SR_SHIFT);

    /* Configuring TX prescalar value */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG1_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->txClkDivisor =
        ((regVal & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
            PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);

    /* Configuring TX bits per character value */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->txBitsPerChar =
        ((regVal & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
            PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);

    if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        chNum++;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Configuring the Transmit part of the given UART */
    /* Configuring RX serializer  */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->RXSerializer =
        ((regVal & PRU_SUART_CH_CTRL_SR_MASK) >>
            PRU_SUART_CH_CTRL_SR_SHIFT);

    /* Configuring RX prescalar value and Oversampling */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG1_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->rxClkDivisor =
        ((regVal & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
            PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);
    configUart->Oversampling =
        ((regVal & PRU_SUART_CH_CONFIG1_OVS_MASK) >>
            PRU_SUART_CH_CONFIG1_OVS_SHIFT);

    /* Configuring RX bits per character value */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    configUart->rxBitsPerChar =
        ((regVal & PRU_SUART_CH_CONFIG1_DIVISOR_MASK) >>
            PRU_SUART_CH_CONFIG1_DIVISOR_SHIFT);

    return status;
}


int32_t pru_softuart_pending_tx_request(void) {
    uint32_t offset = 0;
    uint32_t u32ISRValue = 0;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        return SUART_SUCCESS;
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        /* Read PRU Interrupt Status Register from PRU */
        offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATCLRINT1 &
                0xFFFF);
        pru_ram_read_data_4byte(offset, (uint32_t *)&u32ISRValue, 1);

        if ((u32ISRValue & 0x1) == 0x1) {
            return PRU_SUART_FAILURE;
        }
    } else {
        return PRU_MODE_INVALID;
    }

    return SUART_SUCCESS;
}

/*
 * suart data transmit routine
 */
int16_t pru_softuart_write
    (suart_handle hUart, uint32_t *ptTxDataBuf, uint16_t dataLen) {
    uint32_t offset = 0;
    uint32_t pruOffset;
    int16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint16_t regVal = 0;

    uint16_t pru_num;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
            pru_num = hUart->uartNum;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
            pru_num = hUart->uartNum;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        pru_num = 0;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Writing data length to SUART channel register */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    regVal &= ~PRU_SUART_CH_CONFIG2_DATALEN_MASK;
    regVal = regVal | (dataLen << PRU_SUART_CH_CONFIG2_DATALEN_SHIFT);
    pru_ram_write_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    /* Writing the data pointer to channel TX data pointer */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXDATA_OFFSET;
    pru_ram_write_data(offset, (uint8_t *)ptTxDataBuf, 4, &pru_arm_iomap);

    /* Service Request to PRU */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    regVal &= ~(PRU_SUART_CH_CTRL_MODE_MASK | PRU_SUART_CH_CTRL_SREQ_MASK);

    regVal |= (PRU_SUART_CH_CTRL_TX_MODE << PRU_SUART_CH_CTRL_MODE_SHIFT) |
        (PRU_SUART_CH_CTRL_SREQ << PRU_SUART_CH_CTRL_SREQ_SHIFT);

    pru_ram_write_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    /* generate ARM->PRU event */
    suart_arm_to_pru_intr(pru_num);

    return status;
}

/*
 * suart data receive routine
 */
int16_t pru_softuart_read
    (suart_handle hUart, uint32_t *ptDataBuf, uint16_t dataLen) {
    uint32_t offset = 0;
    uint32_t pruOffset;
    int16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint16_t regVal = 0;
    uint16_t pru_num;
    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
            pru_num = hUart->uartNum;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
            pru_num = hUart->uartNum;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        pru_num = 0;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Writing data length to SUART channel register */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    regVal &= ~PRU_SUART_CH_CONFIG2_DATALEN_MASK;
    regVal = regVal | (dataLen << PRU_SUART_CH_CONFIG2_DATALEN_SHIFT);
    pru_ram_write_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    /* Writing the data pointer to channel RX data pointer */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXDATA_OFFSET;
    pru_ram_write_data(offset, (uint8_t *)ptDataBuf, 4, &pru_arm_iomap);

    /* Service Request to PRU */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;


    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    regVal &= ~(PRU_SUART_CH_CTRL_MODE_MASK | PRU_SUART_CH_CTRL_SREQ_MASK);

    regVal |= (PRU_SUART_CH_CTRL_RX_MODE << PRU_SUART_CH_CTRL_MODE_SHIFT) |
        (PRU_SUART_CH_CTRL_SREQ << PRU_SUART_CH_CTRL_SREQ_SHIFT);

    pru_ram_write_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    /* enable the timeout interrupt */
    suart_intr_setmask(hUart->uartNum, PRU_RX_INTR, CHN_TXRX_IE_MASK_TIMEOUT);

    /* generate ARM->PRU event */
    suart_arm_to_pru_intr(pru_num);

    return status;
}

/*
 * suart routine to read the data from the RX FIFO
 */
int16_t pru_softuart_read_data(
    suart_handle hUart,
    uint8_t *pDataBuffer,
    int32_t s32MaxLen,
    uint32_t *pu32DataRead) {
    int16_t retVal = PRU_SUART_SUCCESS;
    uint8_t *pu8SrcAddr = NULL;
    uint32_t u32DataRead = 0;
    uint32_t u32DataLen = 0;
    uint32_t u32CharLen = 0;
    uint32_t offset = 0;
    uint32_t pruOffset;
    uint16_t chNum;
    uint16_t u16Status = 0;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Get the data pointer from channel RX data pointer */
    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXDATA_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&pu8SrcAddr, 4, &pru_arm_iomap);

    /* Reading data length from SUART channel register */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CONFIG2_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u32DataLen, 2, &pru_arm_iomap);

    /* read the character length */
    u32CharLen = u32DataLen & PRU_SUART_CH_CONFIG2_BITPERCHAR_MASK;
    u32CharLen -= 2;     /* remove the START & STOP bit */

    u32DataLen &= PRU_SUART_CH_CONFIG2_DATALEN_MASK;
    u32DataLen = u32DataLen >> PRU_SUART_CH_CONFIG2_DATALEN_SHIFT;
    u32DataLen++;

    /* if the character length is greater than 8, then the size doubles */
    if (u32CharLen > 8) {
        u32DataLen *= 2;
    }

    /* Check if the time-out had occured. If, yes, then we need to find the
     * number of bytes read from PRU. Else, we need to read the requested bytes
     */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u16Status, 1, &pru_arm_iomap);

    if (u16Status & CHN_TXRX_STATUS_TIMEOUT) {
        /* determine the number of bytes read into the FIFO */
        offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_BYTESDONECNTR_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&u32DataRead, 1, &pru_arm_iomap);

        /* if the character length is greater than 8, then the size doubles */
        if (u32CharLen > 8) {
            u32DataRead *= 2;
        }

        /* the data corresponding is loaded in second half during the timeout */
        if (u32DataRead > u32DataLen) {
            u32DataRead -= u32DataLen;
            pu8SrcAddr += u32DataLen;
        }

        pru_softuart_clrRxFifo(hUart);
    } else {
        u32DataRead = u32DataLen;
        /* Determine the buffer index by reading the FIFO_OddEven flag*/
        if (u16Status & CHN_TXRX_STATUS_CMPLT) {
            /* if the bit is set, the data is in the first half of the FIFO else
         * the data is in the second half
         */
            pu8SrcAddr += u32DataLen;
        }
    }

    /* we should be copying only max len given by the application */
    if (u32DataRead > (uint32_t)s32MaxLen) {
        u32DataRead = s32MaxLen;
    }

    /* evaluate the virtual address of the FIFO address based on the physical addr */
    pu8SrcAddr = (uint8_t *)((uint32_t)pu8SrcAddr - (uint32_t)pru_arm_iomap.pFifoBufferPhysBase +
        (uint32_t)pru_arm_iomap.pFifoBufferVirtBase);

    /* Now we have both the data length and the source address. copy */
    for (offset = 0; offset < u32DataRead; offset++)
    {
        *pDataBuffer++ = *pu8SrcAddr++;
    }
    *pu32DataRead = u32DataRead;

    retVal = PRU_SUART_SUCCESS;

    return retVal;
}

/*
 * suart routine to disable the receive functionality. This routine stops the PRU
 * from receiving on selected UART and also disables the McASP serializer corresponding
 * to this UART Rx line.
 */
int16_t pru_softuart_stopReceive(suart_handle hUart) {
    uint16_t status = SUART_SUCCESS;
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t chNum;
    uint16_t u16Status;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    /* read the existing value of status flag */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u16Status, 1, &pru_arm_iomap);

    /* we need to clear the busy bit corresponding to this receive channel */
    u16Status &= ~(CHN_TXRX_STATUS_RDY);
    pru_ram_write_data(offset, (uint8_t *)&u16Status, 1, &pru_arm_iomap);

    /* get the serizlizer number being used for this Rx channel */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&u16Status, 2, &pru_arm_iomap);
    u16Status &= PRU_SUART_CH_CTRL_SR_MASK;
    u16Status = u16Status >> PRU_SUART_CH_CTRL_SR_SHIFT;

    /* we need to de-activate the serializer corresponding to this receive */
    status = suart_asp_serializer_deactivate(u16Status, &pru_arm_iomap);

    return status;

}

/*
 * suart routine to get the tx status for a specific uart
 */
int16_t pru_softuart_getTxStatus(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t status = SUART_SUCCESS;
    uint16_t chNum;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;


        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);
    return status;
}

int16_t pru_softuart_clrTxStatus(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t status = SUART_SUCCESS;
    uint16_t chNum;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
    } else if (PRU0_MODE == PRU_MODE_TX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);

    status &= ~(0x2);
    pru_ram_write_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);
    return status;
}

/*
 * suart routine to get the rx status for a specific uart
 */
int16_t pru_softuart_getRxStatus(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t status = SUART_SUCCESS;
    uint16_t chNum;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);
    return status;
}

int16_t pru_softuart_clrRxFifo(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t status = SUART_SUCCESS;
    uint16_t chNum;
    uint16_t regVal;
    uint16_t uartNum;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    uartNum = hUart->uartNum;

    chNum = hUart->uartNum - 1;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        uartNum = 0;
    } else {
        return PRU_MODE_INVALID;
    }

    /* Reset the number of bytes read into the FIFO */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_BYTESDONECNTR_OFFSET;

    pru_ram_read_data(offset, (uint8_t *)&regVal, 1, &pru_arm_iomap);
    regVal &= 0x00;

    pru_ram_write_data(offset, (uint8_t *)&regVal, 1, &pru_arm_iomap);

    /* Service Request to PRU */
    offset = pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_CTRL_OFFSET;

    pru_ram_read_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);

    regVal &= ~(PRU_SUART_CH_CTRL_MODE_MASK | PRU_SUART_CH_CTRL_SREQ_MASK);

    regVal |= (PRU_SUART_CH_CTRL_RX_MODE << PRU_SUART_CH_CTRL_MODE_SHIFT) |
        (PRU_SUART_CH_CTRL_SREQ << PRU_SUART_CH_CTRL_SREQ_SHIFT);

    pru_ram_write_data(offset, (uint8_t *)&regVal, 2, &pru_arm_iomap);
    suart_intr_setmask(hUart->uartNum, PRU_RX_INTR, CHN_TXRX_IE_MASK_TIMEOUT);

    /* generate ARM->PRU event */
    suart_arm_to_pru_intr(uartNum);

    return status;
}


int16_t pru_softuart_clrRxStatus(suart_handle hUart) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t status = SUART_SUCCESS;
    uint16_t chNum;

    if (hUart == NULL) {
        return PRU_SUART_ERR_HANDLE_INVALID;
    }

    chNum = hUart->uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {

        /* channel starts from 0 and uart instance starts from 1 */
        chNum = (hUart->uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (hUart->uartNum <= 4) {
            /* PRU0 */
            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        } else {
            /* PRU1 */
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chNum -= 8;
        }
        chNum++;
    } else if (PRU0_MODE == PRU_MODE_RX_ONLY) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    offset =
        pruOffset + (chNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
        PRU_SUART_CH_TXRXSTATUS_OFFSET;
    pru_ram_read_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);

    status &= ~(0x3C);
    pru_ram_write_data(offset, (uint8_t *)&status, 1, &pru_arm_iomap);
    return status;
}

/*
 * suart_intr_status_read: Gets the Global Interrupt status register
 * for the specified SUART.
 * uartNum < 1 to 6 >
 * txrxFlag < Indicates TX or RX interrupt for the uart >
 */
int16_t pru_softuart_get_isrstatus(uint16_t uartNum, uint16_t *txrxFlag) {
    uint32_t u32IntcOffset;
    uint32_t chNum = 0xFF;
    uint32_t regVal = 0;
    uint32_t u32RegVal = 0;
    uint32_t u32ISRValue = 0;
    uint32_t u32AckRegVal = 0;
    uint32_t u32StatInxClrRegoffset = 0;

    /* initialize the status & Flag to known value */
    *txrxFlag = 0;

    u32StatInxClrRegoffset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATIDXCLR &
        0xFFFF);

    /* Read PRU Interrupt Status Register from PRU */
    u32IntcOffset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATCLRINT1 &
            0xFFFF);

    pru_ram_read_data_4byte(u32IntcOffset, (uint32_t *)&u32ISRValue, 1);

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chNum = uartNum * 2 - 2;

        /* Check if the interrupt occured for Tx */
        u32RegVal = PRU_SUART0_TX_EVT_BIT << ((uartNum - 1) * 2);
        if (u32ISRValue & u32RegVal) {
            /* interupt occured for TX */
            *txrxFlag |= PRU_TX_INTR;

            /* acknowledge the TX interrupt  */
            u32AckRegVal = chNum + PRU_SUART0_TX_EVT;
            pru_ram_write_data_4byte(u32StatInxClrRegoffset, (uint32_t *)&u32AckRegVal, 1);

        }

        /* Check if the interrupt occured for Rx */
        u32RegVal = PRU_SUART0_RX_EVT_BIT << ((uartNum - 1) * 2);
        pru_ram_read_data_4byte(u32IntcOffset, (uint32_t *)&u32ISRValue, 1);
        if (u32ISRValue & u32RegVal) {
            /* interupt occured for RX */
            *txrxFlag |= PRU_RX_INTR;
            chNum += 1;

            /* acknowledge the RX interrupt  */
            u32AckRegVal = chNum + PRU_SUART0_TX_EVT;
            pru_ram_write_data_4byte(u32StatInxClrRegoffset, (uint32_t *)&u32AckRegVal, 1);
        }
    } else {
        chNum = uartNum - 1;
        if ((u32ISRValue & 0x03FC) != 0) {
            /* PRU0 */
            u32RegVal = 1 << (uartNum + 1);
            if (u32ISRValue & u32RegVal) {
                /* acknowledge the interrupt  */
                u32AckRegVal = chNum + PRU_SUART0_TX_EVT;
                pru_ram_write_data_4byte(u32StatInxClrRegoffset, (uint32_t *)&u32AckRegVal, 1);
                *txrxFlag |= PRU_RX_INTR;
            }
        }

        pru_ram_read_data_4byte(u32IntcOffset, (uint32_t *)&u32ISRValue, 1);
        if (u32ISRValue & 0x3FC00) {
            /* PRU1 */
            u32RegVal = 1 << (uartNum + 9);
            if (u32ISRValue & u32RegVal) {
                /* acknowledge the interrupt  */
                u32AckRegVal = chNum + PRU_SUART4_TX_EVT;
                pru_ram_write_data_4byte(u32StatInxClrRegoffset, (uint32_t *)&u32AckRegVal, 1);
                *txrxFlag |= PRU_TX_INTR;
            }
        }

    }

    return regVal;
}

int32_t pru_intr_clr_isrstatus(uint16_t uartNum, uint32_t txrxmode) {
    uint32_t offset;
    uint16_t txrxFlag = 0;
    uint16_t chnNum;

    chnNum = uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chnNum = (uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if (uartNum <= 4) {
            /* PRU0 */
            offset = PRU_SUART_PRU0_ISR_OFFSET + 1;
        } else {
            /* PRU1 */
            offset = PRU_SUART_PRU1_ISR_OFFSET + 1;
            /* First 8 channel corresponds to PRU0 */
            chnNum -= 8;
        }

        if (2 == txrxmode) {
            chnNum++;
        }
    } else if (PRU0_MODE == txrxmode) {
        offset = PRU_SUART_PRU0_ISR_OFFSET + 1;
    } else {
        return PRU_MODE_INVALID;
    }

    pru_ram_read_data(offset, (uint8_t *)&txrxFlag, 1, &pru_arm_iomap);
    txrxFlag &= ~(0x2);
    pru_ram_write_data(offset, (uint8_t *)&txrxFlag, 1, &pru_arm_iomap);

    return 0;
}

int16_t suart_arm_to_pru_intr(uint16_t uartNum) {
    uint32_t u32offset;
    uint32_t u32value;
    int16_t s16retval;

    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        if ((uartNum > 0) && (uartNum <= 4)) {
            /* PRU0 SYS_EVT32 */
            u32value = 0x20;
        } else if ((uartNum > 4) && (uartNum <= 8)) {
            /* PRU1 SYS_EVT33 */
            u32value = 0x21;
        } else {
            return SUART_INVALID_UART_NUM;
        }
    }

    if ((PRU0_MODE == PRU_MODE_RX_ONLY) ||
        (PRU0_MODE == PRU_MODE_TX_ONLY)) {
        if (uartNum == PRU_NUM0) {
            /* PRU0 SYS_EVT32 */
            u32value = 0x20;
        }

        if (uartNum == PRU_NUM1) {
            /* PRU0 SYS_EVT33 */
            u32value = 0x21;
        }
    }

    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATIDXSET &
            0xFFFF);
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }
    return 0;
}

int16_t arm_to_pru_intr_init(void) {
    uint32_t u32offset;
    uint32_t u32value;
    uint32_t intOffset;
    int16_t s16retval = -1;
    #if 0
    /* Set the MCASP Event to PRU0 as Edge Triggered */
    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_TYPE0 &
            0xFFFF);
    u32value = 0x80000000;
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }
    #endif
    /* Clear all the host interrupts */
    for (intOffset = 0; intOffset <= PRU_INTC_HOSTINTLVL_MAX; intOffset++)
    {
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HSTINTENIDXCLR &
                0xFFFF);
        u32value = intOffset;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }
    }

    /* Enable the global interrupt */
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_GLBLEN &
        0xFFFF);
    u32value = 0x1;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }

    /* Enable the Host interrupts for all host channels */
    for (intOffset = 0; intOffset <= PRU_INTC_HOSTINTLVL_MAX; intOffset++)
    {
        u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HSTINTENIDXSET &
            0xFFFF);
        u32value = intOffset;
        s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }
    }

    /* host to channel mapping : Setting the host interrupt for channels 0,1,2,3 */
    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HOSTMAP0 &
            0xFFFF);
    u32value = 0x03020100;
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }

    /* host to channel mapping : Setting the host interrupt for channels 4,5,6,7 */
    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HOSTMAP1 &
            0xFFFF);
    u32value = 0x07060504;
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }

    /* host to channel mapping : Setting the host interrupt for channels 8,9 */
    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HOSTMAP2 &
            0xFFFF);
    u32value = 0x00000908;
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }

    /* Set the channel for System intrrupts
        * MAP Channel 0 to SYS_EVT31
        */
    u32offset =
        (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP7 &
            0xFFFF);
    u32value = 0x0000000000;
    s16retval =
        pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (-1 == s16retval) {
        return -1;
    }


    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* Sets the channel for the system interrupt
        * MAP channel 0 to SYS_EVT32
        * MAP channel 1 to SYS_EVT33
        * MAP channel 2 to SYS_EVT34  SUART0-Tx
        * MAP channel 2 to SYS_EVT35  SUART0-Rx
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP8 &
                0xFFFF);
        u32value = 0x02020100;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 3 to SYS_EVT36	SUART1-Tx
        * MAP channel 3 to SYS_EVT37    SUART1-Rx
        * MAP channel 4 to SYS_EVT38    SUART2-Tx
        * MAP channel 4 to SYS_EVT39    SUART2-Rx
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP9 &
                0xFFFF);
        u32value = 0x04040303;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 5 to SYS_EVT40	SUART3-Tx
        * MAP channel 5 to SYS_EVT41    SUART3-Rx
        * MAP channel 6 to SYS_EVT42    SUART4-Tx
        * MAP channel 6 to SYS_EVT43    SUART4-Rx
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP10 &
                0xFFFF);
        u32value = 0x06060505;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 7 to SYS_EVT44	SUART5-Tx
        * MAP channel 7 to SYS_EVT45    SUART5-Rx
        * MAP channel 8 to SYS_EVT46    SUART6-Tx
        * MAP channel 8 to SYS_EVT47    SUART6-Rx
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP11 &
                0xFFFF);
        u32value = 0x08080707;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 9 to SYS_EVT48	SUART7-Tx
        * MAP channel 9 to SYS_EVT49    SUART7-Rx
        * MAP Channel 1 to SYS_EVT50
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP12 &
                0xFFFF);
        u32value = 0x00010909;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }
    }

    if ((PRU0_MODE == PRU_MODE_RX_ONLY) ||
        (PRU0_MODE == PRU_MODE_TX_ONLY)) {
        /* Sets the channel for the system interrupt
        * MAP channel 0 to SYS_EVT32
        * MAP channel 1 to SYS_EVT33
        * MAP channel 2 to SYS_EVT34  SUART0
        * MAP channel 3 to SYS_EVT35  SUART1
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP8 &
                0xFFFF);
        u32value = 0x03020100;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 4 to SYS_EVT36	SUART2
        * MAP channel 5 to SYS_EVT37    SUART3
        * MAP channel 6 to SYS_EVT38    SUART4
        * MAP channel 7 to SYS_EVT39    SUART5
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP9 &
                0xFFFF);
        u32value = 0x07060504;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 8 to SYS_EVT40	SUART6
        * MAP channel 9 to SYS_EVT41    SUART7
        * MAP channel 2 to SYS_EVT42    SUART0
        * MAP channel 3 to SYS_EVT43    SUART1
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP10 &
                0xFFFF);
        u32value = 0x03020908;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 4 to SYS_EVT44	SUART2
        * MAP channel 5 to SYS_EVT45    SUART3
        * MAP channel 6 to SYS_EVT46    SUART4
        * MAP channel 7 to SYS_EVT47    SUART5
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP11 &
                0xFFFF);
        u32value = 0x07060504;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }

        /* Sets the channel for the system interrupt
        * MAP channel 8 to SYS_EVT48	SUART6
        * MAP channel 9 to SYS_EVT49    SUART7
        * MAP Channel 1 to SYS_EVT50    PRU to PRU Intr
        */
        u32offset =
            (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_CHANMAP12 &
                0xFFFF);
        u32value = 0x00010908;
        s16retval =
            pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (-1 == s16retval) {
            return -1;
        }
    }

    /* Clear required set of system events and enable them using indexed register */
    for (intOffset = 0; intOffset < 18; intOffset++)
    {
        u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_STATIDXCLR & 0xFFFF);
        u32value = 32 + intOffset;
        s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }

    }
    /* enable only the HOST to PRU interrupts and let the PRU to Host events be
     * enabled by the separate API on demand basis.
     */
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXSET & 0xFFFF);
    u32value = 31;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXSET & 0xFFFF);
    u32value = 32;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXSET & 0xFFFF);
    u32value = 33;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXSET & 0xFFFF);
    u32value = 50;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }

    /* Enable the global interrupt */
    u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_GLBLEN &
        0xFFFF);
    u32value = 0x1;
    s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
    if (s16retval == -1) {
        return -1;
    }

    /* Enable the Host interrupts for all host channels */
    for (intOffset = 0; intOffset <= PRU_INTC_HOSTINTLVL_MAX; intOffset++)
    {
        u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_HSTINTENIDXSET &
            0xFFFF);
        u32value = intOffset;
        s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }
    }

    return 0;
}

int32_t suart_pru_to_host_intr_enable(uint16_t uartNum,
    uint32_t txrxmode, int32_t s32Flag) {
    int32_t retVal = 0;
    uint32_t u32offset;
    uint32_t chnNum;
    uint32_t u32value;
    int16_t s16retval = 0;

    if (uartNum > 8) {
        return SUART_INVALID_UART_NUM;
    }

    chnNum = uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        chnNum = (uartNum * 2) - 2;
        if (2 == txrxmode) {            /* Rx mode */
            chnNum++;
        }
        u32value = 34 + chnNum;
    } else if ((PRU_MODE_RX_ONLY == txrxmode) && (PRU0_MODE == PRU_MODE_RX_ONLY)) {
        u32value = 34 + chnNum;
    } else if ((PRU_MODE_TX_ONLY == txrxmode) && (PRU0_MODE == PRU_MODE_TX_ONLY)) {
        u32value = 34 + chnNum;
    } else {
        return -1;
    }

    if (true == s32Flag) {
        u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXSET & 0xFFFF);
        s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }
    } else {
        u32offset = (uint32_t)pru_arm_iomap.pru_io_addr | (PRU_INTC_ENIDXCLR & 0xFFFF);
        s16retval = pru_ram_write_data_4byte(u32offset, (uint32_t *)&u32value, 1);
        if (s16retval == -1) {
            return -1;
        }
    }
    return retVal;
}

int32_t suart_intr_setmask(uint16_t uartNum,
    uint32_t txrxmode, uint32_t intrmask) {
    uint32_t offset;
    uint32_t pruOffset;
    uint32_t txrxFlag;
    uint32_t regval = 0;
    uint32_t chnNum;

    chnNum = uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chnNum = (uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if ((uartNum > 0) && (uartNum <= 4)) {

            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
            offset = PRU_SUART_PRU0_IMR_OFFSET;
        } else if ((uartNum > 4) && (uartNum <= 8)) {
            /* PRU1 */
            offset = PRU_SUART_PRU1_IMR_OFFSET;
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chnNum -= 8;
        } else {
            return SUART_INVALID_UART_NUM;
        }

        if (2 == txrxmode) {            /* rx mode */
            chnNum++;
        }
    } else if (PRU0_MODE == txrxmode) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        offset = PRU_SUART_PRU0_IMR_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }


    regval = 1 << chnNum;

    if (CHN_TXRX_IE_MASK_CMPLT == (intrmask & CHN_TXRX_IE_MASK_CMPLT)) {
        pru_ram_read_data(offset, (uint8_t *)&txrxFlag, 2,
            &pru_arm_iomap);
        txrxFlag &= ~(regval);
        txrxFlag |= regval;
        pru_ram_write_data(offset, (uint8_t *)&txrxFlag, 2,
            &pru_arm_iomap);
    }

    if ((intrmask & SUART_GBL_INTR_ERR_MASK) == SUART_GBL_INTR_ERR_MASK) {
        regval = 0;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(SUART_GBL_INTR_ERR_MASK);
        regval |= (SUART_GBL_INTR_ERR_MASK);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);

    }
    /* Break Indicator Interrupt Masked */
    if ((intrmask & CHN_TXRX_IE_MASK_FE) == CHN_TXRX_IE_MASK_FE) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_FE);
        regval |= CHN_TXRX_IE_MASK_FE;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    /* Framing Error Interrupt Masked */
    if (CHN_TXRX_IE_MASK_BI == (intrmask & CHN_TXRX_IE_MASK_BI)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_BI);
        regval |= CHN_TXRX_IE_MASK_BI;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    /* Timeout error Interrupt Masked */
    if (CHN_TXRX_IE_MASK_TIMEOUT == (intrmask & CHN_TXRX_IE_MASK_TIMEOUT)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_TIMEOUT);
        regval |= CHN_TXRX_IE_MASK_TIMEOUT;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    /* Overrun error Interrupt Masked */
    if (CHN_RX_IE_MASK_OVRN == (intrmask & CHN_RX_IE_MASK_OVRN)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_RX_IE_MASK_OVRN);
        regval |= CHN_RX_IE_MASK_OVRN;
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    return 0;
}

int32_t suart_intr_clrmask(uint16_t uartNum,
    uint32_t txrxmode, uint32_t intrmask) {
    uint32_t offset;
    uint32_t pruOffset;
    uint16_t txrxFlag;
    uint16_t regval = 0;
    uint16_t chnNum;

    chnNum = uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chnNum = (uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if ((uartNum > 0) && (uartNum <= 4)) {

            pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
            offset = PRU_SUART_PRU0_IMR_OFFSET;
        } else if ((uartNum > 4) && (uartNum <= 8)) {
            /* PRU1 */
            offset = PRU_SUART_PRU1_IMR_OFFSET;
            pruOffset = PRU_SUART_PRU1_CH0_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chnNum -= 8;
        } else {
            return SUART_INVALID_UART_NUM;
        }

        if (2 == txrxmode) {            /* rx mode */
            chnNum++;
        }
    } else if (PRU0_MODE == txrxmode) {
        pruOffset = PRU_SUART_PRU0_CH0_OFFSET;
        offset = PRU_SUART_PRU0_IMR_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    regval = 1 << chnNum;

    if (CHN_TXRX_IE_MASK_CMPLT == (intrmask & CHN_TXRX_IE_MASK_CMPLT)) {
        pru_ram_read_data(offset, (uint8_t *)&txrxFlag, 2,
            &pru_arm_iomap);
        txrxFlag &= ~(regval);
        pru_ram_write_data(offset, (uint8_t *)&txrxFlag, 2,
            &pru_arm_iomap);
    }

    if ((intrmask & SUART_GBL_INTR_ERR_MASK) == SUART_GBL_INTR_ERR_MASK) {
        regval = 0;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(SUART_GBL_INTR_ERR_MASK);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);

    }
    /* Break Indicator Interrupt Masked */
    if ((intrmask & CHN_TXRX_IE_MASK_FE) == CHN_TXRX_IE_MASK_FE) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;
        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_FE);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    /* Framing Error Interrupt Masked */
    if (CHN_TXRX_IE_MASK_BI == (intrmask & CHN_TXRX_IE_MASK_BI)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_BI);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }

    /* Timeout error Interrupt Masked */
    if (CHN_TXRX_IE_MASK_TIMEOUT == (intrmask & CHN_TXRX_IE_MASK_TIMEOUT)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_TXRX_IE_MASK_TIMEOUT);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    /* Overrun error Interrupt Masked */
    if (CHN_RX_IE_MASK_OVRN == (intrmask & CHN_RX_IE_MASK_OVRN)) {
        regval = 0;
        offset =
            pruOffset + (chnNum * SUART_NUM_OF_BYTES_PER_CHANNEL) +
            PRU_SUART_CH_CONFIG1_OFFSET;

        pru_ram_read_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
        regval &= ~(CHN_RX_IE_MASK_OVRN);
        pru_ram_write_data(offset, (uint8_t *)&regval, 2,
            &pru_arm_iomap);
    }
    return 0;
}

int32_t suart_intr_getmask(uint16_t uartNum,
    uint32_t txrxmode, uint32_t intrmask) {
    uint16_t chnNum;
    uint32_t offset;
    uint16_t txrxFlag;
    uint16_t regval = 1;

    chnNum = uartNum - 1;
    if ((PRU0_MODE == PRU_MODE_RX_TX_BOTH)) {
        /* channel starts from 0 and uart instance starts from 1 */
        chnNum = (uartNum * SUART_NUM_OF_CHANNELS_PER_SUART) - 2;

        if ((uartNum > 0) && (uartNum <= 4)) {

            offset = PRU_SUART_PRU0_IMR_OFFSET;
        } else if ((uartNum > 4) && (uartNum <= 8)) {
            /* PRU1 */
            offset = PRU_SUART_PRU1_IMR_OFFSET;
            /* First 8 channel corresponds to PRU0 */
            chnNum -= 8;
        } else {
            return SUART_INVALID_UART_NUM;
        }

        if (2 == txrxmode) {            /* rx mode */
            chnNum++;
        }
    } else if (PRU0_MODE == txrxmode) {
        offset = PRU_SUART_PRU0_IMR_OFFSET;
    } else {
        return PRU_MODE_INVALID;
    }

    regval = regval << chnNum;

    pru_ram_read_data(offset, (uint8_t *)&txrxFlag, 2, &pru_arm_iomap);
    txrxFlag &= regval;
    if (0 == intrmask) {
        if (txrxFlag == 0) {
            return 1;
        }
    }

    if (1 == intrmask) {
        if (txrxFlag == regval) {
            return 1;
        }
    }
    return 0;
}

static int32_t suart_set_pru_id(uint32_t pru_no) {
    uint32_t offset;
    uint16_t reg_val = 0;

    if (0 == pru_no) {
        offset = PRU_SUART_PRU0_ID_ADDR;
    } else if (1 == pru_no) {
        offset = PRU_SUART_PRU1_ID_ADDR;
    } else {
        return PRU_SUART_FAILURE;
    }

    pru_ram_read_data(offset, (uint8_t *)&reg_val, 1, &pru_arm_iomap);
    reg_val &= ~SUART_PRU_ID_MASK;
    reg_val = pru_no;
    pru_ram_write_data(offset, (uint8_t *)&reg_val, 1, &pru_arm_iomap);

    return PRU_SUART_SUCCESS;
}
/* End of file */
