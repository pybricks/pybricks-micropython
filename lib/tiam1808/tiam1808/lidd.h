/**
 *  \file    lidd.h
 *
 *  \brief    Definitions used for LIDD driver
 *
 *  This file contains the LIDD Device Abstraction Layer API prototypes
 *  and user interface macro definitions
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef _LIDD_H_
#define _LIDD_H_

#include "hw_lcdc.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
 *                                 MACRO DEFINTIONS
 ******************************************************************************/

/* Disable/ enable LIDD Done interrupt */
#define LIDD_DONE_INT_ENABLE            (LCDC_LIDD_CTRL_DONE_INT_EN)
#define LIDD_DONE_INT_DISABLE           (0)

/* Interrupt status definitions */
#define LIDD_FRAME_DONE_INT_STAT         LCDC_LCD_STAT_DONE

#define LIDD_SYNC_LOST_INT_STAT          LCDC_LCD_STAT_SYNC

#define LIDD_ACBIAS_COUNT_INT_STAT       LCDC_LCD_STAT_ABC

#define LIDD_FIFO_UNDERFLOW_INT_STAT     LCDC_LCD_STAT_FUF

#define LIDD_PALETTE_LOADED_INT_STAT     LCDC_LCD_STAT_PL

#define LIDD_END_OF_FRAME0_INT_STAT      LCDC_LCD_STAT_EOF0

#define LIDD_END_OF_FRAME1_INT_STAT      LCDC_LCD_STAT_EOF1

/* LIDD DMA control */
#define LIDD_DMA_ENABLE                  (LCDC_LIDD_CTRL_LIDD_DMA_EN)
#define LIDD_DMA_DISABLE                 (0)

/* LIDD CSn polarity control */
#define LIDD_CS0_ACTIVE_LOW              (0)
#define LIDD_CS0_ACTIVE_HIGH             (1 << LCDC_LIDD_CTRL_CS0_E0_POL_SHIFT)
#define LIDD_CS1_ACTIVE_LOW              (0)
#define LIDD_CS1_ACTIVE_HIGH             (1 << LCDC_LIDD_CTRL_CS1_E1_POL_SHIFT)

/* LIDD Strobe polarity control */
#define LIDD_WSTROBE_ACTIVE_LOW          (0)
#define LIDD_WSTROBE_ACTIVE_HIGH         (1 << LCDC_LIDD_CTRL_WS_DIR_POL_SHIFT)
#define LIDD_RSTROBE_ACTIVE_LOW          (0)
#define LIDD_RSTROBE_ACTIVE_HIGH         (1 << LCDC_LIDD_CTRL_RS_EN_POL_SHIFT)

/* LIDD ALE polarity control */
#define LIDD_ALE_ACTIVE_LOW              (0)
#define LIDD_ALE_ACTIVE_HIGH             (1 << LCDC_LIDD_CTRL_ALEPOL_SHIFT)

/* LIDD Mode Selection */
#define LIDD_MODE_SYNC_MPU68             (LCDC_LIDD_CTRL_LIDD_MODE_SEL_SYNC_MPU68 \
                                          << LCDC_LIDD_CTRL_LIDD_MODE_SEL_SHIFT)
#define LIDD_MODE_ASYNC_MPU68            (LCDC_LIDD_CTRL_LIDD_MODE_SEL_ASYNC_MPU68 \
                                          << LCDC_LIDD_CTRL_LIDD_MODE_SEL_SHIFT)
#define LIDD_MODE_SYNC_MPU80             (LCDC_LIDD_CTRL_LIDD_MODE_SEL_SYNC_MPU80 \
                                             << LCDC_LIDD_CTRL_LIDD_MODE_SEL_SHIFT)
#define LIDD_MODE_ASYNC_MPU80            (LCDC_LIDD_CTRL_LIDD_MODE_SEL_ASYNC_MPU80 \
                                          << LCDC_LIDD_CTRL_LIDD_MODE_SEL_SHIFT)
#define LIDD_MODE_HITACHI                (LCDC_LIDD_CTRL_LIDD_MODE_SEL_HITACHI \
                                          << LCDC_LIDD_CTRL_LIDD_MODE_SEL_SHIFT)

/* LIDD CS/Strobe/Enable Polarity control */
#define LIDD_CS_STROBE_POLARITY(cs1, cs0, ws, rs, ale) ((unsigned int) \
                                                        (cs1 | cs0 | ws | rs | ale))
/* LIDD CSn Timing configuration */
#define LIDD_CS_CONF(wsu, ws, wh, rsu, rs, rh, ta)   ((unsigned int) \
                                                     (((wsu & 0x1F) << LCDC_LIDD_CS0_CONF_W_SU_SHIFT) | \
                                                      ((ws & 0x3F) << LCDC_LIDD_CS0_CONF_W_STROBE_SHIFT) | \
                                                      ((wh & 0xF) << LCDC_LIDD_CS0_CONF_W_HOLD_SHIFT) | \
                                                      ((rsu & 0x1F) << LCDC_LIDD_CS0_CONF_R_SU_SHIFT) | \
                                                      ((rs & 0x3F) << LCDC_LIDD_CS0_CONF_R_STROBE_SHIFT) | \
                                                      ((rh & 0xF) << LCDC_LIDD_CS0_CONF_R_HOLD_SHIFT) | \
                                                      ((ta & 0x3) << LCDC_LIDD_CS0_CONF_TA_SHIFT)))


/* LIDD DMA configuration */
#define LIDD_DMA_CONFIG(thres, burst, endian)        ((unsigned int) \
                                                     (((thres & 0x7) << LCDC_LCDDMA_CTRL_TH_FIFO_READY_SHIFT) | \
                                                      ((burst & 0x7) << LCDC_LCDDMA_CTRL_BURST_SIZE_SHIFT) | \
                                                      ((endian & 0x1) << LCDC_LCDDMA_CTRL_BIGENDIAN_SHIFT)))

/* Function prototypes */
void LIDDStringWrite(unsigned int baseAddr, unsigned int cs, unsigned int start, char *data, unsigned int len);
void LIDDDMAConfigSet(unsigned int baseAddr, unsigned int dmaEnable, unsigned int doneEnable);
void LIDDClkConfig(unsigned int baseAddr, unsigned int freq, unsigned int moduleFreq);
void LIDDCSTimingConfig(unsigned int baseAddr, unsigned int cs, unsigned int conf);
void LIDDAddrIndexSet(unsigned int baseAddr, unsigned int cs, unsigned int index);
void LIDDDataWrite(unsigned int baseAddr, unsigned int cs, unsigned int data);
void LIDDPolaritySet(unsigned int baseAddr, unsigned int polarity);
void LIDDModeSet(unsigned int baseAddr, unsigned int mode);
void LIDDDMACSSet(unsigned int baseAddr, unsigned int cs);
unsigned int LIDDStatusGet(unsigned int baseAddr);
#ifdef __cplusplus
}
#endif

#endif
