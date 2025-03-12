/**
 *  \file   emac.c
 *
 *  \brief  EMAC APIs.
 *
 *   This file contains the device abstraction layer APIs for EMAC.
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

/* HW Macros and Peripheral Defines */
#include "hw_types.h"
#include "hw_emac.h"
#include "hw_emac_ctrl.h"

/* Driver APIs */
#include "emac.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define EMAC_CONTROL_RESET             (0x01u)
#define EMAC_SOFT_RESET                (0x01u)
#define EMAC_MAX_HEADER_DESC           (8u)
#define EMAC_UNICAST_DISABLE           (0xFFu)

/*******************************************************************************
*                        API FUNCTION DEFINITIONS
*******************************************************************************/
/**
 * \brief   Enables the TXPULSE Interrupt Generation.
 *
 * \param   emacBase      Base address of the EMAC Module registers.
 * \param   emacCtrlBase  Base address of the EMAC CONTROL module registers
 * \param   ctrlCore      Control core for which the interrupt to be enabled.
 * \param   channel       Channel number for which interrupt to be enabled
 *
 * \return  None
 *
 **/
void EMACTxIntPulseEnable(unsigned int emacBase, unsigned int emacCtrlBase,
                          unsigned int ctrlCore, unsigned int channel)
{
    HWREG(emacBase + EMAC_TXINTMASKSET) |= (1 << channel);

    HWREG(emacCtrlBase + EMAC_CTRL_CnTXEN(ctrlCore)) |= (1 << channel);
}

/**
 * \brief   Disables the TXPULSE Interrupt Generation.
 *
 * \param   emacBase      Base address of the EMAC Module registers.
 * \param   emacCtrlBase  Base address of the EMAC CONTROL module registers
 * \param   ctrlCore      Control core for which the interrupt to be disabled.
 * \param   channel       Channel number for which interrupt to be disabled
 *
 * \return  None
 *
 **/
void EMACTxIntPulseDisable(unsigned int emacBase, unsigned int emacCtrlBase,
                           unsigned int ctrlCore, unsigned int channel)
{
    HWREG(emacBase + EMAC_TXINTMASKCLEAR) |= (1 << channel);

    HWREG(emacCtrlBase + EMAC_CTRL_CnTXEN(ctrlCore)) &= ~(1 << channel);
}

/**
 * \brief   Enables the RXPULSE Interrupt Generation.
 *
 * \param   emacBase      Base address of the EMAC Module registers.
 * \param   emacCtrlBase  Base address of the EMAC CONTROL module registers
 * \param   ctrlCore      Control core for which the interrupt to be enabled.
 * \param   channel       Channel number for which interrupt to be enabled
 *
 * \return  None
 *
 **/
void EMACRxIntPulseEnable(unsigned int emacBase, unsigned int emacCtrlBase,
                          unsigned int ctrlCore, unsigned int channel)
{
    HWREG(emacBase + EMAC_RXINTMASKSET) |= (1 << channel);

    HWREG(emacCtrlBase + EMAC_CTRL_CnRXEN(ctrlCore)) |= (1 << channel);
}

/**
 * \brief   Disables the RXPULSE Interrupt Generation.
 *
 * \param   emacBase      Base address of the EMAC Module registers.
 * \param   emacCtrlBase  Base address of the EMAC CONTROL module registers
 * \param   ctrlCore      Control core for which the interrupt to be disabled.
 * \param   channel       Channel number for which interrupt to be disabled
 *
 * \return  None
 *
 **/
void EMACRxIntPulseDisable(unsigned int emacBase, unsigned int emacCtrlBase,
                           unsigned int ctrlCore, unsigned int channel)
{
    HWREG(emacBase + EMAC_RXINTMASKCLEAR) |= (1 << channel);

    HWREG(emacCtrlBase + EMAC_CTRL_CnRXEN(ctrlCore)) &= ~(1 << channel);
}
/**
 * \brief   This API sets the RMII speed. The RMII Speed can be 10 Mbps or 
 *          100 Mbps
 *
 * \param   emacBase     Base address of the EMAC Module registers.
 * \param   speed        speed for setting.
 *          speed can take the following values. \n
 *                EMAC_RMIISPEED_10MBPS - 10 Mbps \n
 *                EMAC_RMIISPEED_100MBPS - 100 Mbps. 
 *
 * \return  None
 *
 **/
