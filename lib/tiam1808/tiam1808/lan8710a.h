/**
 * \file  lan8710a.h
 *
 * \brief Macros and function definitions for LAN8710A PHY
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

#ifndef _LAN8710_H_
#define _LAN8710_H_

#ifdef __cplusplus
extern "C" {
#endif
/* PHY register offset definitions */
#define PHY_BCR                           (0u)
#define PHY_BSR                           (1u)
#define PHY_ID1                           (2u)
#define PHY_ID2                           (3u)
#define PHY_AUTONEG_ADV                   (4u)
#define PHY_LINK_PARTNER_ABLTY            (5u)

/* PHY status definitions */
#define PHY_ID_SHIFT                      (16u)
#define PHY_SOFTRESET                     (0x8000)
#define PHY_AUTONEG_ENABLE                (0x1000u)
#define PHY_AUTONEG_RESTART               (0x0200u)
#define PHY_AUTONEG_COMPLETE              (0x0020u)
#define PHY_AUTONEG_INCOMPLETE            (0x0000u)
#define PHY_AUTONEG_STATUS                (0x0020u)
#define PHY_AUTONEG_ABLE                  (0x0008u)
#define PHY_LPBK_ENABLE                   (0x4000u)
#define PHY_LINK_STATUS                   (0x0004u)



/* PHY ID. The LSB nibble will vary between different phy revisions */
#define LAN8710A_PHY_ID                   (0x0007C0F0u)
#define LAN8710A_PHY_ID_REV_MASK          (0x0000000Fu)

/* Pause operations */
#define LAN8710A_PAUSE_NIL                (0x0000u)
#define LAN8710A_PAUSE_SYM                (0x0400u)
#define LAN8710A_PAUSE_ASYM               (0x0800u)
#define LAN8710A_PAUSE_BOTH_SYM_ASYM      (0x0C00u)

/* 100 Base TX Full Duplex capablity */
#define LAN8710A_100BTX_HD                (0x0000u)
#define LAN8710A_100BTX_FD                (0x0100u)

/* 100 Base TX capability */
#define LAN8710A_NO_100BTX                (0x0000u)
#define LAN8710A_100BTX                   (0x0080u)

/* 10 BaseT duplex capabilities */
#define LAN8710A_10BT_HD                  (0x0000u)
#define LAN8710A_10BT_FD                  (0x0040u)

/* 10 BaseT ability*/
#define LAN8710A_NO_10BT                  (0x0000u)
#define LAN8710A_10BT                     (0x0020u)

/**************************************************************************
                        API function Prototypes
**************************************************************************/
extern unsigned int Lan8710aIDGet(unsigned int mdioBaseAddr,
                                  unsigned int phyAddr);
extern void Lan8710aReset(unsigned int mdioBaseAddr, 
                          unsigned int phyAddr);
extern unsigned int Lan8710aLoopBackEnable(unsigned int mdioBaseAddr,
                                           unsigned int phyAddr);
extern unsigned int Lan8710aLoopBackDisable(unsigned int mdioBaseAddr,
                                            unsigned int phyAddr);

extern unsigned int Lan8710aConfigure(unsigned int mdioBaseAddr, 
                                      unsigned int phyAddr,       
                                      unsigned short speed, 
                                      unsigned short dulplexMode);
extern unsigned int Lan8710aAutoNegotiate(unsigned int mdioBaseAddr,
                                          unsigned int phyAddr,
                                          unsigned short advVal);
extern unsigned int Lan8710aRegRead(unsigned int mdioBaseAddr, 
                                   unsigned int phyAddr,
                                   unsigned int regIdx, 
                                   unsigned short *regValAdr);
extern void Lan8710aRegWrite(unsigned int mdioBaseAddr, 
                             unsigned int phyAddr,
                             unsigned int regIdx, 
                             unsigned short regVal);

extern unsigned int Lan8710aPartnerAbilityGet(unsigned int mdioBaseAddr,
                                              unsigned int phyAddr,
                                              unsigned short *ptnerAblty);

unsigned int Lan8710aLinkStatusGet(unsigned int mdioBaseAddr,
                                   unsigned int phyAddr,
                                   volatile unsigned int retries);
#ifdef __cplusplus
}
#endif
#endif
