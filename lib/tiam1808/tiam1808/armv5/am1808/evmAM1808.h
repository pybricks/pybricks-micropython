/**
 *  \file   evmAM1808.h
 *
 *  \brief  This file contains the board specific function prototypes for use
 *          by applications
 */

/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/ 
*
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
*/

#ifndef __EVMAM1808_H__
#define __EVMAM1808_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
**                      EXTERNAL FUNCTION PROTOTYPES
*******************************************************************************/

extern unsigned int LCDVersionGet(void);
extern void UARTPinMuxSetup(unsigned int instanceNum,
                            unsigned int modemCtrlChoice);
extern void RTCPinMuxSetup(unsigned int alarmPinReqd);
extern void SPI0CSPinMuxSetup(unsigned int csPinNum);
extern void SPI1CSPinMuxSetup(unsigned int csPinNum); 
extern void I2CPinMuxSetup(unsigned int instanceNum);
extern void SPIPinMuxSetup(unsigned int instanceNum);
extern void ConfigRasterDisplayEnable(void);
extern void GPIOBank4Pin0PinMuxSetup(void);
extern void SysCfgRegistersUnlock(void);
extern void SysCfgRegistersLock(void);
extern void EHRPWM0PinMuxSetup(void);
extern void EHRPWM1PinMuxSetup(void);
extern void LIDDDisplayEnable(void);
extern void McASPPinMuxSetup(void);
extern void EMACPinMuxSetup(void);
extern void LIDDPinMuxSetup(void);
extern void LCDPinMuxSetup(void);
extern void NANDPinMuxSetup(void);
extern void EMIFAClkConfig(void);
extern void VPIFPinMuxSetup(void);

#ifdef __cplusplus
}
#endif
#endif
