//****************************************************************************
//
// usbdcomp.c - USB composite device class driver.
//
// Copyright (c) 2010 Texas Instruments Incorporated.  All rights reserved.
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
//****************************************************************************

//#include "hw_memmap.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"
//#include "sysctl.h"
#include "usb.h"
//#include "udma.h"
#include "usblib.h"
#include "usb-ids.h"
#include "usbcdc.h"
#include "usbdevice.h"
#include "usbdcdc.h"
#include "usbdcomp.h"
#include "usblibpriv.h"

//****************************************************************************
//
//! \addtogroup composite_device_class_api
//! @{
//
//****************************************************************************

//****************************************************************************
//
// Device Descriptor.  This is stored in RAM to allow several fields to be
// changed at runtime based on the client's requirements.
//
//****************************************************************************
static unsigned char g_pCompDeviceDescriptor[] =
{
    18,                     // Size of this structure.
    USB_DTYPE_DEVICE,       // Type of this structure.
    USBShort(0x110),        // USB version 1.1 (if we say 2.0, hosts assume
                            // high-speed - see USB 2.0 spec 9.2.6.6)
    USB_CLASS_MISC,         // USB Device Class (spec 5.1.1)
    USB_MISC_SUBCLASS_COMMON, // USB Device Sub-class (spec 5.1.1)
    USB_MISC_PROTOCOL_IAD,  // USB Device protocol (spec 5.1.1)
    64,                     // Maximum packet size for default pipe.
    USBShort(0),            // Vendor ID (filled in during USBDCompositeInit).
    USBShort(0),            // Product ID (filled in during USBDCompositeInit).
    USBShort(0x100),        // Device Version BCD.
    1,                      // Manufacturer string identifier.
    2,                      // Product string identifier.
    3,                      // Product serial number.
    1                       // Number of configurations.
};

//****************************************************************************
//
// Composite class device configuration descriptor.
//
// It is vital that the configuration descriptor bConfigurationValue field
// (byte 6) is 1 for the first configuration and increments by 1 for each
// additional configuration defined here.  This relationship is assumed in the
// device stack for simplicity even though the USB 2.0 specification imposes
// no such restriction on the bConfigurationValue values.
//
// Note that this structure is deliberately located in RAM since we need to
// be able to patch some values in it based on client requirements.
//
//****************************************************************************
static const unsigned char g_pCompConfigDescriptor[] =
{
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(0),                // The total size of this full structure.
    0,                          // The number of interfaces in this
                                // configuration, this will be filled by
                                // the class as it discovers all classes
                                // supported.
    1,                          // The unique value for this configuration.
    0,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_BUS_PWR,      // .
    250,                        // The maximum power in 2mA increments.
};

//****************************************************************************
//
// Various internal handlers needed by this class.
//
//****************************************************************************
static void HandleDisconnect(void *pvInstance);
static void InterfaceChange(void *pvInstance, unsigned char ucInterfaceNum,
                            unsigned char ucAlternateSetting);
static void ConfigChangeHandler(void *pvInstance, unsigned int ulValue);
static void DataSent(void *pvInstance, unsigned int ulInfo);
static void DataReceived(void *pvInstance, unsigned int ulInfo);
static void HandleEndpoints(void *pvInstance, unsigned int ulStatus);
static void HandleRequests(void *pvInstance, tUSBRequest *pUSBRequest);
static void SuspendHandler(void *pvInstance);
static void ResumeHandler(void *pvInstance);
static void ResetHandler(void *pvInstance);
static void GetDescriptor(void *pvInstance, tUSBRequest *pUSBRequest);

//****************************************************************************
//
// Configuration Descriptor.
//
//****************************************************************************
tConfigHeader *g_pCompConfigDescriptors[1];

//****************************************************************************
//
// The FIFO configuration for USB mass storage class device.
//
//****************************************************************************
tFIFOConfig g_sUSBCompositeFIFOConfig =
{
    //
    // IN endpoints.
    //
    {
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN },
        { 1, false, USB_EP_DEV_IN }
    },
    //
    // OUT endpoints.
    //
    {
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT },
        { 1, false, USB_EP_DEV_OUT }
    },
};