void EMACRMIISpeedSet(unsigned int emacBase, unsigned int speed)
{
    HWREG(emacBase + EMAC_MACCONTROL) &= ~EMAC_MACCONTROL_RMIISPEED;
    
    HWREG(emacBase + EMAC_MACCONTROL) |= speed;
}

/**
 * \brief   This API enables the MII control block
 *
 * \param   emacBase     Base address of the EMAC Module registers.
 *
 * \return  None
 *
 **/
void EMACMIIEnable(unsigned int emacBase)
{
    HWREG(emacBase + EMAC_MACCONTROL) |= EMAC_MACCONTROL_GMIIEN;
}

/**
 * \brief   This API sets the duplex mode of operation(full/half) for MAC.
 *
 * \param   emacBase     Base address of the EMAC Module registers.
 * \param   duplexMode   duplex mode of operation.
 *          duplexMode can take the following values. \n
 *                EMAC_DUPLEX_FULL - Full Duplex  \n
 *                EMAC_DUPLEX_HALF - Half Duplex.
 *
 * \return  None
 *
 **/
void EMACDuplexSet(unsigned int emacBase, unsigned int duplexMode)
{
    HWREG(emacBase + EMAC_MACCONTROL) &= ~EMAC_MACCONTROL_FULLDUPLEX;
    
    HWREG(emacBase + EMAC_MACCONTROL) |= duplexMode;
}

/**
 * \brief   API to enable the transmit in the TX Control Register
 *          After the transmit is enabled, any write to TXHDP of
 *          a channel will start transmission
 *
 * \param   emacBase      Base Address of the EMAC Module Registers.
 *
 * \return  None
 *
 **/
void EMACTxEnable(unsigned int emacBase)
{
    HWREG(emacBase + EMAC_TXCONTROL) = EMAC_TXCONTROL_TXEN;
}

/**
 * \brief   API to enable the receive in the RX Control Register
 *          After the transmit is enabled, and write to RXHDP of
 *          a channel, the data can be received in the destination
 *          specified by the corresponding RX buffer descriptor.
 *
 * \param   emacBase      Base Address of the EMAC Module Registers.
 *
 * \return  None
 *
 **/
void EMACRxEnable(unsigned int emacBase)
{
    HWREG(emacBase + EMAC_RXCONTROL) = EMAC_RXCONTROL_RXEN;
}

/**
 * \brief   API to write the TX HDP register. If transmit is enabled,
 *          write to the TX HDP will immediately start transmission.
 *          The data will be taken from the buffer pointer of the TX buffer
 *          descriptor written to the TX HDP
 *
 * \param   emacBase      Base Address of the EMAC Module Registers.\n
 * \param   descHdr       Address of the TX buffer descriptor
 * \param   channel       Channel Number
 *
 * \return  None
 *
 **/
void EMACTxHdrDescPtrWrite(unsigned int emacBase, unsigned int descHdr,
                           unsigned int channel)
{
    HWREG(emacBase + EMAC_TXHDP(channel)) = descHdr;
}

/**
 * \brief   API to write the RX HDP register. If receive is enabled,
 *          write to the RX HDP will enable data reception to point to
 *          the corresponding RX buffer descriptor's buffer pointer.
 *
 * \param   emacBase      Base Address of the EMAC Module Registers.\n
 * \param   descHdr       Address of the RX buffer descriptor
 * \param   channel       Channel Number
 *
 * \return  None
 *
 **/
void EMACRxHdrDescPtrWrite(unsigned int emacBase, unsigned int descHdr,
                           unsigned int channel)
{
    HWREG(emacBase + EMAC_RXHDP(channel)) = descHdr;
}

/**
 * \brief   This API Initializes the EMAC and EMAC Control modules. The
 *          EMAC Control module is reset, the CPPI RAM is cleared. also,
 *          all the interrupts are disabled. This API doesnot enable any
 *          interrupt or operation of the EMAC.
 *
 * \param   emacCtrlBase      Base Address of the EMAC Control module
 *                            registers.\n
 * \param   emacBase          Base address of the EMAC module registers
 *
 * \return  None
 *
 **/
