//*****************************************************************************
//
// usbhscsi.c - USB host SCSI layer used by the USB host MSC driver.
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

#include "hw_types.h"
#include "usblib.h"
#include "usbmsc.h"
#include "usbhost.h"
#include "usbhmsc.h"
#include "usbhscsi.h"

//*****************************************************************************
//
//! \addtogroup usblib_host_class
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is the data verify tag passed between requests.
//
//*****************************************************************************
#define CBW_TAG_VALUE           0x54231990

//*****************************************************************************
//
//! This function is used to issue SCSI commands via USB.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param pSCSICmd is the SCSI command structure to send.
//! \param pucData is pointer to the command data to be sent.
//! \param pulSize is the number of bytes is the number of bytes expected or
//! sent by the command.
//!
//! This internal function is used to handle SCSI commands sent by other
//! functions.  It serves as a layer between the SCSI command and the USB
//! interface being used to send the command.  The \e pSCSI parameter contains
//! the SCSI command to send.  For commands that expect data back, the
//! \e pucData is the buffer to store the data into and \e pulSize is used to
//! store the amount of data to request as well as used to indicate how many
//! bytes were filled into the \e pucData buffer on return.  For commands that
//! are sending data, \e pucData is the data to be sent and \e pulSize is the
//! number of bytes to send.
//!
//! \return This function returns the SCSI status from the command.  The value
//! will be either \b SCSI_CMD_STATUS_PASS or \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
static unsigned int
USBHSCSISendCommand(unsigned int ulInPipe, unsigned int ulOutPipe,
                    tMSCCBW *pSCSICmd, unsigned char *pucData,
                    unsigned int *pulSize)
{
    tMSCCSW CmdStatus;
    unsigned int ulBytes;

    //
    // Initialize the command status.
    //
    CmdStatus.dCSWSignature = 0;
    CmdStatus.dCSWTag = 0;
    CmdStatus.bCSWStatus = SCSI_CMD_STATUS_FAIL;

    //
    // Set the CBW signature and tag.
    //
    pSCSICmd->dCBWSignature = CBW_SIGNATURE;
    pSCSICmd->dCBWTag = CBW_TAG_VALUE;

    //
    // Set the size of the data to be returned by the device.
    //
    pSCSICmd->dCBWDataTransferLength = *pulSize;

    //
    // Send the command.
    //
    ulBytes = USBHCDPipeWrite(ulOutPipe,
                              (unsigned char*)pSCSICmd, sizeof(tMSCCBW));

    //
    // If no bytes went out then the command failed.
    //
    if(ulBytes == 0)
    {
        return(SCSI_CMD_STATUS_FAIL);
    }

    //
    // Only request data if there is data to request.
    //
    if(pSCSICmd->dCBWDataTransferLength != 0)
    {
        //
        // See if this is a read or a write.
        //
        if(pSCSICmd->bmCBWFlags & CBWFLAGS_DIR_IN)
        {
            //
            // Read the data back.
            //
            *pulSize = USBHCDPipeRead(ulInPipe, pucData, *pulSize);
        }
        else
        {
            //
            // Write the data out.
            //
            *pulSize = USBHCDPipeWrite(ulOutPipe, pucData, *pulSize);
        }
    }

    //
    // Get the status of the command.
    //
    ulBytes = USBHCDPipeRead(ulInPipe, (unsigned char *)&CmdStatus,
                             sizeof(tMSCCSW));


    //
    // If the status was invalid or did not have the correct signature then
    // indicate a failure.
    //
    if((ulBytes == 0) || (CmdStatus.dCSWSignature != CSW_SIGNATURE) ||
       (CmdStatus.dCSWTag != CBW_TAG_VALUE))
    {
        return(SCSI_CMD_STATUS_FAIL);
    }

    //
    // Return the status.
    //
    return((unsigned int)CmdStatus.bCSWStatus);
}