//****************************************************************************
//
// The device information structure for the USB Composite device.
//
//****************************************************************************
tDeviceInfo g_sCompositeDeviceInfo =
{
    //
    // Device event handler callbacks.
    //
    {
        //
        // GetDescriptor
        //
        GetDescriptor,

        //
        // RequestHandler
        //
        HandleRequests,

        //
        // InterfaceChange
        //
        InterfaceChange,

        //
        // ConfigChange
        //
        ConfigChangeHandler,

        //
        // DataReceived
        //
        DataReceived,

        //
        // DataSentCallback
        //
        DataSent,

        //
        // ResetHandler
        //
        ResetHandler,

        //
        // SuspendHandler
        //
        SuspendHandler,

        //
        // ResumeHandler
        //
        ResumeHandler,

        //
        // DisconnectHandler
        //
        HandleDisconnect,

        //
        // EndpointHandler
        //
        HandleEndpoints
    },
    g_pCompDeviceDescriptor,
    (const tConfigHeader **)g_pCompConfigDescriptors,
    0,
    0,
    &g_sUSBCompositeFIFOConfig
};

//****************************************************************************
//
// This function will check if any device classes need a get descriptor
// handler called.
//
//****************************************************************************
static void
GetDescriptor(void *pvInstance, tUSBRequest *pUSBRequest)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        //
        // Create the individual device pointer.
        //
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnGetDescriptor)
        {
            pDeviceInfo->sCallbacks.pfnGetDescriptor(
                    psDevice->psDevices[ulIdx].pvInstance, pUSBRequest);
        }
    }
}

//****************************************************************************
//
// This function will check if any device classes need an suspend handler
// called.
//
//****************************************************************************
static void
SuspendHandler(void *pvInstance)
{
    unsigned int ulIdx;
    tUSBDCompositeDevice *psDevice;
    const tDeviceInfo *pDeviceInfo;
    void *pvDeviceInst;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    //
    // Inform the application that the device has resumed.
    //
    if(psDevice->pfnCallback)
    {
        psDevice->pfnCallback(pvInstance, USB_EVENT_SUSPEND, 0, 0);
    }

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;
        pvDeviceInst = psDevice->psDevices[ulIdx].pvInstance;

        if(pDeviceInfo->sCallbacks.pfnSuspendHandler)
        {
            pDeviceInfo->sCallbacks.pfnSuspendHandler(pvDeviceInst);
        }
    }
}

//****************************************************************************
//
// This function will check if any device classes need an resume handler
// called.
//
//****************************************************************************
static void
ResumeHandler(void *pvInstance)
{
    unsigned int ulIdx;
    tUSBDCompositeDevice *psDevice;
    const tDeviceInfo *pDeviceInfo;
    void *pvDeviceInst;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    //
    // Inform the application that the device has resumed.
    //
    if(psDevice->pfnCallback)
    {
        psDevice->pfnCallback(pvInstance, USB_EVENT_RESUME, 0, 0);
    }

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;
        pvDeviceInst = psDevice->psDevices[ulIdx].pvInstance;

        if(pDeviceInfo->sCallbacks.pfnResumeHandler)
        {
            pDeviceInfo->sCallbacks.pfnResumeHandler(pvDeviceInst);
        }
    }
}

//****************************************************************************
//
// This function will check if any device classes need an reset handler
// called.
//
//****************************************************************************
static void
ResetHandler(void *pvInstance)
{
    unsigned int ulIdx;
    tUSBDCompositeDevice *psDevice;
    const tDeviceInfo *pDeviceInfo;
    void *pvDeviceInst;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    //
    // Inform the application that the device has been connected.
    //
    if(psDevice->pfnCallback)
    {
        psDevice->pfnCallback(pvInstance, USB_EVENT_CONNECTED, 0, 0);
    }

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;
        pvDeviceInst = psDevice->psDevices[ulIdx].pvInstance;

        if(pDeviceInfo->sCallbacks.pfnResetHandler)
        {
            pDeviceInfo->sCallbacks.pfnResetHandler(pvDeviceInst);
        }
    }
}

