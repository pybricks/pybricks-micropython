/**
 *  \file   emac.h
 *
 *  \brief  EMAC APIs and macros.
 *
 *   This file contains the driver API prototypes and macro definitions.
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

#ifndef __EMAC_H__
#define __EMAC_H__

#include "hw_emac.h"
#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/*
** Macros which can be used as speed parameter to the API EMACRMIISpeedSet
*/
#define EMAC_RMIISPEED_10MBPS                 (0x00000000u)
#define EMAC_RMIISPEED_100MBPS                (0x00008000u)

/*
** Macros which can be used as duplexMode parameter to the API 
** EMACDuplexSet
*/
#define EMAC_DUPLEX_FULL                      (0x00000001u)
#define EMAC_DUPLEX_HALF                      (0x00000000u)

/*
** Macros which can be used as matchFilt  parameters to the API 
** EMACMACAddrSet
*/
/* Address not used to match/filter incoming packets */
#define EMAC_MACADDR_NO_MATCH_NO_FILTER       (0x00000000u)

/* Address will be used to filter incoming packets */
#define EMAC_MACADDR_FILTER                   (0x00100000u)

/* Address will be used to match incoming packets */
#define EMAC_MACADDR_MATCH                    (0x00180000u)

/*
** Macros which can be passed as eoiFlag to EMACRxIntAckToClear API
*/
#define EMAC_INT_CORE0_RX                     (0x1u)
#define EMAC_INT_CORE1_RX                     (0x5u)
#define EMAC_INT_CORE2_RX                     (0x9u)

/*
** Macros which can be passed as eoiFlag to EMACTxIntAckToClear API
*/
#define EMAC_INT_CORE0_TX                     (0x2u)
#define EMAC_INT_CORE1_TX                     (0x6u)
#define EMAC_INT_CORE2_TX                     (0xAu)

/*****************************************************************************/
/*
** Prototypes for the APIs
*/
extern void EMACTxIntPulseEnable(unsigned int emacBase, unsigned int emacCtrlBase,
                                 unsigned int ctrlCore, unsigned int channel);
extern void EMACTxIntPulseDisable(unsigned int emacBase, unsigned int emacCtrlBase,
                                  unsigned int ctrlCore, unsigned int channel);
extern void EMACRxIntPulseEnable(unsigned int emacBase, unsigned int emacCtrlBase,
                                 unsigned int ctrlCore, unsigned int channel);
extern void EMACRxIntPulseDisable(unsigned int emacBase, unsigned int emacCtrlBase,
                                   unsigned int ctrlCore, unsigned int channel);
extern void EMACRMIISpeedSet(unsigned int emacBase, unsigned int speed);
extern void EMACDuplexSet(unsigned int emacBase, unsigned int duplexMode);
extern void EMACTxEnable(unsigned int emacBase);
extern void EMACRxEnable(unsigned int emacBase);
extern void EMACTxHdrDescPtrWrite(unsigned int emacBase, unsigned int descHdr,
                                  unsigned int channel);
extern void EMACRxHdrDescPtrWrite(unsigned int emacBase, unsigned int descHdr,
                                  unsigned int channel);
extern void EMACInit(unsigned int emacCtrlBase, unsigned int emacBase);
extern void EMACMACSrcAddrSet(unsigned int emacBase, unsigned char *macAddr);
extern void EMACMACAddrSet(unsigned int emacBase, unsigned int channel,
                           unsigned char *macAddr, unsigned int matchFilt);
extern void EMACMIIEnable(unsigned int emacBase);
extern void EMACRxUnicastSet(unsigned int emacBase, unsigned int channel);
extern void EMACCoreIntAck(unsigned int emacBase, unsigned int eoiFlag);
extern void EMACTxCPWrite(unsigned int emacBase, unsigned int channel,
                          unsigned int comPtr);
extern void EMACRxCPWrite(unsigned int emacBase, unsigned int channel,
                          unsigned int comPtr);
extern void EMACRxBroadCastEnable(unsigned int emacBase, unsigned int channel);
extern void EMACNumFreeBufSet(unsigned int emacBase, unsigned int channel,
                              unsigned int nBuf);
extern unsigned int EMACIntVectorGet(unsigned int emacBase);

#ifdef __cplusplus
}
#endif

#endif /* __EMAC_H__ */
