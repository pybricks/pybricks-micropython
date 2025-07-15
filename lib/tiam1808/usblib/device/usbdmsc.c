//*****************************************************************************
//
// usbdmsc.c - USB mass storage device class driver.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6288 of the Stellaris USB Library.
//
//*****************************************************************************
//#include "inc/hw_memmap.h"

#include "hw_usb.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"
#include "usb.h"
#include "cppi41dma.h"
#include "usblib.h"
#include "usbmsc.h"
#include "usbdevice.h"
#include "usbdmsc.h"
#include "usblibpriv.h"
#include <string.h>

//*****************************************************************************
//
//! \addtogroup msc_device_class_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// DMA configuration Parameters
//
//*****************************************************************************
#define DMA_TX_MAX_CHUNK_SIZE	512 * 8 // 512 blocks size, 8 blocks
#define DMA_RX_MAX_CHUNK_SIZE	512  // 1 block

//*****************************************************************************
//
// The subset of endpoint status flags that we consider to be reception
// errors.  These are passed to the client via USB_EVENT_ERROR if seen.
//
//*****************************************************************************
#define USB_RX_ERROR_FLAGS      (USBERR_DEV_RX_DATA_ERROR |                   \
                                 USBERR_DEV_RX_OVERRUN |                      \
                                 USBERR_DEV_RX_FIFO_FULL)

//*****************************************************************************
//
// These are fields that are used by the USB descriptors for the Mass Storage
// Class.
//
//*****************************************************************************
#define USB_MSC_SUBCLASS_SCSI   0x6
#define USB_MSC_PROTO_BULKONLY  0x50

//*****************************************************************************
//
// Endpoints to use for each of the required endpoints in the driver.
//
//*****************************************************************************
#define DATA_IN_ENDPOINT        USB_EP_1
#define DATA_IN_DMA_CHANNEL    USB_EP_1// UDMA_CHANNEL_USBEP1TX
#define DATA_OUT_ENDPOINT       USB_EP_1
#define DATA_OUT_DMA_CHANNEL    USB_EP_1//UDMA_CHANNEL_USBEP1RX

//*****************************************************************************
//
// Maximum packet size for the bulk endpoints is 64 bytes.
//
//*****************************************************************************
#define DATA_IN_EP_MAX_SIZE     512
#define DATA_OUT_EP_MAX_SIZE    512

//*****************************************************************************
//
// These defines control the sizes of USB transfers for data and commands.
//
//*****************************************************************************
#define MAX_TRANSFER_SIZE       512
#define COMMAND_BUFFER_SIZE     64

//*****************************************************************************
//
// The local buffer used to read in commands and process them.
//
//*****************************************************************************
unsigned char g_pucCommand[COMMAND_BUFFER_SIZE];
unsigned char intStatus = 0;
unsigned int g_bytesRead = 0;
unsigned int g_bytesWritten = 0;


//*****************************************************************************
//
// The current transfer state is held in these variables.
//
//*****************************************************************************
static tMSCCSW g_sSCSICSW;
unsigned short gDMAflag =0;

//*****************************************************************************
//
// The current state for the SCSI commands that are being handled and are
// stored in the tMSCInstance.ucSCSIState structure member.
//
//*****************************************************************************

//
// No command in process.
//
#define STATE_SCSI_IDLE             0x00

//
// Sending and reading logical blocks.
//
#define STATE_SCSI_SEND_BLOCKS      0x01

//
// Receiving and writing logical blocks.
//
#define STATE_SCSI_RECEIVE_BLOCKS   0x02

//
// Send the status once the previous transfer is complete.
//
#define STATE_SCSI_SEND_STATUS      0x03

//
// Status was prepared to be sent and now waiting for it to have gone out.
//
#define STATE_SCSI_SENT_STATUS      0x04

#define STATE_SCSI_COMMAND_RCVD		0x05

//*****************************************************************************
//
// Device Descriptor.  This is stored in RAM to allow several fields to be
// changed at runtime based on the client's requirements.
//
//*****************************************************************************
static unsigned char g_pMSCDeviceDescriptor[] =
{
    18,                     // Size of this structure.
    USB_DTYPE_DEVICE,       // Type of this structure.
    USBShort(0x200),        // USB version 1.1 (if we say 2.0, hosts assume
                            // high-speed - see USB 2.0 spec 9.2.6.6)
    0,                      // USB Device Class (spec 5.1.1)
    0,                      // USB Device Sub-class (spec 5.1.1)
    0,                      // USB Device protocol (spec 5.1.1)
    64,                     // Maximum packet size for default pipe.
    USBShort(0),            // Vendor ID (filled in during USBDCDCInit).
    USBShort(0),            // Product ID (filled in during USBDCDCInit).
    USBShort(0x100),        // Device Version BCD.
    1,                      // Manufacturer string identifier.
    2,                      // Product string identifier.
    3,                      // Product serial number.
    1                       // Number of configurations.
};

//*****************************************************************************
//
// Mass storage device configuration descriptor.
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
//*****************************************************************************
static unsigned char g_pMSCDescriptor[] =
{
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(32),               // The total size of this full structure.
    1,                          // The number of interfaces in this
                                // configuration.
    1,                          // The unique value for this configuration.
    0,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_SELF_PWR,     // Bus Powered, Self Powered, remote wake up.
    250,                        // The maximum power in 2mA increments.
};

//*****************************************************************************
//
// The remainder of the configuration descriptor is stored in flash since we
// don't need to modify anything in it at runtime.
//
//*****************************************************************************
const unsigned char g_pMSCInterface[] =
{
    //
    // Vendor-specific Interface Descriptor.
    //
    9,                          // Size of the interface descriptor.
    USB_DTYPE_INTERFACE,        // Type of this descriptor.
    0,                          // The index for this interface.
    0,                          // The alternate setting for this interface.
    2,                          // The number of endpoints used by this
                                // interface.
    USB_CLASS_MASS_STORAGE,     // The interface class
    USB_MSC_SUBCLASS_SCSI,      // The interface sub-class.
    USB_MSC_PROTO_BULKONLY,     // The interface protocol for the sub-class
                                // specified above.
    0,                          // The string index for this interface.

    //
    // Endpoint Descriptor
    //
    7,                               // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,              // Descriptor type is an endpoint.
    USB_EP_DESC_IN | USB_EP_TO_INDEX(DATA_IN_ENDPOINT),
    USB_EP_ATTR_BULK,                // Endpoint is a bulk endpoint.
    USBShort(DATA_IN_EP_MAX_SIZE),   // The maximum packet size.
    0,                               // The polling interval for this endpoint.

    //
    // Endpoint Descriptor
    //
    7,                               // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,              // Descriptor type is an endpoint.
    USB_EP_DESC_OUT | USB_EP_TO_INDEX(DATA_OUT_ENDPOINT),
    USB_EP_ATTR_BULK,                // Endpoint is a bulk endpoint.
    USBShort(DATA_OUT_EP_MAX_SIZE),  // The maximum packet size.
    0,                               // The polling interval for this endpoint.
};

//*****************************************************************************
//
// The mass storage configuration descriptor is defined as two sections,
// one containing just the 9 byte USB configuration descriptor and the other
// containing everything else that is sent to the host along with it.
//
//*****************************************************************************
const tConfigSection g_sMSCConfigSection =
{
    sizeof(g_pMSCDescriptor),
    g_pMSCDescriptor
};