//****************************************************************************
//
// This function is called to handle data being set to the host so that the
// application callback can be called when the data has been transferred.
//
//****************************************************************************
static void
DataSent(void *pvInstance, unsigned int ulInfo)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnDataSent)
        {
            pDeviceInfo->sCallbacks.pfnDataSent(
                psDevice->psDevices[ulIdx].pvInstance, ulInfo);
        }
    }
}

//****************************************************************************
//
// This function is called to handle data being received back from the host so
// that the application callback can be called when the new data is ready.
//
//****************************************************************************
static void
DataReceived(void *pvInstance, unsigned int ulInfo)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnDataReceived)
        {
            pDeviceInfo->sCallbacks.pfnDataReceived(
                psDevice->psDevices[ulIdx].pvInstance, ulInfo);
        }
    }
}

//****************************************************************************
//
// This function will check if any device classes need an endpoint handler
// called.
//
//****************************************************************************
static void
HandleEndpoints(void *pvInstance, unsigned int ulStatus)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnEndpointHandler)
        {
            pDeviceInfo->sCallbacks.pfnEndpointHandler(
                psDevice->psDevices[ulIdx].pvInstance, ulStatus);
        }
    }
}

//****************************************************************************
//
// This function is called by the USB device stack whenever the device is
// disconnected from the host.
//
//****************************************************************************
static void
HandleDisconnect(void *pvInstance)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    //
    // Inform the application that the device has been disconnected.
    //
    if(psDevice->pfnCallback)
    {
        psDevice->pfnCallback(pvInstance, USB_EVENT_DISCONNECTED, 0, 0);
    }

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnEndpointHandler)
        {
            pDeviceInfo->sCallbacks.pfnDisconnectHandler(
                psDevice->psDevices[ulIdx].pvInstance);
        }
    }
}

//****************************************************************************
//
// This function is called by the USB device stack whenever the device
// interface changes.  It will be passed on to the device classes if they have
// a handler for this function.
//
//****************************************************************************
static void
InterfaceChange(void *pvInstance, unsigned char ucInterfaceNum,
                unsigned char ucAlternateSetting)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnInterfaceChange)
        {
            pDeviceInfo->sCallbacks.pfnInterfaceChange(
                psDevice->psDevices[ulIdx].pvInstance, ucInterfaceNum,
                ucAlternateSetting);
        }
    }
}

//****************************************************************************
//
// This function is called by the USB device stack whenever the device
// configuration changes. It will be passed on to the device classes if they
// have a handler for this function.
//
//****************************************************************************
static void
ConfigChangeHandler(void *pvInstance, unsigned int ulValue)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnConfigChange)
        {
            pDeviceInfo->sCallbacks.pfnConfigChange(
                    psDevice->psDevices[ulIdx].pvInstance, ulValue);
        }
    }
}

//****************************************************************************
//
// This function is called by the USB device stack whenever a non-standard
// request is received.
//
// \param pvInstance
// \param pUSBRequest points to the request received.
//
// This call  will be passed on to the device classes if they have a handler
// for this function.
//
// \return None.
//
//****************************************************************************
static void
HandleRequests(void *pvInstance, tUSBRequest *pUSBRequest)
{
    unsigned int ulIdx;
    const tDeviceInfo *pDeviceInfo;
    tUSBDCompositeDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the device instance pointer.
    //
    psDevice = (tUSBDCompositeDevice *)pvInstance;

    for(ulIdx = 0; ulIdx < psDevice->ulNumDevices; ulIdx++)
    {
        pDeviceInfo = psDevice->psDevices[ulIdx].psDevice;

        if(pDeviceInfo->sCallbacks.pfnRequestHandler)
        {
            pDeviceInfo->sCallbacks.pfnRequestHandler(
                psDevice->psDevices[ulIdx].pvInstance, pUSBRequest);
        }
    }
}

