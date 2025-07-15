//*****************************************************************************
//
// usbdevice.h - types and definitions used during USB enumeration.
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
// This is part of AM1808 StarterWare USB Library and reused from revision 6288
// of the  Stellaris USB Library.
//
//*****************************************************************************

#ifndef __USBDEVICE_H__
#define __USBDEVICE_H__

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
//! \addtogroup device_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The maximum number of independent interfaces that any single device
//! implementation can support.  Independent interfaces means interface
//! descriptors with different bInterfaceNumber values - several interface
//! descriptors offering different alternative settings but the same interface
//! number count as a single interface.
//
//*****************************************************************************
#define USB_MAX_INTERFACES_PER_DEVICE 8

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// The default USB endpoint FIFO configuration structure.  This structure
// contains definitions to set all USB FIFOs into single buffered mode with
// no DMA use.  Each endpoint's FIFO is sized to hold the largest maximum
// packet size for any interface alternate setting in the current config
// descriptor.  A pointer to this structure may be passed in the psFIFOConfig
// field of the tDeviceInfo structure passed to USBCDCInit if the application
// does not require any special handling of the USB controller FIFO.
//
//*****************************************************************************
extern const tFIFOConfig g_sUSBDefaultFIFOConfig;

//*****************************************************************************
//
// Public APIs offered by the USB library device control driver.
//
//*****************************************************************************
extern void USBDCDInit(unsigned int ulIndex, tDeviceInfo *psDevice);
extern void USBDCDTerm(unsigned int ulIndex);
extern void USBDCDStallEP0(unsigned int ulIndex);
extern void USBDCDRequestDataEP0(unsigned int ulIndex, unsigned char *pucData,
                                 unsigned int ulSize);
extern void USBDCDSendDataEP0(unsigned int ulIndex, unsigned char *pucData,
                              unsigned int ulSize);
extern void USBDCDSetDefaultConfiguration(unsigned int ulIndex,
                                          unsigned int ulDefaultConfig);
extern unsigned int USBDCDConfigDescGetSize(const tConfigHeader *psConfig);
extern unsigned int USBDCDConfigDescGetNum(const tConfigHeader *psConfig,
                                            unsigned int ulType);
extern tDescriptorHeader *USBDCDConfigDescGet(const tConfigHeader *psConfig,
                                              unsigned int ulType,
                                              unsigned int ulIndex,
                                              unsigned int *pulSection);
extern unsigned int
       USBDCDConfigGetNumAlternateInterfaces(const tConfigHeader *psConfig,
                                             unsigned char ucInterfaceNumber);
extern tInterfaceDescriptor *
       USBDCDConfigGetInterface(const tConfigHeader *psConfig,
                                unsigned int ulIndex, unsigned int ulAltCfg,
                                unsigned int *pulSection);
extern tEndpointDescriptor *
       USBDCDConfigGetInterfaceEndpoint(const tConfigHeader *psConfig,
                                        unsigned int ulInterfaceNumber,
                                        unsigned int ulAltCfg,
                                        unsigned int ulIndex);
extern void USBDCDPowerStatusSet(unsigned int ulIndex, unsigned char ucPower);
extern tBoolean USBDCDRemoteWakeupRequest(unsigned int ulIndex);

//*****************************************************************************
//
// Early releases of the USB library had the following function named
// incorrectly.  This macro ensures that any code which used the previous name
// will still operate as expected.
//
//*****************************************************************************
#ifndef DEPRECATED
#define USBCDCConfigGetInterfaceEndpoint(a, b, c, d)                          \
            USBDCDConfigGetInterfaceEndpoint((a), (b), (c), (d))
#endif

//*****************************************************************************
//
// Device mode interrupt handler for controller index 0.
//
//*****************************************************************************
extern void USB0DeviceIntHandler(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __USBENUM_H__
