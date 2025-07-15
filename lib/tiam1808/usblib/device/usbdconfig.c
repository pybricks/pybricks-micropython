//*****************************************************************************
//
// usbdconfig.c - High level USB device configuration function.
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

//#include "hw_memmap.h"
#include "hw_usb.h"
#include "hw_types.h"
#include "debug.h"
#include "usb.h"
#include "usblib.h"
#include "usbdevice.h"

//*****************************************************************************
//
//! \addtogroup device_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Mask used to preserve various endpoint configuration flags.
//
//*****************************************************************************
#define EP_FLAGS_MASK           (USB_EP_MODE_MASK | USB_EP_DEV_IN |           \
                                 USB_EP_DEV_OUT)

//*****************************************************************************
//
// Structure used in compiling FIFO size and endpoint properties from a
// configuration descriptor.
//
//*****************************************************************************
typedef struct
{
    unsigned int ulSize[2];
    unsigned int ulType[2];
}
tUSBEndpointInfo;

//*****************************************************************************
//
// Indices used when accessing the tUSBEndpointInfo structure.
//
//*****************************************************************************
#define EP_INFO_IN              0
#define EP_INFO_OUT             1

//*****************************************************************************
//
// Given a maximum packet size and the user's FIFO scaling requirements,
// determine the flags to use to configure the endpoint FIFO and the number
// of bytes of FIFO space occupied.
//
//*****************************************************************************
static unsigned int
GetEndpointFIFOSize(unsigned int ulMaxPktSize, const tFIFOEntry *psFIFOParams,
                    unsigned int *pupBytesUsed)
{
    unsigned int ulBytes;
    unsigned int ulLoop;
    unsigned int ulFIFOSize;

    //
    // A zero multiplier would not be a good thing.
    //
    ASSERT(psFIFOParams->cMultiplier);

    //
    // What is the basic size required for a single buffered FIFO entry
    // containing the required number of packets?
    //
    ulBytes = ulMaxPktSize * (unsigned int)psFIFOParams->cMultiplier;

    //
    // Now we need to find the nearest supported size that accommodates the
    // requested size.  Step through each of the supported sizes until we
    // find one that will do.
    //
    for(ulLoop = USB_FIFO_SZ_8; ulLoop <= USB_FIFO_SZ_8192; ulLoop++)
    {
        //
        // How many bytes does this FIFO value represent?
        //
        ulFIFOSize = USB_FIFO_SZ_TO_BYTES(ulLoop);

        //
        // Is this large enough to satisfy the request?
        //
        if(ulFIFOSize >= ulBytes)
        {
            //
            // Yes - are we being asked to double-buffer the FIFO for this
            // endpoint?
            //
            if(psFIFOParams->bDoubleBuffer)
            {
                //
                // Yes - FIFO requirement is double in this case.
                //
                *pupBytesUsed = ulFIFOSize * 2;
                return(ulLoop | USB_FIFO_SIZE_DB_FLAG);
            }
            else
            {
                //
                // No double buffering so just return the size and associated
                // flag.
                //
                *pupBytesUsed = ulFIFOSize;
                return(ulLoop);
            }
        }
    }

    //
    // If we drop out, we can't support the FIFO size requested.  Signal a
    // problem by returning 0 in the pBytesUsed
    //
    *pupBytesUsed = 0;
    return(USB_FIFO_SZ_8);
}

//*****************************************************************************
//
// Translate a USB endpoint descriptor into the values we need to pass to the
// USBDevEndpointConfigSet() API.
//
//*****************************************************************************
static void
GetEPDescriptorType(tEndpointDescriptor *psEndpoint, unsigned int *pulEPIndex,
                    unsigned int *pulMaxPktSize, unsigned int *pulFlags)
{
    //
    // Get the endpoint index.
    //
    *pulEPIndex = psEndpoint->bEndpointAddress & USB_EP_DESC_NUM_M;

    //
    // Extract the maximum packet size.
    //
    *pulMaxPktSize = psEndpoint->wMaxPacketSize & USB_EP_MAX_PACKET_COUNT_M;

    //
    // Is this an IN or an OUT endpoint?
    //
    *pulFlags = (psEndpoint->bEndpointAddress & USB_EP_DESC_IN) ?
                 USB_EP_DEV_IN : USB_EP_DEV_OUT;

    //
    // Set the endpoint mode.
    //
    switch(psEndpoint->bmAttributes & USB_EP_ATTR_TYPE_M)
    {
        case USB_EP_ATTR_CONTROL:
            *pulFlags |= USB_EP_MODE_CTRL;
            break;

        case USB_EP_ATTR_BULK:
            *pulFlags |= USB_EP_MODE_BULK;
            break;

        case USB_EP_ATTR_INT:
            *pulFlags |= USB_EP_MODE_INT;
            break;

        case USB_EP_ATTR_ISOC:
            *pulFlags |= USB_EP_MODE_ISOC;
            break;
    }
}