void EMACInit(unsigned int emacCtrlBase, unsigned int emacBase)
{
    unsigned int cnt;

    /* Reset the EMAC Control Module. This clears the CPPI RAM also */
    HWREG(emacCtrlBase + EMAC_CTRL_SOFTRESET) = EMAC_CONTROL_RESET;

    while(HWREG(emacCtrlBase + EMAC_CTRL_SOFTRESET) & EMAC_CONTROL_RESET);

    /* Reset the EMAC Control Module. This clears the CPPI RAM also */
    HWREG(emacBase + EMAC_SOFTRESET) = EMAC_SOFT_RESET;

    while(HWREG(emacBase + EMAC_SOFTRESET) & EMAC_SOFT_RESET);

    HWREG(emacBase + EMAC_MACCONTROL)= 0;
    HWREG(emacBase + EMAC_RXCONTROL)= 0;
    HWREG(emacBase + EMAC_TXCONTROL)= 0;

    /* Initialize all the header descriptor pointer registers */
    for(cnt =  0; cnt< EMAC_MAX_HEADER_DESC; cnt++)
    {
        HWREG(emacBase + EMAC_RXHDP(cnt)) = 0;
        HWREG(emacBase + EMAC_TXHDP(cnt)) = 0;
        HWREG(emacBase + EMAC_RXCP(cnt)) = 0;
        HWREG(emacBase + EMAC_TXCP(cnt)) = 0;
        HWREG(emacBase + EMAC_RXFREEBUFFER(cnt)) = 0xFF;
    }
    /* Clear the interrupt enable for all the channels */
    HWREG(emacBase + EMAC_TXINTMASKCLEAR) = 0xFF;
    HWREG(emacBase + EMAC_RXINTMASKCLEAR) = 0xFF;

    HWREG(emacBase + EMAC_MACHASH1) = 0;
    HWREG(emacBase + EMAC_MACHASH2) = 0;

    HWREG(emacBase + EMAC_RXBUFFEROFFSET) = 0;
}

/**
 * \brief   Sets the MAC Address in MACSRCADDR registers.
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   macAddr       Start address of a MAC address array.
 *                        The array[0] shall be the LSB of the MAC address
 *
 * \return  None
 *
 **/
void  EMACMACSrcAddrSet(unsigned int emacBase, unsigned char *macAddr)
{
    HWREG(emacBase + EMAC_MACSRCADDRHI) = macAddr[5] |(macAddr[4] << 8)
                                     |(macAddr[3] << 16) |(macAddr[2] << 24);
    HWREG(emacBase + EMAC_MACSRCADDRLO) = macAddr[1] | (macAddr[0] << 8);
}

/**
 * \brief   Sets the MAC Address in MACADDR registers.
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number
 * \param   matchFilt     Match or Filter
 * \param   macAddr       Start address of a MAC address array.
 *                        The array[0] shall be the LSB of the MAC address
 *          matchFilt can take the following values \n
 *          EMAC_MACADDR_NO_MATCH_NO_FILTER - Address is not used to match
 *                                             or filter incoming packet. \n
 *          EMAC_MACADDR_FILTER - Address is used to filter incoming packets \n
 *          EMAC_MACADDR_MATCH - Address is used to match incoming packets \n
 *
 * \return  None
 *
 **/
void EMACMACAddrSet(unsigned int emacBase, unsigned int channel,
                    unsigned char *macAddr, unsigned int matchFilt)
{
    HWREG(emacBase + EMAC_MACINDEX) = channel;

    HWREG(emacBase + EMAC_MACADDRHI) = macAddr[5] |(macAddr[4] << 8)
                                     |(macAddr[3] << 16) |(macAddr[2] << 24);
    HWREG(emacBase + EMAC_MACADDRLO) = macAddr[1] | (macAddr[0] << 8)
                                     | matchFilt | (channel << 16);
}