//****************************************************************************
//
// This function handles sending interface number changes to device instances.
//
//****************************************************************************
static void
CompositeIfaceChange(tCompositeEntry *pCompDevice, unsigned char ucOld,
                     unsigned char ucNew)
{
    unsigned char pucInterfaces[2];

    if(pCompDevice->psDevice->sCallbacks.pfnDeviceHandler)
    {
        //
        // Create the data to pass to the device handler.
        //
        pucInterfaces[0] = ucOld;
        pucInterfaces[1] = ucNew;

        //
        // Call the device handler to inform the class of the interface number
        // change.
        //
        pCompDevice->psDevice->sCallbacks.pfnDeviceHandler(
            pCompDevice->pvInstance, USB_EVENT_COMP_IFACE_CHANGE,
            (void *)pucInterfaces);
    }
}

//****************************************************************************
//
// This function handles sending endpoint number changes to device instances.
//
//****************************************************************************
static void
CompositeEPChange(tCompositeEntry *pCompDevice, unsigned char ucOld,
                     unsigned char ucNew)
{
    unsigned char pucInterfaces[2];
    unsigned char ucIndex;

    if(pCompDevice->psDevice->sCallbacks.pfnDeviceHandler)
    {
        //
        // Create the data to pass to the device handler.
        //
        pucInterfaces[0] = ucOld;
        pucInterfaces[1] = ucNew;

        ucNew--;

        if(ucOld & USB_RTYPE_DIR_IN)
        {
            ucIndex = (ucOld & ~USB_RTYPE_DIR_IN) - 1;

            g_sUSBCompositeFIFOConfig.sIn[ucNew].bDoubleBuffer =
                pCompDevice->psDevice->psFIFOConfig->sIn[ucIndex].bDoubleBuffer;

            g_sUSBCompositeFIFOConfig.sIn[ucNew].cMultiplier =
                pCompDevice->psDevice->psFIFOConfig->sIn[ucIndex].cMultiplier;

            g_sUSBCompositeFIFOConfig.sIn[ucNew].usEPFlags =
                pCompDevice->psDevice->psFIFOConfig->sIn[ucIndex].usEPFlags;
        }
        else
        {
            ucIndex = ucOld - 1;

            g_sUSBCompositeFIFOConfig.sOut[ucNew].bDoubleBuffer =
               pCompDevice->psDevice->psFIFOConfig->sOut[ucIndex].bDoubleBuffer;

            g_sUSBCompositeFIFOConfig.sOut[ucNew].cMultiplier =
                pCompDevice->psDevice->psFIFOConfig->sOut[ucIndex].cMultiplier;

            g_sUSBCompositeFIFOConfig.sOut[ucNew].usEPFlags =
                pCompDevice->psDevice->psFIFOConfig->sOut[ucIndex].usEPFlags;
        }
        //
        // Call the device handler to inform the class of the interface number
        // change.
        //
        pCompDevice->psDevice->sCallbacks.pfnDeviceHandler(
            pCompDevice->pvInstance, USB_EVENT_COMP_EP_CHANGE,
            (void *)pucInterfaces);
    }
}