//*****************************************************************************
//
//! Configure the USB controller appropriately for the device whose config
//! descriptor is passed.
//!
//! \param ulIndex is the zero-based index of the USB controller which is to
//! be configured.
//! \param psConfig is a pointer to the configuration descriptor that the
//! USB controller is to be set up to support.
//! \param psFIFOConfig is a pointer to an array of NUM_USB_EP tFIFOConfig
//! structures detailing how the FIFOs are to be set up for each endpoint
//! used by the configuration.
//!
//! This function may be used to initialize a USB controller to operate as
//! the device whose configuration descriptor is passed.  The function
//! enables the USB controller, partitions the FIFO appropriately and
//! configures each endpoint required by the configuration.  If the supplied
//! configuration supports multiple alternate settings for any interface,
//! the USB FIFO is set up assuming the worst case use (largest packet size
//! for a given endpoint in any alternate setting using that endpoint) to
//! allow for on-the-fly alternate setting changes later.  On return from this
//! function, the USB controller is configured for correct operation of
//! the default configuration of the device described by the descriptor passed.
//!
//! The \e psFIFOConfig parameter allows the caller to provide additional
//! information on USB FIFO configuration that cannot be determined merely
//! by parsing the configuration descriptor.  The descriptor provides
//! information on the endpoints that are to be used and the maximum packet
//! size for each but cannot determine whether, for example, double buffering
//! is to be used or how many packets the application wants to be able to
//! store in a given endpoint's FIFO.
//!
//! USBDCDConfig() is an optional call and applications may chose to make
//! direct calls to SysCtlPeripheralEnable(), SysCtlUSBPLLEnable(),
//! USBDevEndpointConfigSet() and USBFIFOConfigSet() instead of using this
//! function.  If this function is used, it must be called prior to
//! USBDCDInit() since this call assumes that the low level hardware
//! configuration has been completed before it is made.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
USBDeviceConfig(unsigned int ulIndex, const tConfigHeader *psConfig,
                const tFIFOConfig *psFIFOConfig)
{
    unsigned int ulLoop;
    unsigned int ulCount;
    unsigned int ulNumInterfaces;
    unsigned int ulEpIndex;
    unsigned int ulEpType;
    unsigned int ulMaxPkt;
    unsigned int ulNumEndpoints;
    unsigned int ulFlags;
    unsigned int ulBytesUsed;
    unsigned int ulSection;
    tInterfaceDescriptor *psInterface;
    tEndpointDescriptor *psEndpoint;
    tUSBEndpointInfo psEPInfo[NUM_USB_EP - 1];

    //
    // We only support 1 USB controller currently.
    //
    ASSERT(ulIndex == 0);

    //
    // Catch bad pointers in a debug build.
    //
    ASSERT(psConfig);
    ASSERT(psFIFOConfig);

    //
    // Clear out our endpoint info.
    //
    for(ulLoop = 0; ulLoop < (NUM_USB_EP - 1); ulLoop++)
    {
        psEPInfo[ulLoop].ulSize[EP_INFO_IN] = 0;
        psEPInfo[ulLoop].ulType[EP_INFO_IN] = 0;
        psEPInfo[ulLoop].ulSize[EP_INFO_OUT] = 0;
        psEPInfo[ulLoop].ulType[EP_INFO_OUT] = 0;
    }

    //
    // How many (total) endpoints does this configuration describe?
    //
    ulNumEndpoints = USBDCDConfigDescGetNum(psConfig,
                                            USB_DTYPE_ENDPOINT);

    //
    // How many interfaces are included?
    //
    ulNumInterfaces = USBDCDConfigDescGetNum(psConfig,
                                             USB_DTYPE_INTERFACE);

    //
    // Look at each endpoint and determine the largest max packet size for
    // each endpoint.  This will determine how we partition the USB FIFO.
    //
    for(ulLoop = 0; ulLoop < ulNumEndpoints; ulLoop++)
    {
        //
        // Get a pointer to the endpoint descriptor.
        //
        psEndpoint = (tEndpointDescriptor *)USBDCDConfigDescGet(
                                psConfig, USB_DTYPE_ENDPOINT, ulLoop,
                                &ulSection);

        //
        // Extract the endpoint number and whether it is an IN or OUT
        // endpoint.
        //
        ulEpIndex = (unsigned int)
                        psEndpoint->bEndpointAddress & USB_EP_DESC_NUM_M;
        ulEpType =  (psEndpoint->bEndpointAddress & USB_EP_DESC_IN) ?
                     EP_INFO_IN : EP_INFO_OUT;

        //
        // Make sure the endpoint number is valid for our controller.  If not,
        // return false to indicate an error.  Note that 0 is invalid since
        // you shouldn't reference endpoint 0 in the config descriptor.
        //
        if((ulEpIndex >= NUM_USB_EP) || (ulEpIndex == 0))
        {
            return(false);
        }

        //
        // Does this endpoint have a max packet size requirement larger than
        // any previous use we have seen?
        //
        if(psEndpoint->wMaxPacketSize >
           psEPInfo[ulEpIndex - 1].ulSize[ulEpType])
        {
            //
            // Yes - remember the new maximum packet size.
            //
            psEPInfo[ulEpIndex - 1].ulSize[ulEpType] =
                psEndpoint->wMaxPacketSize;
        }
    }

    //
    // At this point, we have determined the maximum packet size required
    // for each endpoint by any possible alternate setting of any interface
    // in this configuration.  Now determine the endpoint settings required
    // for the interface setting we are actually going to use.
    //
    for(ulLoop = 0; ulLoop < ulNumInterfaces; ulLoop++)
    {
        //
        // Get the next interface descriptor in the config descriptor.
        //
        psInterface = USBDCDConfigGetInterface(psConfig,
                                               ulLoop,
                                               USB_DESC_ANY,
                                               &ulSection);

        //
        // Is this the default interface (bAlternateSetting set to 0)?
        //
        if(psInterface && (psInterface->bAlternateSetting == 0))
        {
            //
            // This is an interface we are interested in so gather the
            // information on its endpoints.
            //
            ulNumEndpoints = (unsigned int)psInterface->bNumEndpoints;

            //
            // Walk through each endpoint in this interface and configure
            // it appropriately.
            //
            for(ulCount = 0; ulCount < ulNumEndpoints; ulCount++)
            {
                //
                // Get a pointer to the endpoint descriptor.
                //
                psEndpoint = USBDCDConfigGetInterfaceEndpoint(psConfig,
                                            psInterface->bInterfaceNumber,
                                            psInterface->bAlternateSetting,
                                            ulCount);

                //
                // Make sure we got a good pointer.
                //
                if(psEndpoint)
                {
                    //
                    // Determine maximum packet size and flags from the
                    // endpoint descriptor.
                    //
                    GetEPDescriptorType(psEndpoint, &ulEpIndex, &ulMaxPkt,
                                        &ulFlags);

                    //
                    // Make sure no-one is trying to configure endpoint 0.
                    //
                    if(!ulEpIndex)
                    {
                        return(false);
                    }

                    //
                    // Include any additional flags that the user wants.
                    //
                    if((ulFlags & (USB_EP_DEV_IN | USB_EP_DEV_OUT)) ==
                        USB_EP_DEV_IN)
                    {
                        //
                        // This is an IN endpoint.
                        //
                        ulFlags |= (unsigned int)(
                                  psFIFOConfig->sIn[ulEpIndex - 1].usEPFlags);
                        psEPInfo[ulEpIndex - 1].ulType[EP_INFO_IN] = ulFlags;
                    }
                    else
                    {
                        //
                        // This is an OUT endpoint.
                        //
                        ulFlags |= (unsigned int)(
                                  psFIFOConfig->sOut[ulEpIndex - 1].usEPFlags);
                        psEPInfo[ulEpIndex - 1].ulType[EP_INFO_OUT] = ulFlags;
                    }

                    //
                    // Set the endpoint configuration.
                    //
                    USBDevEndpointConfigSet(USB0_BASE,
                                            INDEX_TO_USB_EP(ulEpIndex),
                                            ulMaxPkt, ulFlags);
                }
            }
        }
    }

    //
    // At this point, we have configured all the endpoints that are to be
    // used by this configuration's alternate setting 0.  Now we go on and
    // partition the FIFO based on the maximum packet size information we
    // extracted earlier.  Endpoint 0 is automatically configured to use the
    // first MAX_PACKET_SIZE_EP0 bytes of the FIFO so we start from there.
    //
    ulCount = MAX_PACKET_SIZE_EP0;
    for(ulLoop = 1; ulLoop < NUM_USB_EP; ulLoop++)
    {
        //
        // Configure the IN endpoint at this index if it is referred to
        // anywhere.
        //
        if(psEPInfo[ulLoop - 1].ulSize[EP_INFO_IN])
        {
            //
            // What FIFO size flag do we use for this endpoint?
            //
            ulMaxPkt = GetEndpointFIFOSize(
                                     psEPInfo[ulLoop - 1].ulSize[EP_INFO_IN],
                                     &(psFIFOConfig->sIn[ulLoop - 1]),
                                     &ulBytesUsed);

            //
            // If we are told that 0 bytes of FIFO will be used, this implies
            // that there is an error in psFIFOConfig or the descriptor
            // somewhere so return an error indicator to the caller.
            //
            if(!ulBytesUsed)
            {
                return(false);
            }

            //
            // Now actually configure the FIFO for this endpoint.
            //
            USBFIFOConfigSet(USB0_BASE, INDEX_TO_USB_EP(ulLoop), ulCount,
                             ulMaxPkt, USB_EP_DEV_IN);
            ulCount += ulBytesUsed;
        }

        //
        // Configure the OUT endpoint at this index.
        //
        if(psEPInfo[ulLoop - 1].ulSize[EP_INFO_OUT])
        {
            //
            // What FIFO size flag do we use for this endpoint?
            //
            ulMaxPkt = GetEndpointFIFOSize(
                                     psEPInfo[ulLoop - 1].ulSize[EP_INFO_OUT],
                                     &(psFIFOConfig->sOut[ulLoop - 1]),
                                     &ulBytesUsed);

            //
            // If we are told that 0 bytes of FIFO will be used, this implies
            // that there is an error in psFIFOConfig or the descriptor
            // somewhere so return an error indicator to the caller.
            //
            if(!ulBytesUsed)
            {
                return(false);
            }

            //
            // Now actually configure the FIFO for this endpoint.
            //
            USBFIFOConfigSet(USB0_BASE, INDEX_TO_USB_EP(ulLoop), ulCount,
                             ulMaxPkt, USB_EP_DEV_OUT);
            ulCount += ulBytesUsed;
        }

    }

    //
    // If we get to the end, all is well.
    //
    return(true);
}

