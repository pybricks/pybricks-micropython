
/** ============================================================================
 *   \file  hw_usbOtg_AM1808.h
 *
 *   \brief This file contains the offset of USB OTG registers
 *
 *  ============================================================================
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


#ifndef __HW_USBOTG_H__
#define __HW_USBOTG_H__

/* If building with a C++ compiler, make all of the definitions in this header
 * have a C binding. */
#ifdef __cplusplus
extern "C"
{
#endif

/* The following are defines for the Univeral Serial Bus OTG register offsets. */

#define USB_0_OTGBASE				SOC_USB_0_OTG_BASE

#define USB_0_REVISION				0x00
#define USB_0_CTRL	            	0x04
#define USB_0_STAT	            	0x08
#define USB_0_EMULATION				0x08
#define USB_0_MODE					0x10
#define USB_0_AUTOREQ 	        	0x14
#define USB_0_SRP_FIX_TIME 	    	0x18
#define USB_0_TEARDOWN 	        	0x1c
#define USB_0_INTR_SRC        		0x20
#define USB_0_INTR_SRC_SET 	    	0x24
#define USB_0_INTR_SRC_CLEAR 		0x28
#define USB_0_INTR_MASK 	       	0x2c
#define USB_0_INTR_MASK_SET 	   	0x30
#define USB_0_INTR_MASK_CLEAR 	 	0x34
#define USB_0_INTR_SRC_MASKED 	 	0x38
#define USB_0_END_OF_INTR 	     	0x3c

#define USB_0_GEN_RNDIS_SIZE_EP1	0x50
#define USB_0_GEN_RNDIS_SIZE_EP2	0x54
#define USB_0_GEN_RNDIS_SIZE_EP3	0x58
#define USB_0_GEN_RNDIS_SIZE_EP4	0x5C
#define USB_0_GENR_INTR				0x22

// Bits for CTRL
#define USBOTG_CTRL_RESET           0x01
#define USBOTG_CTRL_CLKFACK         0x02
#define USBOTG_CTRL_UINT            0x08
#define USBOTG_CTRL_RNDIS           0x10

// The following are interrupt bits as found in the OTG wrapper
#define USBOTG_INTR_EP0             0x00000001
#define USBOTG_INTR_EP1_IN          0x00000002
#define USBOTG_INTR_EP2_IN          0x00000004
#define USBOTG_INTR_EP3_IN          0x00000008
#define USBOTG_INTR_EP4_IN          0x00000010
#define USBOTG_INTR_EP1_OUT         0x00000200
#define USBOTG_INTR_EP2_OUT         0x00000400
#define USBOTG_INTR_EP3_OUT         0x00000800
#define USBOTG_INTR_EP4_OUT         0x00001000
#define USBOTG_INTR_DRVVBUS         0x01000000  // DRVVBUS level change
#define USBOTG_INTR_VBUSERR         0x00800000  // VBUS Error
#define USBOTG_INTR_SESREQ          0x00400000  // SESSION REQUEST
#define USBOTG_INTR_DISCON          0x00200000  // Session Disconnect
#define USBOTG_INTR_CONN            0x00100000  // Session Connect
#define USBOTG_INTR_SOF             0x00080000  // Start of Frame
#define USBOTG_INTR_BABBLE          0x00040000  // Babble Detected
#define USBOTG_INTR_RESET           0x00040000  // RESET Signaling Detected
#define USBOTG_INTR_RESUME          0x00020000  // RESUME Signaling Detected
#define USBOTG_INTR_SUSPEND         0x00010000  // SUSPEND Signaling Detected

#ifdef __cplusplus
}
#endif

#endif /* __HW_USBOTG_H__ */