const tConfigSection g_sMSCInterfaceSection =
{
    sizeof(g_pMSCInterface),
    g_pMSCInterface
};

//*****************************************************************************
//
// This array lists all the sections that must be concatenated to make a
// single, complete bulk device configuration descriptor.
//
//*****************************************************************************
const tConfigSection *g_psMSCSections[] =
{
    &g_sMSCConfigSection,
    &g_sMSCInterfaceSection
};

#define NUM_MSC_SECTIONS (sizeof(g_psMSCSections) / sizeof(tConfigSection *))

//*****************************************************************************
//
// The header for the single configuration we support.  This is the root of
// the data structure that defines all the bits and pieces that are pulled
// together to generate the configuration descriptor.
//
//*****************************************************************************
const tConfigHeader g_sMSCConfigHeader =
{
    NUM_MSC_SECTIONS,
    g_psMSCSections
};

//*****************************************************************************
//
// Configuration Descriptor.
//
//*****************************************************************************
const tConfigHeader * const g_pMSCConfigDescriptors[] =
{
    &g_sMSCConfigHeader
};

//*****************************************************************************
//
// Various internal handlers needed by this class.
//
//*****************************************************************************
static void HandleDisconnect(void *pvInstance);
static void ConfigChangeHandler(void *pvInstance, unsigned int ulValue);
static void HandleEndpoints(void *pvInstance, unsigned int ulStatus);
static void HandleRequests(void *pvInstance, tUSBRequest *pUSBRequest);
static void USBDSCSISendStatus(const tUSBDMSCDevice *psDevice);
unsigned int USBDSCSICommand(const tUSBDMSCDevice *psDevice,
                              tMSCCBW *pSCSICBW);
static void HandleDevice(void *pvInstance, unsigned int ulRequest,
                         void *pvRequestData);

//*****************************************************************************
//
// The FIFO configuration for USB mass storage class device.
//
//*****************************************************************************
const tFIFOConfig g_sUSBMSCFIFOConfig =
{
    //
    // IN endpoints.
    //
    {
        { 1, false, USB_EP_DEV_IN | USB_EP_DMA_MODE_1 | USB_EP_AUTO_SET },
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
        { 1, false, USB_EP_DEV_OUT | USB_EP_DMA_MODE_1 | USB_EP_AUTO_CLEAR },
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

//*****************************************************************************
//
// The device information structure for the USB MSC device.
//
//*****************************************************************************
tDeviceInfo g_sMSCDeviceInfo =
{
    //
    // Device event handler callbacks.
    //
    {
        //
        // GetDescriptor
        //
        0,

        //
        // RequestHandler
        //
        HandleRequests,

        //
        // InterfaceChange
        //
        0,

        //
        // ConfigChange
        //
        ConfigChangeHandler,

        //
        // DataReceived
        //
        0,

        //
        // DataSentCallback
        //
        0,

        //
        // ResetHandler
        //
        0,

        //
        // SuspendHandler
        //
        0,

        //
        // ResumeHandler
        //
        0,

        //
        // DisconnectHandler
        //
        HandleDisconnect,

        //
        // EndpointHandler
        //
        HandleEndpoints,

        //
        // Device handler
        //
        HandleDevice
    },
    g_pMSCDeviceDescriptor,
    g_pMSCConfigDescriptors,
    0,
    0,
	&g_sUSBDefaultFIFOConfig,
    NULL,

		//&g_sUSBMSCFIFOConfig
};

//*****************************************************************************
//
//! This function is used by an application if it can detect insertion or
//! removal of the media.
//!
//! \param pvInstance is the mass storage device instance that had a media
//! change.
//! \param eMediaStatus is the updated status for the media.
//!
//! This function should be called by an application when it detects a change
//! in the status of the media in use by the USB mass storage class.  The
//! \e eMediaStatus parameter will indicate the new status of the media and
//! can also indicate that the application has no knowledge of the media state.
//!
//! There are currently the three following values for the \e eMediaStatus
//! parameter:
//! - USBDMSC_MEDIA_PRESENT indicates that the media is present or has been
//! added.
//! - USBDMSC_MEDIA_NOTPRESENT indicates that the media is not present or was
//! removed.
//! - USBDMSC_MEDIA_UNKNOWN indicates that the application has no knowledge of
//! the media state and the USB mass storage class.
//!
//! It will be left up to the application to call this function whenever it
//! detects a change or simply call it once with USBDMSC_MEDIA_UNKNOWN and
//! allow the mass storage class to infer the state from the remaining device
//! APIs.
//!
//! \note It is recommended that the application use this function to inform
//! the mass storage class of media state changes as it will lead to a more
//! responsive system.
//!
//! \return None.
//
//*****************************************************************************
void
USBDMSCMediaChange(void *pvInstance, tUSBDMSCMediaStatus eMediaStatus)
{
    const tUSBDMSCDevice *psDevice;

    //
    // Create a device instance pointer.
    //
    psDevice = pvInstance;

    //
    // Save the current media status.
    //
    psDevice->psPrivateData->eMediaStatus = eMediaStatus;
}

//*****************************************************************************
//
// This function is called to handle the interrupts on the Bulk endpoints for
// the mass storage class.
//
//*****************************************************************************
static void
HandleEndpoints(void *pvInstance, unsigned int ulStatus)
{
    const tUSBDMSCDevice *psDevice;
    tMSCInstance *psInst;
    tMSCCBW *pSCSICBW;
    unsigned int pendReg = 0;

#ifdef DMA_MODE
	unsigned int rxBuffer;
	unsigned int txBuffer;
	unsigned int dmaStatus = 0;
	unsigned int nBlocks;
#else
	unsigned int ulSize;
	unsigned int ulMaxsize = MAX_TRANSFER_SIZE;
#endif

	ASSERT(pvInstance != 0);

    //
    // Determine if the serial device is in single or composite mode because
    // the meaning of ulIndex is different in both cases.
    //
    psDevice = pvInstance;

    //
    // Initialize the workspace in the passed instance structure.
    //
    psInst = psDevice->psPrivateData;

#ifdef DMA_MODE
	// Get the Starvation interrupt status
	CppiDmaGetINTD0Status(USB_INSTANCE);

	// Get the DMA Interrupt status
	pendReg = CppiDmaGetPendStatus(USB_INSTANCE);
#endif

    //
    // Handler for the bulk IN data endpoint.
    //
    if(ulStatus & (1 << USB_EP_TO_INDEX(psInst->ucINEndpoint))
		|| (pendReg & CPDMA_TX_PENDING))
    {


#ifdef DMA_MODE
		if(pendReg & CPDMA_TX_PENDING)
		{
			 dmaStatus = dmaTxCompletion(USB_INSTANCE, psInst->ucINEndpoint);
		}
#endif

		switch(psInst->ucSCSIState)
        {
			//
            // Handle the case where we are sending out data due to a read
            // command.
            //
            case STATE_SCSI_SEND_BLOCKS:
            {

#ifndef DMA_MODE
				//
                // Decrement the number of bytes left to send.
               	//
                psInst->ulBytesToTransfer -= MAX_TRANSFER_SIZE;

				//
				//Add the bytes transfered
				//
				g_bytesRead = g_bytesRead + MAX_TRANSFER_SIZE;

#else			//
				// Check if DMA is completed, Check the remaining bytes
				//
				if(dmaStatus == DMA_TX_COMPLETED)
				{
					if(psInst->ulBytesToTransfer > DMA_TX_MAX_CHUNK_SIZE)
					{
						psInst->ulBytesToTransfer -= DMA_TX_MAX_CHUNK_SIZE;
						g_bytesRead = g_bytesRead + DMA_TX_MAX_CHUNK_SIZE;
					}
					else
					{
						g_bytesRead = g_bytesRead +psInst->ulBytesToTransfer;
						psInst->ulBytesToTransfer = 0;
					}

				}
#endif

                //
                // If we are done then move on to the status phase.
                //
                if(psInst->ulBytesToTransfer == 0)
                {

					//
                    // Set the status so that it can be sent when this
                    // response has has be successfully sent.
                    //
                    g_sSCSICSW.bCSWStatus = 0;
                    g_sSCSICSW.dCSWDataResidue = 0;
					g_bytesRead = 0;

                    //
                    // Send back the status once this transfer is complete.
                    //
                    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;

#ifdef DMA_MODE
					//
					//Disable the RX and TX DMA before sending the CSW
					//DMA is not used to send the CSW.
					//
					disableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
					disableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
#endif

					USBDSCSISendStatus(psDevice);

                    if(psDevice->pfnEventCallback)
                    {
                        psDevice->pfnEventCallback(0, USBD_MSC_EVENT_IDLE, 0,
                                                   0);
                    }

                    //
                    // The transfer is complete so don't read anymore data.
                    //
                    break;
                }


#ifndef DMA_MODE

				if(g_bytesRead == DEVICE_BLOCK_SIZE)
				{
					//
					// Move on to the next Logical Block.
					//
					psInst->ulCurrentLBA++;
					g_bytesRead = 0;
				}
#endif
				//
				// Read the new data and send it out.
				//
#ifdef DMA_MODE
				if(dmaStatus == DMA_TX_COMPLETED)
				{
					//
					//If current DMA is operation is completed, check how may byets remaining
					//to send
					//
					g_bytesRead = 0;
					psInst->ulCurrentLBA += (DMA_TX_MAX_CHUNK_SIZE / MAX_TRANSFER_SIZE);
					nBlocks = psInst->ulBytesToTransfer / MAX_TRANSFER_SIZE;

					if(nBlocks > (DMA_TX_MAX_CHUNK_SIZE / MAX_TRANSFER_SIZE))
						nBlocks = (DMA_TX_MAX_CHUNK_SIZE / MAX_TRANSFER_SIZE);

					//Allocate buffer for the remaining data
					txBuffer = (unsigned int)cppiDmaAllocnBuffer(nBlocks);

					//
					//Read the blocks and send it out
					//
					psDevice->sMediaFunctions.BlockRead(psInst->pvMedia,
						(unsigned char *)txBuffer, psInst->ulCurrentLBA, nBlocks);

					doDmaTxTransfer(USB_INSTANCE, (unsigned char *)txBuffer,
						 (nBlocks * DEVICE_BLOCK_SIZE), psInst->ucINEndpoint);
				}
#else
				//
				// Read the new data and send it out.
				//
				psDevice->sMediaFunctions.BlockRead(psInst->pvMedia,
					(unsigned char *)psInst->pulBuffer,
					psInst->ulCurrentLBA, 1);

				USBEndpointDataPut(USB0_BASE,  psInst->ucINEndpoint,
					(unsigned char *)psInst->pulBuffer, MAX_TRANSFER_SIZE);
				USBEndpointDataSend(USB0_BASE,  psInst->ucINEndpoint, USB_TRANS_IN);
#endif

				break;
            }

            //
            // Handle sending status.
            //
            case STATE_SCSI_SEND_STATUS:
            {
#ifdef DMA_MODE
				//
				//Disable the RX and TX DMA before sending the CSW
				//DMA is not used to send the CSW.
				//
				disableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
				disableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
#endif

				//
                // Indicate success and no extra data coming.
                //
                USBDSCSISendStatus(psDevice);

                break;
            }

            //
            // Handle completing sending status.
            //
            case STATE_SCSI_SENT_STATUS:
            {

#ifdef DMA_MODE
				disableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
				disableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
#endif

				psInst->ucSCSIState = STATE_SCSI_IDLE;

                break;
            }

            //
            // These cases should not occur as the being in the IDLE state due
            // to an IN interrupt is invalid.
            //
            case STATE_SCSI_IDLE:
            default:
            {
                break;
            }
        }
    }

    //
    // Handler for the bulk OUT data endpoint.
    //
    if(ulStatus & (0x10000 << USB_EP_TO_INDEX(psInst->ucOUTEndpoint))
		|| (pendReg & CPDMA_RX_PENDING))
    {

        switch(psInst->ucSCSIState)
        {
            //
            // Receiving and writing bytes to the storage device.
            //
            case STATE_SCSI_RECEIVE_BLOCKS:
            {

#ifndef DMA_MODE
				//
				//Get the data from the FIFO and send Ack
				//
				USBEndpointDataGet(psInst->ulUSBBase, psInst->ucOUTEndpoint,
					(unsigned char *)psInst->pulBuffer, &ulMaxsize);

				USBDevEndpointDataAck(psInst->ulUSBBase, psInst->ucOUTEndpoint, false);

				//
				//Write the data to the block media
				//
				psDevice->sMediaFunctions.BlockWrite(psInst->pvMedia,
					(unsigned char *)psInst->pulBuffer,
						psInst->ulCurrentLBA, 1);

#endif

#ifdef DMA_MODE
				//
				//During receive operation, we need to wait in loop till we recive all data
				//
				while(CppiDmaGetPendStatus(USB_INSTANCE) & CPDMA_RX_PENDING)
				{

					//Decrement the bytes recived
					psInst->ulBytesToTransfer -= DMA_RX_MAX_CHUNK_SIZE;
					g_bytesWritten = g_bytesWritten + DMA_RX_MAX_CHUNK_SIZE;

#else
					psInst->ulBytesToTransfer -= MAX_TRANSFER_SIZE;
					g_bytesWritten = g_bytesWritten + MAX_TRANSFER_SIZE;
#endif

#ifdef DMA_MODE
					//
					//Get the data from the RX completeion queue
					//
					rxBuffer = dmaRxCompletion(USB_INSTANCE, psInst->ucOUTEndpoint);


					//
					//Write the data to the block media
					//
					psDevice->sMediaFunctions.BlockWrite(psInst->pvMedia,
						(unsigned char *)rxBuffer, psInst->ulCurrentLBA, 1);
					//
					//Load another BD to the receive queue
					//
					doDmaRxTransfer(USB_INSTANCE, MAX_TRANSFER_SIZE,
						(unsigned char *)rxBuffer, psInst->ucOUTEndpoint);

#endif
					if(g_bytesWritten == DEVICE_BLOCK_SIZE)

					{
		               	g_bytesWritten = 0;
						psInst->ulCurrentLBA++;

					}

#ifdef DMA_MODE
				}

					cppiDmaHandleError(USB_INSTANCE);
#endif

				//
				// Check if all bytes have been received.
				//
				if(psInst->ulBytesToTransfer == 0)
                {
                    //
                    // Set the status so that it can be sent when this response
                    // has be successfully sent.
                    //
                    g_sSCSICSW.bCSWStatus = 0;
                 	g_sSCSICSW.dCSWDataResidue = 0;
					g_bytesWritten = 0;
					psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;

#ifdef DMA_MODE
					//
					//Disable the RX and TX DMA before sending the CSW
					//DMA is not used to send the CSW.
					//
					disableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
					disableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#endif
					//
                    // Indicate success and no extra data coming.
                    //

					USBDSCSISendStatus(psDevice);

                 }

                break;
            }

            //
            // If there is an OUT transfer in idle state then it was a new
            // command.
            //
            case STATE_SCSI_IDLE:
            {

#ifdef DMA_MODE

				//
				//Enable the RX DMA as it is disabled during last CSW
				//
				enableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
				while(!(CppiDmaGetPendStatus(USB_INSTANCE) & CPDMA_RX_PENDING));
				rxBuffer = dmaRxCompletion(USB_INSTANCE, psInst->ucOUTEndpoint);
				pSCSICBW = (tMSCCBW *)rxBuffer;

				//
				//Recceive the command
				//
				doDmaRxTransfer(USB_INSTANCE, MAX_TRANSFER_SIZE,
					(unsigned char *)rxBuffer, psInst->ucOUTEndpoint);

#else
                //
                // Attempt to handle the new command.
                //

                ulSize = COMMAND_BUFFER_SIZE;

				//
                // Receive the command.
                //
				USBEndpointDataGet(psInst->ulUSBBase, psInst->ucOUTEndpoint,
							  	 g_pucCommand, &ulSize);

				pSCSICBW = (tMSCCBW *)g_pucCommand;

				//
				// Acknowledge the OUT data packet.
				//
				USBDevEndpointDataAck(psInst->ulUSBBase, psInst->ucOUTEndpoint,
										false);
#endif

				//
                // If this is a valid CBW then handle it.
                //
                if(pSCSICBW->dCBWSignature == CBW_SIGNATURE)
                {
                    g_sSCSICSW.dCSWSignature = CSW_SIGNATURE;
                    g_sSCSICSW.dCSWTag = pSCSICBW->dCBWTag;
                    g_sSCSICSW.dCSWDataResidue = 0;
                    g_sSCSICSW.bCSWStatus = 0;
					USBDSCSICommand(psDevice, pSCSICBW);

                }
                else
                {
                    //
                    // Just return to the idle state since we are now out of
                    // sync with the host.  This should not happen, but this
                    // should allow the device to synchronize with the host
                    // controller.
                    //
                    psInst->ucSCSIState = STATE_SCSI_IDLE;

                }

                break;
            }
            default:
            {
                break;
            }
        }


    }


}

//*****************************************************************************
//
// Device instance specific handler.
//
//*****************************************************************************
static void
HandleDevice(void *pvInstance, unsigned int ulRequest, void *pvRequestData)
{
    tMSCInstance *psInst;
    unsigned char *pucData;

    //
    // Create the serial instance data.
    //
    psInst = ((tUSBDMSCDevice *)pvInstance)->psPrivateData;

    //
    // Create the char array used by the events supported by the USB CDC
    // serial class.
    //
    pucData = (unsigned char *)pvRequestData;

    switch(ulRequest)
    {
        //
        // This was an interface change event.
        //
        case USB_EVENT_COMP_IFACE_CHANGE:
        {
            psInst->ucInterface = pucData[1];
            break;
        }

        //
        // This was an endpoint change event.
        //
        case USB_EVENT_COMP_EP_CHANGE:
        {
            //
            // Determine if this is an IN or OUT endpoint that has changed.
            //
            if(pucData[0] & USB_EP_DESC_IN)
            {
                psInst->ucINEndpoint = INDEX_TO_USB_EP((pucData[1] & 0x7f));

            }
            else
            {
                //
                // Extract the new endpoint number.
                //
                psInst->ucOUTEndpoint = INDEX_TO_USB_EP(pucData[1] & 0x7f);

            }
            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This function is called by the USB device stack whenever the device is
// disconnected from the host.
//
//*****************************************************************************
static void
HandleDisconnect(void *pvInstance)
{
    const tUSBDMSCDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the instance pointer.
    //
    psDevice = (const tUSBDMSCDevice *)pvInstance;

    //
    // Close the drive requested.
    //
    if(psDevice->psPrivateData->pvMedia != 0)
    {
        psDevice->psPrivateData->pvMedia = 0;
        psDevice->sMediaFunctions.Close(0);
    }

    //
    // If we have a control callback, let the client know we are open for
    // business.
    //
    if(psDevice->pfnEventCallback)
    {
        //
        // Pass the connected event to the client.
        //
        psDevice->pfnEventCallback(pvInstance, USB_EVENT_DISCONNECTED, 0, 0);
    }
}

//*****************************************************************************
//
// This function is called by the USB device stack whenever the device
// configuration changes.
//
//*****************************************************************************
static void
ConfigChangeHandler(void *pvInstance, unsigned int ulValue)
{
    tMSCInstance *psInst;
    const tUSBDMSCDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create the instance pointer.
    //
    psDevice = (const tUSBDMSCDevice *)pvInstance;

    //
    // Create the serial instance data.
    //
    psInst = psDevice->psPrivateData;

    //
    // Insure that DMA is disable whenever the configuration is set.
    //
    USBEndpointDMADisable(USB0_BASE, psInst->ucINEndpoint, USB_EP_DEV_IN);
    USBEndpointDMADisable(USB0_BASE, psInst->ucOUTEndpoint, USB_EP_DEV_OUT);

    //
    // If we have a control callback, let the client know we are open for
    // business.
    //
    if(psDevice->pfnEventCallback)
    {
        //
        // Pass the connected event to the client.
        //
        psDevice->pfnEventCallback(pvInstance, USB_EVENT_CONNECTED, 0, 0);
    }
}

//*****************************************************************************
//
//! This function should be called once for the mass storage class device to
//! initialized basic operation and prepare for enumeration.
//!
//! \param ulIndex is the index of the USB controller to initialize for
//! mass storage class device operation.
//! \param psDevice points to a structure containing parameters customizing
//! the operation of the mass storage device.
//!
//! In order for an application to initialize the USB device mass storage
//! class, it must first call this function with the a valid mass storage
//! device class structure in the \e psDevice parameter.  This allows this
//! function to initialize the USB controller and device code to be prepared to
//! enumerate and function as a USB mass storage device.
//!
//! This function returns a void pointer that must be passed in to all other
//! APIs used by the mass storage class.
//!
//! See the documentation on the tUSBDMSCDevice structure for more information
//! on how to properly fill the structure members.
//!
//! \return Returns 0 on failure or a non-zero void pointer on success.
//
//*****************************************************************************
void *
USBDMSCInit(unsigned int ulIndex, const tUSBDMSCDevice *psDevice)
{
    //
    // Check parameter validity.
    //
    ASSERT(ulIndex == 0);
    ASSERT(psDevice);
    ASSERT(psDevice->ppStringDescriptors);
    ASSERT(psDevice->psPrivateData);

    USBDMSCCompositeInit(ulIndex, psDevice);

    //
    // All is well so now pass the descriptors to the lower layer and put
    // the bulk device on the bus.
    //
    USBDCDInit(ulIndex, psDevice->psPrivateData->psDevInfo);

	//
	// Return the pointer to the instance indicating that everything went well.
	//
    return ((void *)psDevice);
}

//*****************************************************************************
//
//! This function should be called once for the mass storage class device to
//! initialized basic operation and prepare for enumeration.
//!
//! \param ulIndex is the index of the USB controller to initialize for
//! mass storage class device operation.
//! \param psDevice points to a structure containing parameters customizing
//! the operation of the mass storage device.
//!
//! In order for an application to initialize the USB device mass storage
//! class, it must first call this function with the a valid mass storage
//! device class structure in the \e psDevice parameter.  This allows this
//! function to initialize the USB controller and device code to be prepared to
//! enumerate and function as a USB mass storage device.
//!
//! This function returns a void pointer that must be passed in to all other
//! APIs used by the mass storage class.
//!
//! See the documentation on the tUSBDMSCDevice structure for more information
//! on how to properly fill the structure members.
//!
//! \return Returns 0 on failure or a non-zero void pointer on success.
//
//*****************************************************************************
void *
USBDMSCCompositeInit(unsigned int ulIndex, const tUSBDMSCDevice *psDevice)
{
    tMSCInstance *psInst;
    tDeviceDescriptor *psDevDesc;

    //
    // Check parameter validity.
    //
    ASSERT(ulIndex == 0);
    ASSERT(psDevice);
    ASSERT(psDevice->ppStringDescriptors);
    ASSERT(psDevice->psPrivateData);

    //
    // Initialize the workspace in the passed instance structure.
    //
    psInst = psDevice->psPrivateData;
    psInst->psConfDescriptor = (tConfigDescriptor *)g_pMSCDescriptor;
    psInst->psDevInfo = &g_sMSCDeviceInfo;
    psInst->ulUSBBase = USB0_BASE;
    psInst->bConnected = false;
    psInst->eMediaStatus = USBDMSC_MEDIA_UNKNOWN;

    //
    // Set the initial interface and endpoints.
    //
    psInst->ucInterface = 0;
    psInst->ucOUTEndpoint = DATA_OUT_ENDPOINT;
    psInst->ucOUTDMA = DATA_OUT_DMA_CHANNEL;
    psInst->ucINEndpoint = DATA_IN_ENDPOINT;
    psInst->ucINDMA = DATA_IN_DMA_CHANNEL;

    //
    // Set the initial SCSI state to idle.
    //
    psInst->ucSCSIState = STATE_SCSI_IDLE;

    //
    // Fix up the device descriptor with the client-supplied values.
    //
    psDevDesc = (tDeviceDescriptor *)psInst->psDevInfo->pDeviceDescriptor;
    psDevDesc->idVendor = psDevice->usVID;
    psDevDesc->idProduct = psDevice->usPID;

    //
    // Fix up the configuration descriptor with client-supplied values.
    //
    psInst->psConfDescriptor->bmAttributes = psDevice->ucPwrAttributes;
    psInst->psConfDescriptor->bMaxPower =
                        (unsigned char)(psDevice->usMaxPowermA / 2);

    //
    // Plug in the client's string stable to the device information
    // structure.
    //
    psInst->psDevInfo->ppStringDescriptors = psDevice->ppStringDescriptors;
    psInst->psDevInfo->ulNumStringDescriptors
        = psDevice->ulNumStringDescriptors;
    psInst->psDevInfo->pvInstance = (void *)psDevice;

    //
    // Open the drive requested.
    //
    psInst->pvMedia = psDevice->sMediaFunctions.Open(0);

    if(psInst->pvMedia == 0)
    {
        //
        // There is no media currently present.
        //
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;
    }
    else
    {
        //
        // Media is now ready for use.
        //
        psInst->ucSenseKey = SCSI_RS_KEY_UNIT_ATTN;
        psInst->usAddSenseCode = SCSI_RS_MED_NOTRDY2RDY;
    }


    //
    // Enable Clocking to the USB controller.
    //
	USBModuleClkEnable(ulIndex, USB0_BASE);


    //
    // Turn on USB Phy clock.
    //
    UsbPhyOn();

    //
    // Return the pointer to the instance indicating that everything went well.
    //
    return((void *)psDevice);
}

//*****************************************************************************
//
//! Shuts down the mass storage device.
//!
//! \param pvInstance is the pointer to the device instance structure as
//! returned by USBDMSCInit() or USBDMSCInitComposite().
//!
//! This function terminates mass storage operation for the instance supplied
//! and removes the device from the USB bus.  Following this call, the
//! \e psDevice instance may not me used in any other call to the mass storage
//! device other than USBDMSCInit() or USBDMSCInitComposite().
//!
//! \return None.
//
//*****************************************************************************
void
USBDMSCTerm(void *pvInstance)
{
    const tUSBDMSCDevice *psDevice;

    ASSERT(pvInstance != 0);

    //
    // Create a device instance pointer.
    //
    psDevice = pvInstance;

    //
    // If the media was opened the close it out.
    //
    if(psDevice->psPrivateData->pvMedia == 0)
    {
        psDevice->psPrivateData->pvMedia = 0;
        psDevice->sMediaFunctions.Close(0);
    }

    //
    // Cleanly exit device mode.
    //
    USBDCDTerm(0);
}

//*****************************************************************************
//
// This function is called by the USB device stack whenever a non-standard
// request is received.
//
// \param pvInstance is instance data for this request.
// \param pUSBRequest points to the request received.
//
// This call parses the provided request structure to determine the command.
// The only mass storage command supported over endpoint 0 is the Get Max LUN
// command.
//
// \return None.
//
//*****************************************************************************
static void
HandleRequests(void *pvInstance, tUSBRequest *pUSBRequest)
{
    //
    // This class only support a single LUN.
    //
    static const unsigned char ucMaxLun = 0;

    ASSERT(pvInstance != 0);

    //
    // Determine the type of request.
    //
    switch(pUSBRequest->bRequest)
    {
        //
        // A Set Report request is received from the host when it sends an
        // Output report via endpoint 0.
        //
        case USBREQ_GET_MAX_LUN:
        {
            //
            // Send our response to the host.
            //
            USBDCDSendDataEP0(0, (unsigned char *)&ucMaxLun, 1);

            break;
        }

        //
        // This request was not recognized so stall.
        //
        default:
        {
            USBDCDStallEP0(0);
            break;
        }
    }
}

//*****************************************************************************
//
// This function is used to handle the SCSI Inquiry command when it is received
// from the host.
//
//*****************************************************************************
static void
USBDSCSIInquiry(const tUSBDMSCDevice *psDevice)
{
    int iIdx;
    tMSCInstance *psInst;
#ifdef DMA_MODE
	unsigned char *cmdBuffer;
#endif
    //
    // Create the serial instance data.
    //
    psInst = psDevice->psPrivateData;

    //
    // Direct Access device, Removable storage and SCSI 1 responses.
    //
#ifdef _TMS320C6X
    _mem4(&g_pucCommand[0]) = SCSI_INQ_PDT_SBC | (SCSI_INQ_RMB << 8);
#else
    *(unsigned int *)&g_pucCommand[0] = SCSI_INQ_PDT_SBC | (SCSI_INQ_RMB << 8);
#endif

    //
    // Additional Length is fixed at 31 bytes.
    //
#ifdef _TMS320C6X
    _mem4(&g_pucCommand[4]) = 31;
#else
    *(unsigned int *)&g_pucCommand[4] = 31;
#endif

    //
    // Copy the Vendor string.
    //
    for(iIdx = 0; iIdx < 8; iIdx++)
    {
        g_pucCommand[iIdx + 8] = psDevice->pucVendor[iIdx];
    }

    //
    // Copy the Product string.
    //
    for(iIdx = 0; iIdx < 16; iIdx++)
    {
        g_pucCommand[iIdx + 16] = psDevice->pucProduct[iIdx];
    }

    //
    // Copy the Version string.
    //
    for(iIdx = 0; iIdx < 4; iIdx++)
    {
        g_pucCommand[iIdx + 32] = psDevice->pucVersion[iIdx];
    }



#ifdef DMA_MODE

	//
	//Allocate buffer for the command
	//
	cmdBuffer = (unsigned char*)cppiDmaAllocBuffer();
	memcpy(cmdBuffer, g_pucCommand, 36);

	//
	//send command response
	//
	doDmaTxTransfer(USB_INSTANCE, cmdBuffer, 36, psInst->ucINEndpoint);
	enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else

	//
	// Send the SCSI Inquiry Response.
	//
	USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint, g_pucCommand, 36);

	//
	// Send the data to the host.
	//
	USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);
#endif



    //
    // Set the status so that it can be sent when this response has
    // has be successfully sent.
    //
    g_sSCSICSW.bCSWStatus = 0;
    g_sSCSICSW.dCSWDataResidue = 0;

    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
}

//*****************************************************************************
//
// This function is used to handle the SCSI Read Capacities command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIReadCapacities(const tUSBDMSCDevice *psDevice)
{
    unsigned int ulBlocks;
    tMSCInstance *psInst;

#ifdef DMA_MODE
	unsigned char *cmdBuffer;
#endif

    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    if(psInst->pvMedia != 0)
    {
        ulBlocks = psDevice->sMediaFunctions.NumBlocks(psInst->pvMedia);

#ifdef _TMS320C6X
        _mem4(&g_pucCommand[0]) = 0x08000000;
#else
        *(unsigned int *)&g_pucCommand[0] = 0x08000000;
#endif

        //
        // Fill in the number of blocks, the bytes endianness must be changed.
        //
        g_pucCommand[4] = ulBlocks >> 24;
        g_pucCommand[5] = 0xff & (ulBlocks >> 16);
        g_pucCommand[6] = 0xff & (ulBlocks >> 8);
        g_pucCommand[8] = 0xff & (ulBlocks);

        //
        // Current media capacity
        //
        g_pucCommand[8] = 0x02;

        //
        // Fill in the block size, which is fixed at DEVICE_BLOCK_SIZE.
        //
        g_pucCommand[9] = 0xff & (DEVICE_BLOCK_SIZE >> 16);
        g_pucCommand[10] = 0xff & (DEVICE_BLOCK_SIZE >> 8);
        g_pucCommand[11] = 0xff & DEVICE_BLOCK_SIZE;

		//
		// Send out the 12 bytes that are in this response.
		//
#ifdef DMA_MODE

		//
		//Allocate buffer for the command
		//
		cmdBuffer = (unsigned char*)cppiDmaAllocBuffer();
		memcpy(cmdBuffer, g_pucCommand, 12);

		//
		//Send the command response
		//
		doDmaTxTransfer(USB_INSTANCE, cmdBuffer, 12, psInst->ucINEndpoint);
		enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else
		USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint, g_pucCommand, 12);
		USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);
#endif

		//
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 0;
        g_sSCSICSW.dCSWDataResidue = 0;
    }
    else
    {
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 1;
        g_sSCSICSW.dCSWDataResidue = 0;

        //
        // Stall the IN endpoint
        //
        USBDevEndpointStall(USB0_BASE, psInst->ucINEndpoint, USB_EP_DEV_IN);

        //
        // Mark the sense code as valid and indicate that these is no media
        // present.
        //
        psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;
    }

    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
}

//*****************************************************************************
//
// This function is used to handle the SCSI Read Capacity command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIReadCapacity(const tUSBDMSCDevice *psDevice)
{
    unsigned int ulBlocks;
    tMSCInstance *psInst;
#ifdef DMA_MODE
	unsigned char *cmdBuffer;
#endif
    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    ulBlocks = psDevice->sMediaFunctions.NumBlocks(psInst->pvMedia);

    //
    // Only decrement if any blocks were found.
    //
    if(ulBlocks != 0)
    {
        //
        // One less than the maximum number is the last addressable
        // block.
        //
        ulBlocks--;
    }

    if(psInst->pvMedia != 0)
    {
        //
        // Fill in the number of blocks, the bytes endianness must be changed.
        //
        g_pucCommand[0] = 0xff & (ulBlocks >> 24);
        g_pucCommand[1] = 0xff & (ulBlocks >> 16);
        g_pucCommand[2] = 0xff & (ulBlocks >> 8);
        g_pucCommand[3] = 0xff & (ulBlocks);

        g_pucCommand[4] = 0;

        //
        // Fill in the block size, which is fixed at DEVICE_BLOCK_SIZE.
        //
        g_pucCommand[5] = 0xff & (DEVICE_BLOCK_SIZE >> 16);
        g_pucCommand[6] = 0xff & (DEVICE_BLOCK_SIZE >> 8);
        g_pucCommand[7] = 0xff & DEVICE_BLOCK_SIZE;

        //
        // Send the SCSI Inquiry Response.
        //
#ifdef DMA_MODE

		//
		//Allocate buffer for the command
		//
		cmdBuffer = (unsigned char*)cppiDmaAllocBuffer();
		memcpy(cmdBuffer, g_pucCommand, 8);

		//
		//Send the command response
		//
		doDmaTxTransfer(USB_INSTANCE, cmdBuffer, 8, psInst->ucINEndpoint);
		enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else
		USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint, g_pucCommand, 8);
		USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);
#endif

        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 0;
        g_sSCSICSW.dCSWDataResidue = 0;
    }
    else
    {
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 1;
        g_sSCSICSW.dCSWDataResidue = 0;

        //
        // Stall the IN endpoint
        //
        USBDevEndpointStall(USB0_BASE, psInst->ucINEndpoint, USB_EP_DEV_IN);

        //
        // Mark the sense code as valid and indicate that these is no media
        // present.
        //
        psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;
    }

    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
}

//*****************************************************************************
//
// This function is used to handle the SCSI Request Sense command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIRequestSense(const tUSBDMSCDevice *psDevice)
{
    tMSCInstance *psInst;
#ifdef DMA_MODE
	unsigned char *cmdBuffer;
#endif

    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    //
    // The request sense response.
    //
    g_pucCommand[0] = psInst->ucErrorCode;
    g_pucCommand[1] = 0;
    g_pucCommand[2] = psInst->ucSenseKey;
	g_pucCommand[3] = 0;
	g_pucCommand[4] = 0;
	g_pucCommand[5] = 0;
	g_pucCommand[6] = 0;
	//*(unsigned long *)&g_pucCommand[3] = 0;

    //
    // There are 10 more bytes of data.
    //
    g_pucCommand[7] = 10;

   // *(unsigned long *)&g_pucCommand[8] = 0;

	g_pucCommand[8] = 0;
	g_pucCommand[9] = 0;
	g_pucCommand[10] = 0;
	g_pucCommand[11] = 0;
    //
    // Transition from not ready to ready.
    //
	g_pucCommand[12] = 0xff & (psInst->usAddSenseCode >> 8);
	g_pucCommand[12] = 0xff & (psInst->usAddSenseCode);
   // *(unsigned short *)&g_pucCommand[12] = psInst->usAddSenseCode;
  //  *(unsigned long *)&g_pucCommand[14] = 0;
	g_pucCommand[14] = 0;
	g_pucCommand[15] = 0;
	g_pucCommand[16] = 0;
	g_pucCommand[17] = 0;

    //
    // Send the SCSI Inquiry Response.
    //
#ifdef DMA_MODE

	//
	//Allocate buffer for the command
	//
	cmdBuffer = (unsigned char*)cppiDmaAllocBuffer();
	memcpy(cmdBuffer, g_pucCommand, 18);

	//
	//send the command response
	//
	doDmaTxTransfer(USB_INSTANCE, cmdBuffer, 18, psInst->ucINEndpoint);
	enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else
	USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint, g_pucCommand, 18);
	USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);
#endif
    //
    // Reset the valid flag on errors.
    //
    psInst->ucErrorCode = SCSI_RS_CUR_ERRORS;

    //
    // Set the status so that it can be sent when this response has
    // has be successfully sent.
    //
    g_sSCSICSW.bCSWStatus = 0;
    g_sSCSICSW.dCSWDataResidue = 0;

    //
    // Move on to the status phase.
    //
    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
}

//*****************************************************************************
//
// This function is used to handle the SCSI Read 10 command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIRead10(const tUSBDMSCDevice *psDevice, tMSCCBW *pSCSICBW)
{
    tMSCInstance *psInst;
	unsigned int usNumBlocks;

#ifdef DMA_MODE
	unsigned int txBuffer;
	unsigned int nBlocks;
#endif

	//
    // Default the number of blocks.
    //
    usNumBlocks = 0;

    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;


	if(psInst->pvMedia != 0)
    {
        //
        // Get the logical block from the CBW structure. This switching
        // is required to convert from big to little endian.
        //
        psInst->ulCurrentLBA = (pSCSICBW->CBWCB[2] << 24) |
                               (pSCSICBW->CBWCB[3] << 16) |
                               (pSCSICBW->CBWCB[4] << 8) |
                               (pSCSICBW->CBWCB[5] << 0);

		//
        // More bytes to read.
        //
        usNumBlocks = (pSCSICBW->CBWCB[7] << 8) | pSCSICBW->CBWCB[8];


#ifdef DMA_MODE
		if(usNumBlocks > (DMA_TX_MAX_CHUNK_SIZE / MAX_TRANSFER_SIZE))
			nBlocks = (DMA_TX_MAX_CHUNK_SIZE / MAX_TRANSFER_SIZE);
		else
			nBlocks = usNumBlocks;

		//
		//Allocate buffer for TX data
		//
		txBuffer=(unsigned int)cppiDmaAllocnBuffer(nBlocks);

		//
        // Read the next logical block from the storage device.
        //
		if(psDevice->sMediaFunctions.BlockRead(psInst->pvMedia,
               ((unsigned char *)txBuffer),
               psInst->ulCurrentLBA, nBlocks) == 0)
        {
            psInst->pvMedia = 0;
            psDevice->sMediaFunctions.Close(0);
	    }
#else

		//
        // Read the next logical block from the storage device.
        //
		if(psDevice->sMediaFunctions.BlockRead(psInst->pvMedia,
			((unsigned char *)psInst->pulBuffer),
			psInst->ulCurrentLBA, 1) == 0)
			{
				psInst->pvMedia = 0;
				psDevice->sMediaFunctions.Close(0);
			}


#endif
    }



	//
    // If there is media present then start transferring the data.
    //
    if(psInst->pvMedia != 0)
    {

        //
        // Schedule the remaining bytes to send.
        //
        psInst->ulBytesToTransfer = (DEVICE_BLOCK_SIZE * usNumBlocks);

#ifdef DMA_MODE

		//
		//Load the DMA queue with the data buffer
		//
		doDmaTxTransfer(USB_INSTANCE, (unsigned char *)txBuffer,
			(nBlocks *DEVICE_BLOCK_SIZE), psInst->ucINEndpoint);
		//
		//Enable the DMA for TX operation
		//
		enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else
		USBEndpointDataPut(USB0_BASE,  psInst->ucINEndpoint,
			(unsigned char *)psInst->pulBuffer,
				MAX_TRANSFER_SIZE);
		USBEndpointDataSend(USB0_BASE,	psInst->ucINEndpoint,
			USB_TRANS_IN);
#endif
		//
        // Move on and start sending blocks.
        //
        psInst->ucSCSIState = STATE_SCSI_SEND_BLOCKS;

        if(psDevice->pfnEventCallback)
        {
            psDevice->pfnEventCallback(0, USBD_MSC_EVENT_READING, 0, 0);
        }
    }
    else
    {
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 1;
        g_sSCSICSW.dCSWDataResidue = 0;

        //
        // Stall the IN endpoint
        //
        USBDevEndpointStall(USB0_BASE, psInst->ucINEndpoint, USB_EP_DEV_IN);

        //
        // Mark the sense code as valid and indicate that these is no media
        // present.
        //
        psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;

        psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
    }
}

//*****************************************************************************
//
// This function is used to handle the SCSI Read 10 command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIWrite10(const tUSBDMSCDevice *psDevice, tMSCCBW *pSCSICBW)
{
	unsigned short usNumBlocks;
	tMSCInstance *psInst;

	//
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    //
    // If there is media present then start transferring the data.
    //
    if(psInst->pvMedia != 0)
    {
        //
        // Get the logical block from the CBW structure. This switching
        // is required to convert from big to little endian.
        //
        psInst->ulCurrentLBA = (pSCSICBW->CBWCB[2] << 24) |
                         (pSCSICBW->CBWCB[3] << 16) |
                         (pSCSICBW->CBWCB[4] << 8) |
                         (pSCSICBW->CBWCB[5] << 0);
		//
        // More bytes to read.
        //
        usNumBlocks = (pSCSICBW->CBWCB[7] << 8) | pSCSICBW->CBWCB[8];

        psInst->ulBytesToTransfer = DEVICE_BLOCK_SIZE * usNumBlocks;

        //
        // Start sending logical blocks, these are always multiples of
        // DEVICE_BLOCK_SIZE bytes.
        //
        psInst->ucSCSIState = STATE_SCSI_RECEIVE_BLOCKS;

#ifdef DMA_MODE
		enableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
#endif

		//
        // Notify the application of the write event.
        //
        if(psDevice->pfnEventCallback)
        {
            psDevice->pfnEventCallback(0, USBD_MSC_EVENT_WRITING, 0, 0);
        }


    }
    else
    {
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 1;
        g_sSCSICSW.dCSWDataResidue = 0;

        //
        // Stall the IN endpoint
        //
        USBDevEndpointStall(USB0_BASE, psInst->ucOUTEndpoint, USB_EP_DEV_OUT);

        //
        // Mark the sense code as valid and indicate that these is no media
        // present.
        //
        psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;
        psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
    }
}