//*****************************************************************************
//
//! Configure the affected USB endpoints appropriately for one alternate
//! interface setting.
//!
//! \param ulIndex is the zero-based index of the USB controller which is to
//! be configured.
//! \param psConfig is a pointer to the configuration descriptor that contains
//! the interface whose alternate settings is to be configured.
//! \param ucInterfaceNum is the number of the interface whose alternate
//! setting is to be configured.  This number corresponds to the
//! bInterfaceNumber field in the desired interface descriptor.
//! \param ucAlternateSetting is the alternate setting number for the desired
//! interface.  This number corresponds to the bAlternateSetting field in the
//! desired interface descriptor.
//!
//! This function may be used to reconfigure the endpoints of an interface
//! for operation in one of the interface's alternate settings.  Note that this
//! function assumes that the endpoint FIFO settings will not need to change
//! and only the endpoint mode is changed.  This assumption is valid if the
//! USB controller was initialized using a previous call to USBDCDConfig().
//!
//! In reconfiguring the interface endpoints, any additional configuration
//! bits set in the endpoint configuration other than the direction (\b
//! USB_EP_DEV_IN or \b USB_EP_DEV_OUT) and mode (\b USB_EP_MODE_MASK) are
//! preserved.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
USBDeviceConfigAlternate(unsigned int ulIndex, const tConfigHeader *psConfig,
                         unsigned char ucInterfaceNum,
                         unsigned char ucAlternateSetting)
{
    unsigned int ulNumInterfaces;
    unsigned int ulNumEndpoints;
    unsigned int ulLoop;
    unsigned int ulCount;
    unsigned int ulMaxPkt;
    unsigned int ulOldMaxPkt;
    unsigned int ulFlags;
    unsigned int ulOldFlags;
    unsigned int ulSection;
    unsigned int ulEpIndex;
    tInterfaceDescriptor *psInterface;
    tEndpointDescriptor *psEndpoint;

    //
    // How many interfaces are included in the descriptor?
    //
    ulNumInterfaces = USBDCDConfigDescGetNum(psConfig,
                                             USB_DTYPE_INTERFACE);

    //
    // Find the interface descriptor for the supplied interface and alternate
    // setting numbers.
    //

    for(ulLoop = 0; ulLoop < ulNumInterfaces; ulLoop++)
    {
        //
        // Get the next interface descriptor in the config descriptor.
        //
        psInterface = USBDCDConfigGetInterface(psConfig, ulLoop, USB_DESC_ANY,
                                               &ulSection);

        //
        // Is this the default interface (bAlternateSetting set to 0)?
        //
        if(psInterface &&
           (psInterface->bInterfaceNumber == ucInterfaceNum) &&
           (psInterface->bAlternateSetting == ucAlternateSetting))
        {
            //
            // This is an interface we are interested in and the descriptor
            // representing the alternate setting we want so go ahead and
            // reconfigure the endpoints.
            //

            //
            // How many endpoints does this interface have?
            //
            ulNumEndpoints = (unsigned int)psInterface->bNumEndpoints;

            //
            // Walk through each endpoint in turn.
            //
            for(ulCount = 0; ulCount < ulNumEndpoints; ulCount++)
            {
                //
                // Get a pointer to the endpoint descriptor.
                //
                psEndpoint = USBDCDConfigGetInterfaceEndpoint(psConfig,
                                              psInterface->bInterfaceNumber,
                                              psInterface->bAlternateSetting,
                                              ulCount);

                //
                // Make sure we got a good pointer.
                //
                if(psEndpoint)
                {
                    //
                    // Determine maximum packet size and flags from the
                    // endpoint descriptor.
                    //
                    GetEPDescriptorType(psEndpoint, &ulEpIndex, &ulMaxPkt,
                                        &ulFlags);

                    //
                    // Make sure no-one is trying to configure endpoint 0.
                    //
                    if(!ulEpIndex)
                    {
                        return(false);
                    }

                    //
                    // Get the existing endpoint configuration and mask in the
                    // new mode and direction bits, leaving everything else
                    // unchanged.
                    //
                    ulOldFlags = ulFlags;
                    USBDevEndpointConfigGet(USB0_BASE,
                                            INDEX_TO_USB_EP(ulEpIndex),
                                            &ulOldMaxPkt,
                                            &ulOldFlags);

                    //
                    // Mask in the previous DMA and auto-set bits.
                    //
                    ulFlags = (ulFlags & EP_FLAGS_MASK) |
                              (ulOldFlags & ~EP_FLAGS_MASK);

                    //
                    // Set the endpoint configuration.
                    //
                    USBDevEndpointConfigSet(USB0_BASE,
                                            INDEX_TO_USB_EP(ulEpIndex),
                                            ulMaxPkt, ulFlags);
                }
            }

            //
            // At this point, we have reconfigured the desired interface so
            // return indicating all is well.
            //
            return(true);
        }
    }

    return(false);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
