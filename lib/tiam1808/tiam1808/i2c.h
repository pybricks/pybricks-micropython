/**
 *  \file       i2c.h
 *
 *  \brief      This file contains the function prototypes for the device
 *              abstraction layer for I2C. It also contains some
 *              related macro definitions and some files to be included.
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


#ifndef _I2C_H_
#define _I2C_H_

#include "hw_i2c.h"
#ifdef __cplusplus
extern "C" {
#endif

/******************Macros to confiure I2C********************************************/

#define I2C_CFG_MST_TX                  (I2C_ICMDR_MST |I2C_ICMDR_TRX) 

#define I2C_CFG_MST_RX                  (I2C_ICMDR_MST |(I2C_ICMDR_TRX & 0x00000000))

#define I2C_CFG_NACKMOD                 I2C_ICMDR_NACKMOD

#define I2C_CFG_START                   I2C_ICMDR_STT

#define I2C_CFG_STOP                    I2C_ICMDR_STP

#define I2C_CFG_7BIT_ADDR_MODE          (I2C_ICMDR_XA & 0x00000000)

#define I2C_CFG_10BIT_ADDR_MODE         I2C_ICMDR_XA

#define I2C_CFG_FREE_FORMAT_DATA        I2C_ICMDR_FDF

#define I2C_CFG_SLAVE_STT               (I2C_ICMDR_TRX |I2C_ICMDR_STT\
                                        (I2C_ICMDR_MST & 0x00000000))

#define I2C_CFG_RESET                   I2C_ICMDR_IRS

#define I2C_CFG_LOOP_BACK               I2C_ICMDR_DLB

#define I2C_CFG_REPEAT_MODE             I2C_ICMDR_RM

/**********MACROS TO Clear Status of the Interrupts************************************/

#define I2C_CLEAR_ARBITARTION_LOST      I2C_ICSTR_AL

#define I2C_CLEAR_NO_ACK                I2C_ICSTR_NACK

#define I2C_CLEAR_ADDR_READY            I2C_ICSTR_ARDY

#define I2C_CLEAR_DATA_READY            I2C_ICSTR_ICRRDY

#define I2C_CLEAR_TRANSMIT_READY        I2C_ICSTR_ICXRDY

#define I2C_CLEAR_STOP_CONDITION        I2C_ICSTR_SCD

#define I2C_CLEAR_BUS_BUSY              I2C_ICSTR_BB

#define I2C_CLEAR_NO_ACK_SENT           I2C_ICSTR_NACKSNT

/**********MACROS To Enable or Disable Interrupts****************************************/

#define I2C_INT_ARBITARTION_LOST       I2C_ICIMR_AL

#define I2C_INT_NO_ACK                 I2C_ICIMR_NACK

#define I2C_INT_ADDR_READY             I2C_ICIMR_ARDY

#define I2C_INT_DATA_READY             I2C_ICIMR_ICRRDY

#define I2C_INT_TRANSMIT_READY         I2C_ICIMR_ICXRDY

#define I2C_INT_STOP_CONDITION         I2C_ICSTR_SCD

#define I2C_INT_ADDR_SLAVE             I2C_ICIMR_AAS

/*********MACRO of interrupt code******************************************************/
#define I2C_INTCODE_TX_READY           I2C_ICIVR_INTCODE_ICXRDY

#define I2C_INTCODE_RX_READY           I2C_ICIVR_INTCODE_ICRRDY

#define I2C_INTCODE_STOP               I2C_ICIVR_INTCODE_SCD

#define I2C_INTCODE_NACK               I2C_ICIVR_INTCODE_NACK

#define I2C_INTCODE_AAS                I2C_ICIVR_INTCODE_AAS

#define I2C_INTCODE_AL                 I2C_ICIVR_INTCODE_AL 
/**************************************************************************************/
#define I2C_DMA_TX_ENABLE              (0x01 << I2C_ICDMAC_TXDMAEN_SHIFT)
#define I2C_DMA_RX_ENABLE              I2C_ICDMAC_RXDMAEN      

/*********Prototype of driver API*****************************************************/

extern void I2CMasterInitExpClk(unsigned int baseAdd, unsigned int inputClk,
                                unsigned int scaledClk, unsigned int outputClk);
extern void I2CMasterEnable(unsigned int baseAddr);
extern void I2CMasterDisable(unsigned int baseAddr);
extern void I2CMasterIntEnableEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CSlaveIntEnableEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CMasterIntDisableEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CSlaveIntDisableEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CMasterIntClearEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CSlaveIntClearEx(unsigned int baseAddr, unsigned int intFlag);
extern void I2CMasterSlaveAddrSet(unsigned int baseAddr, unsigned int slaveAddr);
extern void I2CMasterControl(unsigned int baseAddr, unsigned int cmd);
extern void I2CMasterDataPut(unsigned int baseAddr, unsigned char data);
extern void I2CSlaveDataPut(unsigned int baseAddr, unsigned char data);
extern unsigned int I2CMasterIntStatus(unsigned int baseAddr);
extern unsigned int I2CSlaveIntStatus(unsigned int baseAddr);
extern unsigned int I2CSlaveIntStatusEx(unsigned int baseAddr,unsigned int intFlag);
extern unsigned int  I2CMasterBusBusy(unsigned int  baseAddr);
extern unsigned int I2CMasterDataGet(unsigned int baseAddr);
extern unsigned int I2CSlaveDataGet(unsigned int baseAddr);
extern unsigned int I2CMasterErr(unsigned int baseAddr);
extern void I2COwnAddressSet(unsigned int baseAddr);
extern void I2CIsr(void);
extern void I2CSetDataCount(unsigned int baseAddr, unsigned int count);
extern void I2CMasterStart(unsigned int baseAddr);
extern unsigned int I2CInterruptVectorGet(unsigned int baseAddr);
extern void I2CDMATxRxEventDisable(unsigned int baseAddr);
extern void I2CDMATxEventEnable(unsigned int baseAddr);
extern void I2CDMARxEventEnable(unsigned int baseAddr);
extern void I2CDMATxEventDisable(unsigned int baseAddr);
extern void I2CDMARxEventDisable(unsigned int baseAddr);
extern unsigned int I2CMasterIsBusy(unsigned int baseAddr);
extern void I2CStatusClear(unsigned int baseAddr, unsigned int status);
extern void I2CMasterStop(unsigned int baseAddr);
extern unsigned int I2CSlaveAddressGet(unsigned int baseAddr);

#ifdef __cplusplus
}
#endif
#endif

