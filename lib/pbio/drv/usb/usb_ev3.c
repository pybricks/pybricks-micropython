// *****************************************************************************
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
// This is part of AM1808 Sitaraware firmware package, modified and reused from revision 6288
// of the DK-LM3S9B96 Firmware Package.
//
// *****************************************************************************

// EV3 USB serial driver.
//
// Primarily based on usb_dev_serial.c and usb_serial_structs.c from TI's USB
// library as above. USB hooks from starterware/platform/evmAM1808/usb.c
// Additional inspiration by liyixiao from EV3RT.
//
// Pybricks modifications are licensed as follows:
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <stdbool.h>

#include <contiki.h>

#include <pbdrv/usb.h>
#include <pbio/util.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_usbOtg_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_usbphyGS60.h>
#include <tiam1808/hw/hw_usb.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_psc_AM1808.h>
#include <tiam1808/psc.h>

#include <pbdrv/clock.h>

// Revisit: USB init currently has a few blocking waits. Need to split up to
// async functions.
void delay(uint32_t ms) {
    uint32_t start = pbdrv_clock_get_ms();
    while (pbdrv_clock_get_ms() - start < ms) {
        /* Wait */
    }
}

unsigned int USBVersionGet(void) {
    return USB_REV_AM1808;
}

void USBEnableInt(unsigned int ulBase) {
    HWREG(ulBase + USB_0_INTR_MASK_SET) = 0x01FF1E1F;
}

void USBClearInt(unsigned int ulBase) {
    HWREG(ulBase + USB_0_INTR_SRC_CLEAR) = HWREG(ulBase + USB_0_INTR_SRC);
}

void USBModuleClkEnable(unsigned int ulIndex, unsigned int ulBase) {
    PSCModuleControl(SOC_PSC_1_REGS, 1, 0, PSC_MDCTL_NEXT_ENABLE);
}

void USBModuleClkDisable(unsigned int ulIndex, unsigned int ulBase) {
    PSCModuleControl(SOC_PSC_1_REGS, 1, 0, PSC_MDCTL_NEXT_DISABLE);
}

PROCESS(pbdrv_usb_process, "USB");


// #include <tiam1808/usb.h>
#include <tiam1808/usblib/usblib.h>
#include <tiam1808/usblib/usbcdc.h>
#include <tiam1808/usblib/usb-ids.h>
#include <tiam1808/usblib/usbdevice.h>
#include <tiam1808/usblib/usbdcdc.h>

// The languages supported by this device.
const unsigned char g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

// The manufacturer string.
const unsigned char g_pManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

// The product string.
const unsigned char g_pProductString[] =
{
    2 + (16 * 2),
    USB_DTYPE_STRING,
    'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0,
    'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0
};

