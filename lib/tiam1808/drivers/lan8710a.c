/**
 *  \file   lan8710a.c
 *
 *  \brief  APIs for configuring LAN8710A.
 *
 *   This file contains the device abstraction APIs for PHY LAN8270A.
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

/* HW Macros */
#include "hw_types.h"

/* Driver APIs */
#include "mdio.h"
#include "lan8710a.h"

/*******************************************************************************
*                        API FUNCTION DEFINITIONS
*******************************************************************************/
/**
 * \brief   Reads the PHY ID.
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 *
 * \return  32 bit PHY ID (ID1:ID2)
 *
 **/
unsigned int Lan8710aIDGet(unsigned int mdioBaseAddr, unsigned int phyAddr)
{
    unsigned int id = 0;
    unsigned short data;

    /* read the ID1 register */
    MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_ID1, &data);

    /* update the ID1 value */
    id = data << PHY_ID_SHIFT;
 
    /* read the ID2 register */
    MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_ID2, &data);

    /* update the ID2 value */
    id |= data; 

    /* return the ID in ID1:ID2 format */
    return id;
}

/**
 * \brief   Reads a register from the the PHY
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   regIdx        Index of the register to be read
 * \param   regValAdr     address where value of the register will be written
 *
 * \return  status of the read
 *
 **/
unsigned int Lan8710aRegRead(unsigned int mdioBaseAddr, unsigned int phyAddr,
                            unsigned int regIdx, unsigned short *regValAdr)
{
    return (MDIOPhyRegRead(mdioBaseAddr, phyAddr, regIdx, regValAdr));
}

/**
 * \brief   Writes a register with the input
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   regIdx        Index of the register to be read
 * \param   regValAdr     value to be written
 *
 * \return  None
 *
 **/
void Lan8710aRegWrite(unsigned int mdioBaseAddr, unsigned int phyAddr,
                      unsigned int regIdx, unsigned short regVal)
{
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, regIdx, regVal);
}

/**
 * \brief   Resets the PHY
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 *
 * \return  None
 *
 **/
void Lan8710aReset(unsigned int mdioBaseAddr, unsigned int phyAddr)
{
    /* Reset the phy */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, PHY_SOFTRESET);
}

/**
 * \brief   Enables Loop Back mode
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 *
 * \return  status after enabling.  \n
 *          TRUE if loop back is enabled \n
 *          FALSE if not able to enable
 *
 **/
unsigned int Lan8710aLoopBackEnable(unsigned int mdioBaseAddr, unsigned int phyAddr)
{
    unsigned short data;

    if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BCR, &data) != TRUE )
    {
        return FALSE;
    }

    data |= PHY_LPBK_ENABLE;

    /* Enable loop back */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, data);

    return TRUE;
}

/**
 * \brief   Disables Loop Back mode
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 *
 * \return  status after enabling.  \n
 *          TRUE if loop back is disabled \n
 *          FALSE if not able to disable
 *
 **/
unsigned int Lan8710aLoopBackDisable(unsigned int mdioBaseAddr, unsigned int phyAddr)
{
    unsigned short data;

    if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BCR, &data) != TRUE )
    {
        return FALSE;
    }

    data &= ~(PHY_LPBK_ENABLE);

    /* Disable loop back */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, data);

    return TRUE;
}

/**
 * \brief   Configures the PHY for a given speed and duplex mode. This 
 *          API will first reset the PHY. Then it sets the desired speed
 *          and duplex mode for the PHY.
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   speed         Speed to be enabled
 * \param   duplexMode    Duplex Mode
 *
 * \return  status after configuring \n
 *          TRUE if configuration successful
 *          FALSE if configuration failed
 *
 **/
unsigned int Lan8710aConfigure(unsigned int mdioBaseAddr, unsigned int phyAddr,
                               unsigned short speed, unsigned short duplexMode)
{
    unsigned short data;
    
    data = PHY_SOFTRESET;
    
    /* Reset the phy */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, data);

    /* wait till the reset bit is auto cleared */
    while(data)
    {
        /* Read the reset */
        if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BCR, &data) != TRUE)
        {
            return FALSE;
        }
    }

    /* Set the configurations */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, (speed | duplexMode));

    return TRUE;
}