/**
 * \brief   Acknowledges an interrupt processed to the EMAC Control Core.
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   eoiFlag       Type of interrupt to acknowledge to the EMAC Control
 *                         module.
 *          eoiFlag can take the following values \n
 *             EMAC_INT_CORE0_TX - Core 0 TX Interrupt
 *             EMAC_INT_CORE1_TX - Core 1 TX Interrupt
 *             EMAC_INT_CORE2_TX - Core 2 TX Interrupt
 *             EMAC_INT_CORE0_RX - Core 0 RX Interrupt
 *             EMAC_INT_CORE1_RX - Core 1 RX Interrupt
 *             EMAC_INT_CORE2_RX - Core 2 RX Interrupt
 * \return  None
 * 
 **/
void EMACCoreIntAck(unsigned int emacBase, unsigned int eoiFlag)
{
    /* Acknowledge the EMAC Control Core */
    HWREG(emacBase + EMAC_MACEOIVECTOR) = eoiFlag;
}

/**
 * \brief   Writes the the TX Completion Pointer for a specific channel
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number.
 * \param   comPtr        Completion Pointer Value to be written
 *
 * \return  None
 *
 **/
void EMACTxCPWrite(unsigned int emacBase, unsigned int channel, unsigned int comPtr)
{
    HWREG(emacBase + EMAC_TXCP(channel)) = comPtr;
}

/**
 * \brief   Writes the the RX Completion Pointer for a specific channel
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number.
 * \param   comPtr        Completion Pointer Value to be written
 *
 * \return  None
 *
 **/
void EMACRxCPWrite(unsigned int emacBase, unsigned int channel, unsigned int comPtr)
{
    HWREG(emacBase + EMAC_RXCP(channel)) = comPtr;
}

/**
 * \brief   Acknowledges an interrupt processed to the EMAC module. After
 *          processing an interrupt, the last processed buffer descriptor is
 *          written to the completion pointer. Also this API acknowledges
 *          the EMAC Control Module that the RX interrupt is processed for
 *          a specified core
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number
 * \param   comPtr        Completion Pointer value. This shall be the buffer
 *                        descriptor address last processed.
 * \param   eoiFlag       Type of interrupt to acknowledge to the EMAC Control
                          module.
 *          eoiFlag can take the following values \n
 *             EMAC_INT_CORE0_RX - Core 0 RX Interrupt
 *             EMAC_INT_CORE1_RX - Core 1 RX Interrupt
 *             EMAC_INT_CORE2_RX - Core 2 RX Interrupt
 * \return  None
 *
 **/
void EMACRxIntAckToClear(unsigned int emacBase, unsigned int channel,
                         unsigned int comPtr, unsigned eoiFlag)
{
    HWREG(emacBase + EMAC_RXCP(channel)) = comPtr;

    /* Acknowledge the EMAC Control Core */
    HWREG(emacBase + EMAC_MACEOIVECTOR) = eoiFlag;
}

/**
 * \brief   Enables a specific channel to receive broadcast frames
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number.
 *
 * \return  None
 *
 **/
void EMACRxBroadCastEnable(unsigned int emacBase, unsigned int channel)
{
    HWREG(emacBase + EMAC_RXMBPENABLE) &= ~EMAC_RXMBPENABLE_RXBROADCH;

    HWREG(emacBase + EMAC_RXMBPENABLE) |=
                              EMAC_RXMBPENABLE_RXBROADEN | 
                              (channel << EMAC_RXMBPENABLE_RXBROADCH_SHIFT);
}

/**
 * \brief   Enables unicast for a specific channel
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number.
 *
 * \return  None
 *
 **/
void EMACRxUnicastSet(unsigned int emacBase, unsigned int channel)
{
    HWREG(emacBase + EMAC_RXUNICASTSET) |= (1 << channel);
}

/**
 * \brief   Set the free buffers for a specific channel
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 * \param   channel       Channel Number.
 * \param   nBuf          Number of free buffers
 *
 * \return  None
 *
 **/
void EMACNumFreeBufSet(unsigned int emacBase, unsigned int channel,
                       unsigned int nBuf)
{
    HWREG(emacBase + EMAC_RXFREEBUFFER(channel)) = nBuf;
}

/**
 * \brief   Gets the interrupt vectors of EMAC, which are pending
 *
 * \param   emacBase      Base Address of the EMAC module registers.
 *
 * \return  Vectors
 *
 **/
unsigned int EMACIntVectorGet(unsigned int emacBase)
{
    return (HWREG(emacBase + EMAC_MACINVECTOR));
}

/***************************** End Of File ***********************************/