// The serial number string.
const unsigned char g_pSerialNumberString[] =
{
    2 + (8 * 2),
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

// The control interface description string.
const unsigned char g_pControlInterfaceString[] =
{
    2 + (21 * 2),
    USB_DTYPE_STRING,
    'A', 0, 'C', 0, 'M', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
    'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

// The configuration description string.
const unsigned char g_pConfigString[] =
{
    2 + (26 * 2),
    USB_DTYPE_STRING,
    'S', 0, 'e', 0, 'l', 0, 'f', 0, ' ', 0, 'P', 0, 'o', 0, 'w', 0,
    'e', 0, 'r', 0, 'e', 0, 'd', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0,
    'o', 0, 'n', 0
};

// The descriptor string table.
const unsigned char *const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString,
    g_pControlInterfaceString,
    g_pConfigString
};

extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;

// Global flag indicating that a USB configuration has been set.
static volatile bool g_bUSBConfigured = false;

static void GetLineCoding(tLineCoding *psLineCoding) {
    //
    // Get the current line coding set in the UART.
    //
    psLineCoding->ulRate = 115200;
    psLineCoding->ucDatabits = 8;
    psLineCoding->ucParity = USB_CDC_PARITY_NONE;
    psLineCoding->ucStop = USB_CDC_STOP_BITS_1;

}

// *****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ulCBData is the client-supplied callback data value for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
// *****************************************************************************
unsigned int RxHandler(void *pvCBData, unsigned int ulEvent, unsigned int ulMsgValue, void *pvMsgData) {

    //
    // Which event are we being sent?
    //
    switch (ulEvent)
    {
        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE: {
            // Hack to get CTRL+C to work. Needs to be moved to pbsys.
            char *test = pvMsgData;
            extern bool pbsys_main_stdin_event(uint8_t c);
            for (unsigned int i = 0; i < ulMsgValue; i++) {
                pbsys_main_stdin_event(test[i]);
            }
            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process. We return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something. The actual number of
        // bytes in the UART FIFO is not important here, merely whether or
        // not everything previously sent to us has been transmitted.
        //
        case USB_EVENT_DATA_REMAINING: {
            return 0;
        }

        //
        // We are being asked to provide a buffer into which the next packet
        // can be read. We do not support this mode of receiving data so let
        // the driver know by returning 0. The CDC driver should not be sending
        // this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER: {
            return 0;
        }

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
            break;
    }

    return 0;
}

// *****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ulCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
// *****************************************************************************
static unsigned int TxHandler(void *pvCBData, unsigned int ulEvent, unsigned int ulMsgValue, void *pvMsgData) {
    //
    // Which event have we been sent?
    //
    switch (ulEvent)
    {
        case USB_EVENT_TX_COMPLETE:
            //
            // Since we are using the USBBuffer, we don't need to do anything here.
            //
            break;

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
            break;
    }
    return 0;
}

// *****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
// *****************************************************************************
unsigned int ControlHandler(void *pvCBData, unsigned int ulEvent, unsigned int ulMsgValue, void *pvMsgData) {
    //
    // Which event are we being asked to process?
    //
    switch (ulEvent)
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:
            g_bUSBConfigured = true;

            //
            // Flush our buffers.
            //
            USBBufferFlush(&g_sTxBuffer);
            USBBufferFlush(&g_sRxBuffer);
            process_poll(&pbdrv_usb_process);
            break;

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
            g_bUSBConfigured = false;
            process_poll(&pbdrv_usb_process);
            break;

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
            GetLineCoding(pvMsgData);
            break;

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_LINE_CODING:
            break;

        //
        // Set the current serial communication parameters.
        //
        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
            //
            // TODO: If configured with GPIOs controlling the handshake lines,
            // set them appropriately depending upon the flags passed in the wValue
            // field of the request structure passed.
            //
            break;

        //
        // Send a break condition on the serial line.
        //
        case USBD_CDC_EVENT_SEND_BREAK:
            break;

        //
        // Clear the break condition on the serial line.
        //
        case USBD_CDC_EVENT_CLEAR_BREAK:
            break;

        //
        // Ignore SUSPEND and RESUME for now.
        //
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
            break;

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
            break;
    }

    return 0;
}
// *****************************************************************************
//
// The CDC device initialization and customization structures. In this case,
// we are using USBBuffers between the CDC device class driver and the
// application code. The function pointers and callback data values are set
// to insert a buffer in each of the data channels, transmit and receive.
//
// With the buffer in place, the CDC channel callback is set to the relevant
// channel function and the callback data is set to point to the channel
// instance data. The buffer, in turn, has its callback set to the application
// function and the callback data set to our CDC instance structure.
//
// *****************************************************************************
tCDCSerInstance g_sCDCInstance;

const tUSBDCDCDevice g_sCDCDevice =
{
    USB_VID_STELLARIS,
    USB_PID_SERIAL,
    0,
    USB_CONF_ATTR_SELF_PWR,
    ControlHandler,
    (void *)&g_sCDCDevice,
    USBBufferEventCallback,
    (void *)&g_sRxBuffer,
    USBBufferEventCallback,
    (void *)&g_sTxBuffer,
    g_pStringDescriptors,
    PBIO_ARRAY_SIZE(g_pStringDescriptors),
    &g_sCDCInstance
};

#define USB_CDC_BUFFER_SIZE (2048)