//****************************************************************************
//
// This function merges the configuration descriptors into a single multiple
// instance device.
//
//****************************************************************************
unsigned int
BuildCompositeDescriptor(tUSBDCompositeDevice *psCompDevice)
{
    unsigned int ulIdx, ulOffset, ulCPIdx, ulFixINT, ulDev;
    unsigned short usTotalLength, usBytes;
    unsigned char ucInterface, ucINEndpoint, ucOUTEndpoint;
    unsigned char *pucData, *pucConfig;
    const tConfigHeader *pConfigHeader;
    tDescriptorHeader *psHeader;
    const unsigned char *pucDescriptor;
    tInterfaceDescriptor *psInterface;
    tEndpointDescriptor *psEndpoint;
    const tDeviceInfo *psDevice;

    //
    // Save the number of devices to look through.
    //
    ulDev = 0;
    ulIdx = 0;
    ucInterface = 0;
    ucINEndpoint = 1;
    ucOUTEndpoint = 1;
    ulOffset = 0;
    ulFixINT = 0;

    //
    // This puts the first section pointer in the first entry in the list
    // of sections.
    //
    psCompDevice->psPrivateData->ppsCompSections[0] =
        &psCompDevice->psPrivateData->psCompSections[0];

    //
    // Put the pointer to this instances configuration descriptor into the
    // front of the list.
    //
    psCompDevice->psPrivateData->ppsCompSections[0]->pucData =
        (unsigned char *)&psCompDevice->psPrivateData->sConfigDescriptor;

    psCompDevice->psPrivateData->ppsCompSections[0]->ucSize =
        psCompDevice->psPrivateData->sConfigDescriptor.bLength;

    //
    // The configuration descriptor is 9 bytes so initialize the total length
    // to 9 bytes.
    //
    usTotalLength = 9;

    //
    // Copy the section pointer into the section array for the composite
    // device.  This is awkward but is required given the definition
    // of the structures.
    //
    psCompDevice->psPrivateData->ppsCompSections[1] =
        &psCompDevice->psPrivateData->psCompSections[1];

    //
    // Copy the point to the application supplied space into the section list.
    //
    psCompDevice->psPrivateData->ppsCompSections[1]->ucSize = 0;
    psCompDevice->psPrivateData->ppsCompSections[1]->pucData =
        psCompDevice->psPrivateData->pucData;

    //
    // Create a local pointer to the data that is used to copy data from
    // the other devices into the composite descriptor.
    //
    pucData = psCompDevice->psPrivateData->pucData;

    while(ulDev < psCompDevice->ulNumDevices)
    {
        //
        // Save the current starting address of this descriptor.
        //
        pucConfig = pucData + ulOffset;

        //
        // Create a local pointer to the configuration header.
        //
        psDevice = psCompDevice->psDevices[ulDev].psDevice;

        pConfigHeader = psDevice->ppConfigDescriptors[0];

        //
        // Loop through the sections, skipping the first which is always the
        // configuration descriptor for the device.
        //
        for(ulIdx = 1; ulIdx < pConfigHeader->ucNumSections; ulIdx++)
        {
            //
            // Initialize the local offset in this descriptor.
            //
            usBytes = 0;

            //
            // Get a pointer to the configuration descriptor.
            //
            pucDescriptor = pConfigHeader->psSections[ulIdx]->pucData;

            //
            // Bounds check the allocated space and return if there is not
            // enough space.
            //
            if(ulOffset > psCompDevice->psPrivateData->ulDataSize)
            {
                return(1);
            }

            //
            // Copy the descriptor from the device into the descriptor list.
            //
            for(ulCPIdx = 0;
                ulCPIdx < pConfigHeader->psSections[ulIdx]->ucSize;
                ulCPIdx++)
            {
                pucData[ulCPIdx + ulOffset] = pucDescriptor[ulCPIdx];
            }

            //
            // Read out the descriptors in this section.
            //
            while(usBytes < pConfigHeader->psSections[ulIdx]->ucSize)
            {
                //
                // Create a descriptor header pointer.
                //
                psHeader = (tDescriptorHeader *)&pucData[ulOffset + usBytes];

                //
                // Check for interface descriptors and modify the numbering to
                // match the composite device.
                //
                if(psHeader->bDescriptorType == USB_DTYPE_INTERFACE)
                {
                    psInterface = (tInterfaceDescriptor *)psHeader;

                    //
                    // See if this is an alternate setting or the initial
                    // setting.
                    //
                    if(psInterface->bAlternateSetting != 0)
                    {
                        //
                        // If this is an alternate setting then use the
                        // previous interface number because the current one
                        // has already been incremented.
                        //
                        psInterface->bInterfaceNumber = ucInterface - 1;
                    }
                    else
                    {
                        //
                        // Notify the class that it's interface number has
                        // changed.
                        //
                        CompositeIfaceChange(&psCompDevice->psDevices[ulDev],
                                             psInterface->bInterfaceNumber,
                                             ucInterface);
                        //
                        // This was the non-alternate setting so save the
                        // value and move to the next interface number.
                        //
                        psInterface->bInterfaceNumber = ucInterface;

                        //
                        // No strings allowed on interface descriptors for
                        // composite devices.
                        //
                        psInterface->iInterface = 0;

                        ucInterface++;
                    }
                }
                //
                // Check for endpoint descriptors and modify the numbering to
                // match the composite device.
                //
                else if(psHeader->bDescriptorType == USB_DTYPE_ENDPOINT)
                {
                    psEndpoint = (tEndpointDescriptor *)psHeader;

                    //
                    // Check if this is an IN or OUT endpoint.
                    //
                    if(psEndpoint->bEndpointAddress & USB_RTYPE_DIR_IN)
                    {
                        //
                        // Check if this is the special Fixed Interrupt class
                        // and this is the interrupt endpoint.
                        //
                        if(((psEndpoint->bmAttributes & USB_EP_ATTR_TYPE_M) ==
                                USB_EP_ATTR_INT) &&
                           (psCompDevice->usPID == USB_PID_COMP_SERIAL))
                        {
                            //
                            // Check if the Fixed Interrupt endpoint has been
                            // set yet.
                            //
                            if(ulFixINT == 0)
                            {
                                //
                                // Allocate he fixed interrupt endpoint and
                                // save its number.
                                //
                                ulFixINT = ucINEndpoint++;
                            }

                            CompositeEPChange(&psCompDevice->psDevices[ulDev],
                                              psEndpoint->bEndpointAddress,
                                              ulFixINT);

                            psEndpoint->bEndpointAddress = ulFixINT |
                                                           USB_RTYPE_DIR_IN;
                        }
                        else
                        {
                            //
                            // Notify the class that it's interface number has
                            // changed.
                            //
                            CompositeEPChange(&psCompDevice->psDevices[ulDev],
                                              psEndpoint->bEndpointAddress,
                                              ucINEndpoint);

                            psEndpoint->bEndpointAddress = ucINEndpoint++ |
                                                           USB_RTYPE_DIR_IN;
                        }
                    }
                    else
                    {
                        //
                        // Notify the class that it's interface number has
                        // changed.
                        //
                        CompositeEPChange(&psCompDevice->psDevices[ulDev],
                                          psEndpoint->bEndpointAddress,
                                          ucOUTEndpoint);
                        psEndpoint->bEndpointAddress = ucOUTEndpoint++;
                    }
                }

                //
                // Move on to the next descriptor.
                //
                usBytes += psHeader->bLength;
            }

            ulOffset += pConfigHeader->psSections[ulIdx]->ucSize;

            usTotalLength += usBytes;
        }

        //
        // Allow the device class to make adjustments to the configuration
        // descriptor.
        //
        psCompDevice->psDevices[ulDev].psDevice->sCallbacks.pfnDeviceHandler(
                psCompDevice->psDevices[ulDev].pvInstance,
                USB_EVENT_COMP_CONFIG, (void *)pucConfig);

        ulDev++;
    }

    //
    // Modify the configuration descriptor to match the number of interfaces
    // and the new total size.
    //
    psCompDevice->psPrivateData->sCompConfigHeader.ucNumSections = 2;
    psCompDevice->psPrivateData->ppsCompSections[1]->ucSize = ulOffset;
    psCompDevice->psPrivateData->sConfigDescriptor.bNumInterfaces =
       ucInterface;
    psCompDevice->psPrivateData->sConfigDescriptor.wTotalLength =
       usTotalLength;

    return(0);
}