//*****************************************************************************
//
//! This will issue the SCSI inquiry command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param pucData is the data buffer to return the results into.
//! \param pulSize is the size of buffer that was passed in on entry and the
//! number of bytes returned.
//!
//! This function should be used to issue a SCSI Inquiry command to a mass
//! storage device.  To allow for multiple devices, the \e ulInPipe and
//! \e ulOutPipe parameters indicate which USB pipes to use for this call.
//!
//! \note The \e pucData buffer pointer should have at least
//! \b SCSI_INQUIRY_DATA_SZ bytes of data or this function will overflow the
//! buffer.
//!
//! \return This function returns the SCSI status from the command.  The value
//! will be either \b SCSI_CMD_STATUS_PASS or \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIInquiry(unsigned int ulInPipe, unsigned int ulOutPipe,
                unsigned char *pucData, unsigned int *pulSize)
{
    tMSCCBW SCSICmd;

    //
    // The number of bytes of data that the host expects to transfer on the
    // Bulk-In or Bulk-Out endpoint (as indicated by the Direction bit) during
    // the execution of this command.  If this field is zero, the device and
    // the host shall transfer no data between the CBW and the associated CSW,
    // and the device shall ignore the value of the Direction bit in
    // bmCBWFlags.
    //
    *pulSize = SCSI_INQUIRY_DATA_SZ;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // This is the length of the command itself.
    //
    SCSICmd.bCBWCBLength = 6;

#if defined (__IAR_SYSTEMS_ICC__)     
    //
    // Send Inquiry command with no request for vital product data.
    //
    SCSICmd.CBWCB[0] = SCSI_INQUIRY_CMD;
    SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    //
    // Allocation length.
    //
    SCSICmd.CBWCB[4] = SCSI_INQUIRY_DATA_SZ;
    SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
	SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
	//
	// Send Inquiry command with no request for vital product data.
	//
	_mem4(&SCSICmd.CBWCB[0]) = SCSI_INQUIRY_CMD;

	//
	// Allocation length.
	//
	_mem4(&SCSICmd.CBWCB[4]) = SCSI_INQUIRY_DATA_SZ;
	_mem4(&SCSICmd.CBWCB[8]) = 0;
	_mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	//
	// Send Inquiry command with no request for vital product data.
	//
	*((unsigned int *)&SCSICmd.CBWCB[0]) = SCSI_INQUIRY_CMD;

	//
	// Allocation length.
	//
	*((unsigned int *)&SCSICmd.CBWCB[4]) = SCSI_INQUIRY_DATA_SZ;
	*((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif  




    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This will issue the SCSI read capacity command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param pucData is the data buffer to return the results into.
//! \param pulSize is the size of buffer that was passed in on entry and the
//! number of bytes returned.
//!
//! This function should be used to issue a SCSI Read Capacity command
//! to a mass storage device that is connected.  To allow for multiple devices,
//! the \e ulInPipe and \e ulOutPipe parameters indicate which USB pipes to
//! use for this call.
//!
//! \note The \e pucData buffer pointer should have at least
//! \b SCSI_READ_CAPACITY_SZ bytes of data or this function will overflow the
//! buffer.
//!
//! \return This function returns the SCSI status from the command.  The value
//! will be either \b SCSI_CMD_STATUS_PASS or \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIReadCapacity(unsigned int ulInPipe, unsigned int ulOutPipe,
                     unsigned char *pucData, unsigned int *pulSize)
{
    tMSCCBW SCSICmd;

    //
    // Set the size of the command data.
    //
    *pulSize = SCSI_READ_CAPACITY_SZ;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the length of the command itself.
    //
    SCSICmd.bCBWCBLength = 12;   
    
	//
	// Only use the first byte and set it to the Read Capacity command.  The
	// rest are set to 0.	//
	 
#if defined (__IAR_SYSTEMS_ICC__)  
    SCSICmd.CBWCB[0] = SCSI_READ_CAPACITY;
    SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    SCSICmd.CBWCB[4] = 0;
	SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
    SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
    _mem4(&SCSICmd.CBWCB[0]) = SCSI_READ_CAPACITY;
    _mem4(&SCSICmd.CBWCB[4]) = 0;
    _mem4(&SCSICmd.CBWCB[8]) = 0;
    _mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	*((unsigned int *)&SCSICmd.CBWCB[0]) = SCSI_READ_CAPACITY;
	*((unsigned int *)&SCSICmd.CBWCB[4]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif

    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This will issue the SCSI read capacities command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param pucData is the data buffer to return the results into.
//! \param pulSize is the size of buffer that was passed in on entry and the
//! number of bytes returned.
//!
//! This function should be used to issue a SCSI Read Capacities command
//! to a mass storage device that is connected.  To allow for multiple devices,
//! the \e ulInPipe and \e ulOutPipe parameters indicate which USB pipes to
//! use for this call.
//!
//! \return This function returns the SCSI status from the command.  The value
//! will be either \b SCSI_CMD_STATUS_PASS or \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIReadCapacities(unsigned int ulInPipe, unsigned int ulOutPipe,
                       unsigned char *pucData, unsigned int *pulSize)
{
    tMSCCBW SCSICmd;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the length of the command itself.
    //
    SCSICmd.bCBWCBLength = 12;
           
    //
    // Only use the first byte and set it to the Read Capacity command.  The
    // rest are set to 0.
    //
#if defined (__IAR_SYSTEMS_ICC__)  
    SCSICmd.CBWCB[0] = SCSI_READ_CAPACITIES;
    SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    SCSICmd.CBWCB[4] = 0;
	SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
    SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
    _mem4(&SCSICmd.CBWCB[0]) = SCSI_READ_CAPACITIES;
    _mem4(&SCSICmd.CBWCB[4]) = 0;
    _mem4(&SCSICmd.CBWCB[8]) = 0;
    _mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	*((unsigned int *)&SCSICmd.CBWCB[0]) = SCSI_READ_CAPACITIES;
	*((unsigned int *)&SCSICmd.CBWCB[4]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif

    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This will issue the SCSI Mode Sense(6) command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param ulFlags is a combination of flags defining the exact query that is
//! to be made.
//! \param pucData is the data buffer to return the results into.
//! \param pulSize is the size of the buffer on entry and number of bytes read
//! on exit.
//!
//! This function should be used to issue a SCSI Mode Sense(6) command
//! to a mass storage device.  To allow for multiple devices,the \e ulInPipe
//! and \e ulOutPipe parameters indicate which USB pipes to use for this call.
//! The call will return at most the number of bytes in the \e pulSize
//! parameter, however it can return less and change the \e pulSize parameter
//! to the number of valid bytes in the \e *pulSize buffer.
//!
//! The \e ulFlags parameter is a combination of the following three sets of
//! definitions:
//!
//! One of the following values must be specified:
//!
//! - \b SCSI_MS_PC_CURRENT request for current settings.
//! - \b SCSI_MS_PC_CHANGEABLE request for changeable settings.
//! - \b SCSI_MS_PC_DEFAULT request for default settings.
//! - \b SCSI_MS_PC_SAVED request for the saved values.
//!
//! One of these following values must also be specified to determine the page
//! code for the request:
//!
//! - \b SCSI_MS_PC_VENDOR is the vendor specific page code.
//! - \b SCSI_MS_PC_DISCO is the disconnect/reconnect page code.
//! - \b SCSI_MS_PC_CONTROL is the control page code.
//! - \b SCSI_MS_PC_LUN is the protocol specific LUN page code.
//! - \b SCSI_MS_PC_PORT is the protocol specific port page code.
//! - \b SCSI_MS_PC_POWER is the power condition page code.
//! - \b SCSI_MS_PC_INFORM is the informational exceptions page code.
//! - \b SCSI_MS_PC_ALL will request all pages codes supported by the device.
//!
//! The last value is optional and supports the following global flag:
//! - \b SCSI_MS_DBD disables returning block descriptors.
//!
//! Example: Request for all current settings.
//!
//! \verbatim
//! SCSIModeSense6(ulInPipe, ulOutPipe,
//!                SCSI_MS_PC_CURRENT | SCSI_MS_PC_ALL,
//!                pucData, pulSize);
//! \endverbatim
//!
//! \return This function returns the SCSI status from the command.  The value
//! will be either \b SCSI_CMD_STATUS_PASS or \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIModeSense6(unsigned int ulInPipe, unsigned int ulOutPipe,
                   unsigned int ulFlags, unsigned char *pucData,
                   unsigned int *pulSize)
{
    tMSCCBW SCSICmd;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the size of the command data.
    //
    SCSICmd.bCBWCBLength = 6;

    //
    // Set the options for the Mode Sense Command (6).
    //    
#if defined (__IAR_SYSTEMS_ICC__)   
    SCSICmd.CBWCB[0] = (SCSI_MODE_SENSE_6 | ulFlags);
	SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    SCSICmd.CBWCB[4] = (unsigned char)*pulSize;
	SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
    SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
    _mem4(&SCSICmd.CBWCB[0]) = (SCSI_MODE_SENSE_6 | ulFlags);
    _mem4(&SCSICmd.CBWCB[4]) = (unsigned char)*pulSize;
    _mem4(&SCSICmd.CBWCB[8]) = 0;
    _mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	*((unsigned int *)&SCSICmd.CBWCB[0]) = (SCSI_MODE_SENSE_6 | ulFlags);
	*((unsigned int *)&SCSICmd.CBWCB[4]) = (unsigned char)*pulSize;
	*((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif
    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This function issues a SCSI Test Unit Ready command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//!
//! This function is used to issue a SCSI Test Unit Ready command to a device.
//! This call will simply return the results of issuing this command.
//!
//! \return This function returns the results of the SCSI Test Unit Ready
//! command.  The value will be either \b SCSI_CMD_STATUS_PASS or
//! \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSITestUnitReady(unsigned int ulInPipe, unsigned int ulOutPipe)
{
    tMSCCBW SCSICmd;
    unsigned int ulSize;

    //
    // No data in this command.
    //
    ulSize = 0;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the size of the command data.
    //
    SCSICmd.bCBWCBLength = 6;

    //
    // Set the parameter options.
    //
#if defined (__IAR_SYSTEMS_ICC__) 
    SCSICmd.CBWCB[0] = SCSI_TEST_UNIT_READY;
    SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    SCSICmd.CBWCB[4] = 0;
	SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
    SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
    _mem4(&SCSICmd.CBWCB[0]) = SCSI_TEST_UNIT_READY;
    _mem4(&SCSICmd.CBWCB[4]) = 0;
    _mem4(&SCSICmd.CBWCB[8]) = 0;
    _mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	*((unsigned int *)&SCSICmd.CBWCB[0]) = SCSI_TEST_UNIT_READY;
    *((unsigned int *)&SCSICmd.CBWCB[4]) = 0;
    *((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
    *((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif

    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, 0, &ulSize));
}

//*****************************************************************************
//
//! This function issues a SCSI Request Sense command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param pucData is the data buffer to return the results into.
//! \param pulSize is the size of the buffer on entry and number of bytes read
//! on exit.
//!
//! This function is used to issue a SCSI Request Sense command to a device.
//! It will return the data in the buffer pointed to by \e pucData.  The
//! parameter \e pulSize should have the allocation size in bytes of the buffer
//! pointed to by pucData.
//!
//! \return This function returns the results of the SCSI Request Sense
//! command.  The value will be either \b SCSI_CMD_STATUS_PASS or
//! \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIRequestSense(unsigned int ulInPipe, unsigned int ulOutPipe,
                     unsigned char *pucData, unsigned int *pulSize)
{
    tMSCCBW SCSICmd;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the size of the command data.
    //
    SCSICmd.bCBWCBLength = 12;

    //
    // Set the parameter options.
    //
#if defined (__IAR_SYSTEMS_ICC__) 
	SCSICmd.CBWCB[0] = SCSI_REQUEST_SENSE;    
    SCSICmd.CBWCB[1] = 0;
    SCSICmd.CBWCB[2] = 0;
    SCSICmd.CBWCB[3] = 0;  
    SCSICmd.CBWCB[4] = 18;
	SCSICmd.CBWCB[5] = 0;
	SCSICmd.CBWCB[6] = 0;
	SCSICmd.CBWCB[7] = 0;
    SCSICmd.CBWCB[8] = 0;
    SCSICmd.CBWCB[9] = 0;
    SCSICmd.CBWCB[10] = 0;
    SCSICmd.CBWCB[11] = 0;
    SCSICmd.CBWCB[12] = 0;
    SCSICmd.CBWCB[13] = 0;
    SCSICmd.CBWCB[14] = 0;
    SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
    _mem4(&SCSICmd.CBWCB[0]) = SCSI_REQUEST_SENSE;
    _mem4(&SCSICmd.CBWCB[4]) = 18;
    _mem4(&SCSICmd.CBWCB[8]) = 0;
    _mem4(&SCSICmd.CBWCB[12]) = 0;
#else
	*((unsigned int *)&SCSICmd.CBWCB[0]) = SCSI_REQUEST_SENSE;
	*((unsigned int *)&SCSICmd.CBWCB[4]) = 18;
	*((unsigned int *)&SCSICmd.CBWCB[8]) = 0;
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;

#endif
    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This function issues a SCSI Read(10) command to a device.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param ulLBA is the logical block address to read.
//! \param pucData is the data buffer to return the data.
//! \param pulSize is the size of the buffer on entry and number of bytes read
//! on exit.
//! \param ulNumBlocks is the number of contiguous blocks to read from the
//! device.
//!
//! This function is used to issue a SCSI Read(10) command to a device.  The
//! \e ulLBA parameter specifies the logical block address to read from the
//! device.  The data from this block will be returned in the buffer pointed to
//! by \e pucData.  The parameter \e pulSize should indicate enough space to
//! hold a full block size, or only the first pulSize bytes of the LBA will
//! be returned.
//!
//! \return This function returns the results of the SCSI Read(10) command.
//! The value will be either \b SCSI_CMD_STATUS_PASS or
//! \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIRead10(unsigned int ulInPipe, unsigned int ulOutPipe,
               unsigned int ulLBA, unsigned char *pucData,
               unsigned int *pulSize, unsigned int ulNumBlocks)
{
    tMSCCBW SCSICmd;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_IN;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the size of the command data.
    //
    SCSICmd.bCBWCBLength = 10;

    //
    // Set the parameter options.
    //
    SCSICmd.CBWCB[0] = SCSI_READ_10;

    //
    // Clear the reserved field.
    //
    SCSICmd.CBWCB[1] = 0;

    //
    // LBA starts at offset 2.
    //
    SCSICmd.CBWCB[2] = (unsigned char)(ulLBA >> 24);
    SCSICmd.CBWCB[3] = (unsigned char)(ulLBA >> 16);
    SCSICmd.CBWCB[4] = (unsigned char)(ulLBA >> 8);
    SCSICmd.CBWCB[5] = (unsigned char)ulLBA;

    //
    // Clear the reserved field.
    //
    SCSICmd.CBWCB[6] = 0;

    //
    // Transfer length in blocks starts at offset 2.
    // This also sets the Control value to 0 at offset 9.
    //
    SCSICmd.CBWCB[7] = (ulNumBlocks & 0xFF00) >> 8;

#if defined (__IAR_SYSTEMS_ICC__)
	SCSICmd.CBWCB[8] = (ulNumBlocks & 0xFF);
	SCSICmd.CBWCB[9] = 0;
	SCSICmd.CBWCB[10]= 0;
	SCSICmd.CBWCB[11]= 0;
	SCSICmd.CBWCB[12] = 0;
	SCSICmd.CBWCB[13] = 0;
	SCSICmd.CBWCB[14] = 0;
	SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
	_mem4(&SCSICmd.CBWCB[8]) = (ulNumBlocks & 0xFF);
	_mem4(&SCSICmd.CBWCB[12]) = 0;
#else	
	*((unsigned int *)&SCSICmd.CBWCB[8]) = (ulNumBlocks & 0xFF);
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif    

    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
//! This function issues a SCSI Write(10) command to a device.
//!
//! This function is used to issue a SCSI Write(10) command to a device.  The
//! \e ulLBA parameter specifies the logical block address on the device.  The
//! data to write to this block should be in the buffer pointed to by
//! \e pucData parameter.  The parameter \e pulSize should indicate the amount
//! of data to write to the specified LBA.
//!
//! \param ulInPipe is the USB IN pipe to use for this command.
//! \param ulOutPipe is the USB OUT pipe to use for this command.
//! \param ulLBA is the logical block address to read.
//! \param pucData is the data buffer to write out.
//! \param pulSize is the size of the buffer.
//! \param ulNumBlocks is the number of contiguous blocks to write to the
//! device.
//!
//! \return This function returns the results of the SCSI Write(10) command.
//! The value will be either \b SCSI_CMD_STATUS_PASS or
//! \b SCSI_CMD_STATUS_FAIL.
//
//*****************************************************************************
unsigned int
USBHSCSIWrite10(unsigned int ulInPipe, unsigned int ulOutPipe,
                unsigned int ulLBA, unsigned char *pucData,
                unsigned int *pulSize, unsigned int ulNumBlocks)
{
    tMSCCBW SCSICmd;

    //
    // This is an IN request.
    //
    SCSICmd.bmCBWFlags = CBWFLAGS_DIR_OUT;

    //
    // Only handle LUN 0.
    //
    SCSICmd.bCBWLUN = 0;

    //
    // Set the size of the command data.
    //
    SCSICmd.bCBWCBLength = 10;

    //
    // Set the parameter options.
    //
    SCSICmd.CBWCB[0] = SCSI_WRITE_10;

    //
    // Clear the reserved field.
    //
    SCSICmd.CBWCB[1] = 0;

    //
    // LBA starts at offset 2.
    //
    SCSICmd.CBWCB[2] = (unsigned char)(ulLBA >> 24);
    SCSICmd.CBWCB[3] = (unsigned char)(ulLBA >> 16);
    SCSICmd.CBWCB[4] = (unsigned char)(ulLBA >> 8);
    SCSICmd.CBWCB[5] = (unsigned char)ulLBA;

    //
    // Clear the reserved field.
    //
    SCSICmd.CBWCB[6] = 0;

    //
    // Set the transfer length in blocks.
    // This also sets the Control value to 0 at offset 9.
    //
    SCSICmd.CBWCB[7] = (ulNumBlocks & 0xFF00) >> 8;

#if defined (__IAR_SYSTEMS_ICC__)    
	SCSICmd.CBWCB[8] = (ulNumBlocks & 0xFF);
	SCSICmd.CBWCB[9] = 0;
	SCSICmd.CBWCB[10]= 0;
	SCSICmd.CBWCB[11]= 0;
	SCSICmd.CBWCB[12] = 0;
	SCSICmd.CBWCB[13] = 0;
	SCSICmd.CBWCB[14] = 0;
	SCSICmd.CBWCB[15] = 0;
#elif defined (_TMS320C6X)
	_mem4(&SCSICmd.CBWCB[8]) = ulNumBlocks & 0xFF;
	_mem4(&SCSICmd.CBWCB[12]) = 0;
#else	
	*((unsigned int *)&SCSICmd.CBWCB[8]) = (ulNumBlocks & 0xFF);
	*((unsigned int *)&SCSICmd.CBWCB[12]) = 0;
#endif   
    //
    // Send the command and get the results.
    //
    return(USBHSCSISendCommand(ulInPipe, ulOutPipe, &SCSICmd, pucData,
                               pulSize));
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