// Receive buffer (from the USB perspective).
unsigned char g_pcUSBRxBuffer[USB_CDC_BUFFER_SIZE] __attribute__ ((aligned(16)));
unsigned char g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE]  __attribute__ ((aligned(16)));
const tUSBBuffer g_sRxBuffer =
{
    false,                          // This is a receive buffer.
    RxHandler,                // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketRead,              // pfnTransfer
    USBDCDCRxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pcUSBRxBuffer,                // pcBuffer
    USB_CDC_BUFFER_SIZE,               // ulBufferSize
    g_pucRxBufferWorkspace          // pvWorkspace
};

// Transmit buffer (from the USB perspective).
unsigned char g_pcUSBTxBuffer[USB_CDC_BUFFER_SIZE] __attribute__ ((aligned(16)));
unsigned char g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE] __attribute__ ((aligned(16)));
const tUSBBuffer g_sTxBuffer =
{
    true,                           // This is a transmit buffer.
    TxHandler,                      // pfnCallback
    (void *)&g_sCDCDevice,          // Callback data is our device pointer.
    USBDCDCPacketWrite,             // pfnTransfer
    USBDCDCTxPacketAvailable,       // pfnAvailable
    (void *)&g_sCDCDevice,          // pvHandle
    g_pcUSBTxBuffer,                // pcBuffer
    USB_CDC_BUFFER_SIZE,            // ulBufferSize
    g_pucTxBufferWorkspace          // pvWorkspace // is this rhe ringbuff??
};

void pbdrv_usb_init(void) {

    // Enable the clocks to the USB and PHY modules. Inspired by EV3RT.
    HWREG(CFGCHIP2_USBPHYCTRL) &= ~SYSCFG_CFGCHIP2_USB0OTGMODE;
    HWREG(CFGCHIP2_USBPHYCTRL) |= CFGCHIP2_FORCE_DEVICE;  // Force USB device operation
    HWREG(CFGCHIP2_USBPHYCTRL) |= CFGCHIP2_REFFREQ_24MHZ; // 24 MHz OSCIN

    process_start(&pbdrv_usb_process);

    /*
    ** Registers the UARTIsr in the Interrupt Vector Table of AINTC.
    ** The event number of UART2 interrupt is 61.
    */

    IntRegister(SYS_INT_USB0, USB0DeviceIntHandler);

    /*
    ** Map the channel number 2 of AINTC to system interrupt 61.
    ** Channel number 2 of AINTC is mapped to IRQ interrupt of ARM9 processor.
    */
    IntChannelSet(SYS_INT_USB0, 2);

    /* Enable the system interrupt number 61 in AINTC.*/
    IntSystemEnable(SYS_INT_USB0);

    //
    // Not configured initially.
    //
    g_bUSBConfigured = false;

    //
    // Initialize the Rx and TX Buffers
    //
    USBBufferInit((tUSBBuffer *)&g_sTxBuffer);
    USBBufferInit((tUSBBuffer *)&g_sRxBuffer);

    USBBufferFlush(&g_sTxBuffer);
    USBBufferFlush(&g_sRxBuffer);

    g_sCDCSerDeviceInfo.sCallbacks.pfnSuspendHandler = g_sCDCSerDeviceInfo.sCallbacks.pfnDisconnectHandler;
    //
    // Pass our device information to the USB library and place the device
    // on the bus.
    //
    USBDCDCInit(0, (tUSBDCDCDevice *)&g_sCDCDevice);
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return g_bUSBConfigured ? PBDRV_USB_BCD_STANDARD_DOWNSTREAM : PBDRV_USB_BCD_NONE;
}

uint32_t pbdrv_usb_write(const uint8_t *data, uint32_t size) {
    return USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, data, size);
}

uint32_t pbdrv_usb_rx_data_available(void) {
    return USBBufferDataAvailable((tUSBBuffer *)&g_sRxBuffer);
}

int32_t pbdrv_usb_get_char(void) {
    uint8_t c;
    if (USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &c, 1) > 0) {
        return c;
    }
    return -1;
}

void pbdrv_usb_tx_flush(void) {
    USBBufferFlush((tUSBBuffer *)&g_sTxBuffer);
}

PROCESS_THREAD(pbdrv_usb_process, ev, data) {

    PROCESS_BEGIN();

    // TODO: Async init USB, pausing pbdrv/core.

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
        // Can handle connect/disconnect events here.
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_EV3