//****************************************************************************
//
//! This function should be called once for the composite class device to
//! initialized basic operation and prepare for enumeration.
//!
//! \param ulIndex is the index of the USB controller to initialize for
//! composite device operation.
//! \param psDevice points to a structure containing parameters customizing
//! the operation of the composite device.
//! \param ulSize is the size in bytes of the data pointed to by the
//! \e pucData parameter.
//! \param pucData is the data area that the composite class can use to build
//! up descriptors.
//!
//! In order for an application to initialize the USB composite device class,
//! it must first call this function with the a valid composite device class
//! structure in the \e psDevice parameter.  This allows this function to
//! initialize the USB controller and device code to be prepared to enumerate
//! and function as a USB composite device.  The \e ulSize and \e pucData
//! parameters should be large enough to hold all of the class instances
//! passed in via the psDevice structure.  This is typically the full size of
//! the configuration descriptor for a device minus its configuration
//! header(9 bytes).
//!
//! This function returns a void pointer that must be passed in to all other
//! APIs used by the composite class.
//!
//! See the documentation on the tUSBDCompositeDevice structure for more
//! information on how to properly fill the structure members.
//!
//! \return This function returns 0 on failure or a non-zero void pointer on
//! success.
//
//****************************************************************************
void *
USBDCompositeInit(unsigned int ulIndex, tUSBDCompositeDevice *psDevice,
        unsigned int ulSize, unsigned char *pucData)
{
    tCompositeInstance *psInst;
    int iIdx;
    unsigned char *pucTemp;

    //
    // Check parameter validity.
    //
    ASSERT(ulIndex == 0);
    ASSERT(psDevice);
    ASSERT(psDevice->ppStringDescriptors);
    ASSERT(psDevice->psPrivateData);

    //
    // Initialize the work space in the passed instance structure.
    //
    psInst = psDevice->psPrivateData;
    psInst->ulDataSize = ulSize;
    psInst->pucData = pucData;

    //
    // Set the device information for the composite device.
    //
    psInst->psDevInfo = &g_sCompositeDeviceInfo;

    g_pCompConfigDescriptors[0] = &psInst->sCompConfigHeader;
    g_pCompConfigDescriptors[0]->ucNumSections = 0;
    g_pCompConfigDescriptors[0]->psSections =
      (const tConfigSection * const *)psDevice->psPrivateData->ppsCompSections;

    //
    // Create a byte pointer to use with the copy.
    //
    pucTemp = (unsigned char *)&psInst->sConfigDescriptor;

    //
    // Copy the default configuration descriptor into the instance data.
    //
    for(iIdx = 0; iIdx < g_pCompConfigDescriptor[0]; iIdx++)
    {
        pucTemp[iIdx] = g_pCompConfigDescriptor[iIdx];
    }

    //
    // Create a byte pointer to use with the copy.
    //
    pucTemp = (unsigned char *)&psInst->sDeviceDescriptor;

    //
    // Copy the default configuration descriptor into the instance data.
    //
    for(iIdx = 0; iIdx < g_pCompDeviceDescriptor[0]; iIdx++)
    {
        pucTemp[iIdx] = g_pCompDeviceDescriptor[iIdx];
    }

    //
    // Fix up the device descriptor with the client-supplied values.
    //
    psInst->sDeviceDescriptor.idVendor = psDevice->usVID;
    psInst->sDeviceDescriptor.idProduct = psDevice->usPID;

    //
    // Fix up the configuration descriptor with client-supplied values.
    //
    psInst->sConfigDescriptor.bmAttributes = psDevice->ucPwrAttributes;
    psInst->sConfigDescriptor.bMaxPower =
        (unsigned char)(psDevice->usMaxPowermA>>1);

    g_sCompositeDeviceInfo.pDeviceDescriptor =
        (const unsigned char *)&psInst->sDeviceDescriptor;

    //
    // Plug in the client's string stable to the device information
    // structure.
    //
    psInst->psDevInfo->ppStringDescriptors = psDevice->ppStringDescriptors;
    psInst->psDevInfo->ulNumStringDescriptors =
        psDevice->ulNumStringDescriptors;

    //
    // Create the combined descriptors.
    //
    if(BuildCompositeDescriptor(psDevice))
    {
        return(0);
    }

    //
    // Set the instance data for this device.
    //
    psInst->psDevInfo->pvInstance = (void *)psDevice;

    //
    // All is well so now pass the descriptors to the lower layer and put
    // the bulk device on the bus.
    //
    USBDCDInit(ulIndex, psInst->psDevInfo);

    //
    // Return the pointer to the instance indicating that everything went
    // well.
    //
    return ((void *)psDevice);
}

//****************************************************************************
//
//! Shuts down the composite device.
//!
//! \param pvInstance is the pointer to the device instance structure as
//! returned by USBDCompositeInit().
//!
//! This function terminates composite device interface for the instance
//! supplied. Following this call, the \e pvInstance instance should not me
//! used in any other calls.
//!
//! \return None.
//
//****************************************************************************
void
USBDCompositeTerm(void *pvInstance)
{
    ASSERT(pvInstance != 0);

}

//****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//****************************************************************************

