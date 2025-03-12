//*****************************************************************************
//
// usbhmsc.h - Definitions for the USB MSC host driver.
//
// Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of AM1808 StarterWare USB Library, resused from revision 6288 of the 
// stellaris USB Library
//
//*****************************************************************************

#ifndef __USBHMSC_H__
#define __USBHMSC_H__

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
//! \addtogroup usblib_host_class
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// These defines are the the events that will be passed in the \e ulEvent
// parameter of the callback from the driver.
//
//*****************************************************************************
#define MSC_EVENT_OPEN          1
#define MSC_EVENT_CLOSE         2

//*****************************************************************************
//
// The prototype for the USB MSC host driver callback function.
//
//*****************************************************************************
typedef void (*tUSBHMSCCallback)(unsigned int ulInstance,
                                 unsigned int ulEvent,
                                 void *pvEventData);

//*****************************************************************************
//
// Prototypes for the USB MSC host driver APIs.
//
//*****************************************************************************
extern unsigned int USBHMSCDriveOpen(unsigned int ulDrive,
                                      tUSBHMSCCallback pfnCallback);
extern void USBHMSCDriveClose(unsigned int ulInstance);
extern int USBHMSCDriveReady(unsigned int ulInstance);
extern int USBHMSCBlockRead(unsigned int ulInstance, unsigned int ulLBA,
                             unsigned char *pucData,
                             unsigned int ulNumBlocks);
extern int USBHMSCBlockWrite(unsigned int ulInstance, unsigned int ulLBA,
                              unsigned char *pucData,
                              unsigned int ulNumBlocks);

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

#endif // __USBHMSC_H__
