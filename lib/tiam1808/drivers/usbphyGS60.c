/**
 * \file    Usbphy.c
 *
 * \brief   This file contains USB Phy related functions.
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
#include "hw_usb.h"

#ifdef _TMS320C6X
/* DSP System Defines */
#include "soc_C6748.h"
#include "hw_usbphyGS60.h"
#else
/* ARM System Defines */
#include "soc_AM1808.h"
#include "hw_usbphyGS60.h"
#endif

/**
 * \brief This function will switch on the USB Phy  
 *          
 *
 * \param    None
 *
 * \return   None
 *
  **/
void UsbPhyOn()
{

    unsigned int  reg = 0;

	reg = HWREG(CFGCHIP2_USBPHYCTRL); 

    reg &= ~(CFGCHIP2_RESET | CFGCHIP2_PHYPWRDN | CFGCHIP2_OTGPWRDN);   

    reg |= CFGCHIP2_PHY_PLLON;

#ifdef USB_MODE_HS_DISABLE
	HWREGB(USB0_BASE + USB_O_POWER) &= 0xdf;
#endif /* USB_MODE_HS_DISABLE  */

    HWREG(CFGCHIP2_USBPHYCTRL) = reg;

    while (!(HWREG(CFGCHIP2_USBPHYCTRL) & CFGCHIP2_PHYCLKGD)); 
	
}


/**
 * \brief This function will switch off  the USB Phy  
 *          
 *
 * \param    None
 *
 * \return   None
 *
  **/
void UsbPhyOff()
{
	unsigned int  reg = 0;
	reg = HWREG(CFGCHIP2_USBPHYCTRL); 
	
	/** Ensure that USB 1.1 reference clock is not being sourced from	
			* USB 2.0 PHY.	Otherwise do not power down the PHY.*/
	
	if (!( reg & CFGCHIP2_USB1PHYCLKMUX) && ( reg & CFGCHIP2_USB1SUSPENDM)) 
	{
	   return;
	}

	/** Power down the on-chip PHY.	*/
	reg |= CFGCHIP2_PHYPWRDN | CFGCHIP2_OTGPWRDN;
	HWREG(CFGCHIP2_USBPHYCTRL) = reg;
}

