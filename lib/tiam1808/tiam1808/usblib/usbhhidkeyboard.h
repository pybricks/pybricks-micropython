//*****************************************************************************
//
// usbhhidkeyboard.h - This file holds the application interfaces for USB
//                     keyboard devices.
//
// Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 8049 of the Stellaris USB Library.
//
//*****************************************************************************

#ifndef __USBHHIDKEYBOARD_H__
#define __USBHHIDKEYBOARD_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup usblib_host_device
//! @{
//
//*****************************************************************************

extern unsigned int USBHKeyboardOpen(tUSBCallback pfnCallback,
                                      unsigned char *pucBuffer,
                                      unsigned int ulBufferSize);
extern unsigned int USBHKeyboardClose(unsigned int ulInstance);
extern unsigned int USBHKeyboardInit(unsigned int ulInstance);
extern unsigned int USBHKeyboardModifierSet(unsigned int ulInstance,
                                             unsigned int ulModifiers);
extern unsigned int USBHKeyboardPollRateSet(unsigned int ulInstance,
                                             unsigned int ulPollRate);

extern unsigned int USBHKeyboardUsageToChar(
    unsigned int ulInstance,
    const tHIDKeyboardUsageTable *pTable,
    unsigned char ucUsageID);

//*****************************************************************************
//
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif
