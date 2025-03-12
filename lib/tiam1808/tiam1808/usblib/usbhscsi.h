//*****************************************************************************
//
// usbhscsi.h - Definitions for the USB host SCSI layer.
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
// stellaris USB Library.
//
//*****************************************************************************

#ifndef __USBHSCSI_H__
#define __USBHSCSI_H__

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
// Prototypes for the APIs exported by the USB SCSI layer.
//
//*****************************************************************************
extern unsigned int USBHSCSIInquiry(unsigned int ulInPipe,
                                     unsigned int ulOutPipe,
                                     unsigned char *pucBuffer,
                                     unsigned int *pulSize);
extern unsigned int USBHSCSIReadCapacity(unsigned int ulInPipe,
                                          unsigned int ulOutPipe,
                                          unsigned char *pData,
                                          unsigned int *pulSize);
extern unsigned int USBHSCSIReadCapacities(unsigned int ulInPipe,
                                            unsigned int ulOutPipe,
                                            unsigned char *pData,
                                            unsigned int *pulSize);
extern unsigned int USBHSCSIModeSense6(unsigned int ulInPipe,
                                        unsigned int ulOutPipe,
                                        unsigned int ulFlags,
                                        unsigned char *pData,
                                        unsigned int *pulSize);
extern unsigned int USBHSCSITestUnitReady(unsigned int ulInPipe,
                                           unsigned int ulOutPipe);
extern unsigned int USBHSCSIRequestSense(unsigned int ulInPipe,
                                          unsigned int ulOutPipe,
                                          unsigned char *pucData,
                                          unsigned int *pulSize);
extern unsigned int USBHSCSIRead10(unsigned int ulInPipe,
                                    unsigned int ulOutPipe,
                                    unsigned int ulLBA,
                                    unsigned char *pucData,
                                    unsigned int *pulSize,
                                    unsigned int ulNumBlocks);
extern unsigned int USBHSCSIWrite10(unsigned int ulInPipe,
                                     unsigned int ulOutPipe,
                                     unsigned int ulLBA,
                                     unsigned char *pucData,
                                     unsigned int *pulSize,
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

#endif // __USBHSCSI_H__