//*****************************************************************************
//
// This function is used to handle the SCSI Mode Sense 6 command when it is
// received from the host.
//
//*****************************************************************************
static void
USBDSCSIModeSense6(const tUSBDMSCDevice *psDevice, tMSCCBW *pSCSICBW)
{
    tMSCInstance *psInst;
#ifdef DMA_MODE
	unsigned char *cmdBuffer;
#endif
    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    //
    // If there is media present send the response.
    //
    if(psInst->pvMedia != 0)
    {
        //
        // Three extra bytes in this response.
        //
        g_pucCommand[0] = 3;
        g_pucCommand[1] = 0;
        g_pucCommand[2] = 0;
        g_pucCommand[3] = 0;

        //
        // Manually send the response back to the host.
        //
#ifdef DMA_MODE

		//
		//Allocate buffer for the command
		//
		cmdBuffer = (unsigned char*)cppiDmaAllocBuffer();
		memcpy(cmdBuffer, g_pucCommand, 4);

		//
		//Send the command response
		//
		doDmaTxTransfer(USB_INSTANCE, cmdBuffer, 4, psInst->ucINEndpoint);
		enableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
#else
		USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint, g_pucCommand, 4);
		USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);
#endif
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 0;
        g_sSCSICSW.dCSWDataResidue = pSCSICBW->dCBWDataTransferLength - 4;
    }
    else
    {
        //
        // Set the status so that it can be sent when this response has
        // has be successfully sent.
        //
        g_sSCSICSW.bCSWStatus = 1;
        g_sSCSICSW.dCSWDataResidue = 0;

        //
        // Stall the IN endpoint
        //
        USBDevEndpointStall(USB0_BASE, psInst->ucINEndpoint, USB_EP_DEV_IN);

        //
        // Mark the sense code as valid and indicate that these is no media
        // present.
        //
        psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
        psInst->ucSenseKey = SCSI_RS_KEY_NOT_READY;
        psInst->usAddSenseCode = SCSI_RS_MED_NOT_PRSNT;
    }

    psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
}