/**
 * \brief   This function does Autonegotiates with the EMAC device connected
 *          to the PHY. It will wait till the autonegotiation completes.
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   advVal        Autonegotiation advertisement value
 *          advVal can take the following any OR combination of the values \n
 *               LAN8710A_100BTX - 100BaseTX
 *               LAN8710A_100BTX_FD - Full duplex capabilty for 100BaseTX 
 *               LAN8710A_10BT - 10BaseT
 *               LAN8710A_10BT_FD - Full duplex capability for 10BaseT
 *
 * \return  status after autonegotiation \n
 *          TRUE if autonegotiation successful
 *          FALSE if autonegotiation failed
 *
 **/
unsigned int Lan8710aAutoNegotiate(unsigned int mdioBaseAddr,
                                   unsigned int phyAddr, unsigned short advVal)
{
    volatile unsigned short data, anar;

    if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BCR, &data) != TRUE )
    {
        return FALSE;
    }
   
    data |= PHY_AUTONEG_ENABLE; 
   
    /* Enable Auto Negotiation */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, data);

    if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BCR, &data) != TRUE )
    {
        return FALSE;
    }

    /* Write Auto Negotiation capabilities */
    MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_AUTONEG_ADV, &anar);
    anar &= ~0xff10;
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_AUTONEG_ADV, (anar |advVal));

    data |= PHY_AUTONEG_RESTART;

    /* Start Auto Negotiation */
    MDIOPhyRegWrite(mdioBaseAddr, phyAddr, PHY_BCR, data);

    /* Get the auto negotiation status*/
    if(MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BSR, &data) != TRUE)
    {
         return FALSE;
    }
    
    /* Wait till auto negotiation is complete */
    while(PHY_AUTONEG_INCOMPLETE == (data & PHY_AUTONEG_STATUS))
    {
         MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BSR, &data);
    }

    /* Check if the PHY is able to perform auto negotiation */
    if(data & PHY_AUTONEG_ABLE)
    {
         return TRUE;
    }
  
    return FALSE;
}

/**
 * \brief   Reads the Link Partner Ability register of the PHY.
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   ptnerAblty    The partner abilities of the EMAC
 *
 * \return  status after reading \n
 *          TRUE if reading successful
 *          FALSE if reading failed
 **/
unsigned int Lan8710aPartnerAbilityGet(unsigned int mdioBaseAddr, 
                                       unsigned int phyAddr,
                                       unsigned short *ptnerAblty)
{
    return (MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_LINK_PARTNER_ABLTY,
                           ptnerAblty));
}

/**
 * \brief   Reads the link status of the PHY.
 *
 * \param   mdioBaseAddr  Base Address of the MDIO Module Registers.
 * \param   phyAddr       PHY Adress.
 * \param   retries       The number of retries before indicating down status
 *
 * \return  link status after reading \n
 *          TRUE if link is up
 *          FALSE if link is down \n
 *
 * \note    This reads both the basic status register of the PHY and the
 *          link register of MDIO for double check
 **/
unsigned int Lan8710aLinkStatusGet(unsigned int mdioBaseAddr,
                                   unsigned int phyAddr,
                                   volatile unsigned int retries)
{
    volatile unsigned short linkStatus;
    volatile unsigned int retVal = TRUE;

    while (retVal == TRUE)
    {
        /* First read the BSR of the PHY */
        MDIOPhyRegRead(mdioBaseAddr, phyAddr, PHY_BSR, &linkStatus);

        if(linkStatus & PHY_LINK_STATUS)
        {
            /* Check if MDIO LINK register is updated */
            linkStatus = MDIOPhyLinkStatusGet(mdioBaseAddr);

            if(linkStatus & (1 << phyAddr))
            {
               break;
            }
            else
            {
                (retries != 0) ? retries-- : (retVal = FALSE);
            }
        }
        else
        {
            (retries != 0) ? retries-- : (retVal = FALSE);
        }
    }

    return retVal;
}

/**************************** End Of File ***********************************/