//*****************************************************************************
//
// This function is used to send out the response data based on the current
// status of the mass storage class.
//
//*****************************************************************************
static void
USBDSCSISendStatus(const tUSBDMSCDevice *psDevice)
{
    tMSCInstance *psInst;

    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    //
    // Respond with the requested status.
    //

	USBEndpointDataPut(USB0_BASE, psInst->ucINEndpoint,
                     (unsigned char *)&g_sSCSICSW, 13);
  	USBEndpointDataSend(USB0_BASE, psInst->ucINEndpoint, USB_TRANS_IN);

    //
    // Move the state to status sent so that the next interrupt will move the
    // statue to idle.
    //
    psInst->ucSCSIState = STATE_SCSI_SENT_STATUS;
}

//*****************************************************************************
//
// This function is used to handle all SCSI commands.
//
//*****************************************************************************
unsigned int
USBDSCSICommand(const tUSBDMSCDevice *psDevice, tMSCCBW *pSCSICBW)
{
    unsigned int ulRetCode;
    unsigned int ulTransferLength;
    tMSCInstance *psInst;

    //
    // Get our instance data pointer.
    //
    psInst = psDevice->psPrivateData;

    //
    // Initialize the return code.
    //
    ulRetCode = 1;

    //
    // Save the transfer length because it may be overwritten by some calls.
    //
    ulTransferLength = pSCSICBW->dCBWDataTransferLength;

    switch(pSCSICBW->CBWCB[0])
    {
        //
        // Respond to the SCSI Inquiry command.
        //
        case SCSI_INQUIRY_CMD:
        {
            USBDSCSIInquiry(psDevice);

            break;
        }

        //
        // Respond to the test unit ready command.
        //
        case SCSI_TEST_UNIT_READY:
        {
            g_sSCSICSW.dCSWDataResidue = 0;

            if(psInst->pvMedia != 0)
            {
                //
                // Set the status to success for now, this could be different
                // if there is no media present.
                //
                g_sSCSICSW.bCSWStatus = 0;
            }
            else
            {
                //
                // Since there was no media, check for media here.
                //
                psInst->pvMedia = psDevice->sMediaFunctions.Open(0);

                //
                // If it is still not present then fail this command.
                //
                if(psInst->pvMedia != 0)
                {
                    g_sSCSICSW.bCSWStatus = 0;
                }
                else
                {
                    g_sSCSICSW.bCSWStatus = 1;
                }
            }
            break;
        }

        //
        // Handle the Read Capacities command.
        //
        case SCSI_READ_CAPACITIES:
        {
            USBDSCSIReadCapacities(psDevice);

            break;
        }

        //
        // Handle the Read Capacity command.
        //
        case SCSI_READ_CAPACITY:
        {
            USBDSCSIReadCapacity(psDevice);

            break;
        }

        //
        // Handle the Request Sense command.
        //
        case SCSI_REQUEST_SENSE:
        {
            USBDSCSIRequestSense(psDevice);

            break;
        }

        //
        // Handle the Read 10 command.
        //
        case SCSI_READ_10:
        {
            USBDSCSIRead10(psDevice, pSCSICBW);

            break;
        }

        //
        // Handle the Write 10 command.
        //
        case SCSI_WRITE_10:
        {

			USBDSCSIWrite10(psDevice, pSCSICBW);

            break;
        }

        //
        // Handle the Mode Sense 6 command.
        //
        case SCSI_MODE_SENSE_6:
        {
            USBDSCSIModeSense6(psDevice, pSCSICBW);

            break;
        }

		case SCSI_VERIFY_10:
		{
			psInst->ucSCSIState = STATE_SCSI_IDLE;
            g_sSCSICSW.bCSWStatus = 0;
            g_sSCSICSW.dCSWDataResidue = pSCSICBW->dCBWDataTransferLength;
            break;
		}

        default:
        {
			psInst->ucSCSIState = STATE_SCSI_IDLE;

			//
            // Set the status so that it can be sent when this response has
            // has be successfully sent.
            //
            g_sSCSICSW.bCSWStatus = 1;
            g_sSCSICSW.dCSWDataResidue = pSCSICBW->dCBWDataTransferLength;

            //
            // If there is data then there is more work to do.
            //
            if(pSCSICBW->dCBWDataTransferLength != 0)
            {

                if(pSCSICBW->bmCBWFlags & CBWFLAGS_DIR_IN)
                {
                    //
                    // Stall the IN endpoint
                    //
                    USBDevEndpointStall(USB0_BASE, psInst->ucINEndpoint,
                                        USB_EP_DEV_IN);
                }
                else
                {
                    //
                    // Stall the OUT endpoint
                    //
                    USBDevEndpointStall(USB0_BASE, psInst->ucOUTEndpoint,
                                        USB_EP_DEV_OUT);

                }
                //
                // Send the status once the stall occurs.
                //
                psInst->ucSCSIState = STATE_SCSI_SEND_STATUS;
            }

            //
            // Set the sense codes.
            //
            psInst->ucErrorCode = SCSI_RS_VALID | SCSI_RS_CUR_ERRORS;
            psInst->ucSenseKey = SCSI_RS_KEY_ILGL_RQST;
            psInst->usAddSenseCode = SCSI_RS_PV_INVALID;

            break;
        }
    }

    //
    // If there is no data then send out the current status.
    //
    if(ulTransferLength == 0)
    {

		#ifdef DMA_MODE
			disableCoreTxDMA(USB_INSTANCE, psInst->ucINEndpoint);
			disableCoreRxDMA(USB_INSTANCE, psInst->ucOUTEndpoint);
		#endif

		USBDSCSISendStatus(psDevice);
    }
    return(ulRetCode);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
