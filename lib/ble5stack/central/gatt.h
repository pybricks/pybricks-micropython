/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef GATT_H
#define GATT_H

#include "att.h"
#include "hci_tl.h"

/*
 * WARNING: The 16-bit UUIDs are assigned by the Bluetooth SIG and published
 *          in the Bluetooth Assigned Numbers page. Do not change these values.
 *          Changing them will cause Bluetooth interoperability issues.
 */

/**
 * GATT Services
 */
#define GAP_SERVICE_UUID                           0x1800 // Generic Access Profile
#define GATT_SERVICE_UUID                          0x1801 // Generic Attribute Profile

/**
 * GATT Declarations
 */
#define GATT_PRIMARY_SERVICE_UUID                  0x2800 // Primary Service
#define GATT_SECONDARY_SERVICE_UUID                0x2801 // Secondary Service
#define GATT_INCLUDE_UUID                          0x2802 // Include
#define GATT_CHARACTER_UUID                        0x2803 // Characteristic

/**
 * GATT Descriptors
 */
#define GATT_CHAR_EXT_PROPS_UUID                   0x2900 // Characteristic Extended Properties
#define GATT_CHAR_USER_DESC_UUID                   0x2901 // Characteristic User Description
#define GATT_CLIENT_CHAR_CFG_UUID                  0x2902 // Client Characteristic Configuration
#define GATT_SERV_CHAR_CFG_UUID                    0x2903 // Server Characteristic Configuration
#define GATT_CHAR_FORMAT_UUID                      0x2904 // Characteristic Presentation Format
#define GATT_CHAR_AGG_FORMAT_UUID                  0x2905 // Characteristic Aggregate Format
#define GATT_VALID_RANGE_UUID                      0x2906 // Valid Range
#define GATT_EXT_REPORT_REF_UUID                   0x2907 // External Report Reference Descriptor
#define GATT_REPORT_REF_UUID                       0x2908 // Report Reference Descriptor

/**
 * GATT Characteristics
 */
#define DEVICE_NAME_UUID                           0x2A00 // Device Name
#define APPEARANCE_UUID                            0x2A01 // Appearance
#define PERI_PRIVACY_FLAG_UUID                     0x2A02 // Peripheral Privacy Flag
#define RECONNECT_ADDR_UUID                        0x2A03 // Reconnection Address
#define PERI_CONN_PARAM_UUID                       0x2A04 // Peripheral Preferred Connection Parameters
#define SERVICE_CHANGED_UUID                       0x2A05 // Service Changed

/**
 * GATT Vendor Specific APIs - GATT Command Opcodes
 */
#define GATT_EXCHANGEMTU                0xFD82
#define GATT_DISCALLPRIMARYSERVICES     0xFD90
#define GATT_DISCPRIMARYSERVICEBYUUID   0xFD86
#define GATT_FINDINCLUDEDSERVICES       0xFDB0
#define GATT_DISCALLCHARS               0xFDB2
#define GATT_DISCCHARSBYUUID            0xFD88
#define GATT_DISCALLCHARDESCS           0xFD84
#define GATT_READCHARVALUE              0xFD8A
#define GATT_READUSINGCHARUUID          0xFDB4
#define GATT_READLONGCHARVALUE          0xFD8C
#define GATT_READMULTILCHARVALUES       0xFD8E
#define GATT_WRITENORSP                 0xFDB6
#define GATT_SIGNEDWRITENORSP           0xFDB8
#define GATT_WRITECHARVALUE             0xFD92
#define GATT_WRITELONGCHARVALUE         0xFD96
#define GATT_READCHARDESC               0xFDBC
#define GATT_READLONGCHARDESC           0xFDBE
#define GATT_ADDSERVICE                 0xFDFC
#define GATT_WRITECHARDESC              0xFDC0
#define GATT_WRITELONGCHARDESC          0xFDC2
#define GATT_NOTIFICATION               0xFD9B
#define GATT_INDICATION                 0xFD9D
#define GATT_ADDSERVICE                 0xFDFC
#define GATT_DELSERVICE                 0xFDFD
#define GATT_ADDATTRIBUTE               0xFDFE
#define GATT_UPDATEMTU                  0xFDFF

/**
 * GATT Vendor Specific APIs - GATT Event Opcodes
 */
#define GATT_CLIENTCHARCFGUPDATED       0x0580

/*********************************************************************
 * CONSTANTS
 */

/**  @addtogroup ATT_GATT_Constants
 *   @{
 *   @defgroup GATT_Permits GATT Attribute Access Permissions Bit Fields
 *   @{
 */
#define GATT_PERMIT_READ                0x01   //!< Attribute is Readable
#define GATT_PERMIT_WRITE               0x02   //!< Attribute is Writable
#define GATT_PERMIT_AUTHEN_READ         0x04   //!< Read requires Authentication
#define GATT_PERMIT_AUTHEN_WRITE        0x08   //!< Write requires Authentication
#define GATT_PERMIT_AUTHOR_READ         0x10   //!< Read requires Authorization
#define GATT_PERMIT_AUTHOR_WRITE        0x20   //!< Write requires Authorization
#define GATT_PERMIT_ENCRYPT_READ        0x40   //!< Read requires Encryption
#define GATT_PERMIT_ENCRYPT_WRITE       0x80   //!< Write requires Encryption
/** @} End GATT_Permits */

#if !defined( GATT_MAX_NUM_PREPARE_WRITES )
  #define GATT_MAX_NUM_PREPARE_WRITES   5 //!< GATT Maximum number of attributes that Attribute Server can prepare for writing per Attribute Client
#endif

/**
 * @defgroup GATT_Key_Sizes GATT Encryption Key Size Limits
 * @{
 */
#define GATT_MIN_ENCRYPT_KEY_SIZE       7  //!< GATT Minimum Encryption Key Size
#define GATT_MAX_ENCRYPT_KEY_SIZE       16 //!< GATT Maximum Encryption Key Size
/** @} End GATT_Key_Sizes */

#define GATT_MAX_ATTR_SIZE              512 //!< GATT Maximum length of an attribute value

#define GATT_BASE_METHOD                0x40  //!< GATT Base Method

#define GATT_INVALID_HANDLE             0x0000 //!< Invalid attribute handle
#define GATT_MIN_HANDLE                 0x0001 //!< Minimum attribute handle
#define GATT_MAX_HANDLE                 0xFFFF //!< Maximum attribute handle

#define GATT_ATTR_HANDLE_SIZE           0x02 //!< Number of octets attribute handle

#define GATT_MAX_MTU                    0xFFFF //!< Maximum MTU size
/** @} */ // end of ATT_GATT_Constants

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * MACROS
 */

/// @cond NODOC

// Attribute Access Permissions
#define gattPermitRead( a )              ( (a) & GATT_PERMIT_READ )
#define gattPermitWrite( a )             ( (a) & GATT_PERMIT_WRITE )
#define gattPermitEncryptRead( a )       ( (a) & GATT_PERMIT_ENCRYPT_READ )
#define gattPermitEncryptWrite( a )      ( (a) & GATT_PERMIT_ENCRYPT_WRITE )
#define gattPermitAuthorRead( a )        ( (a) & GATT_PERMIT_AUTHOR_READ )
#define gattPermitAuthorWrite( a )       ( (a) & GATT_PERMIT_AUTHOR_WRITE )
#define gattPermitAuthenRead( a )        ( (a) & GATT_PERMIT_AUTHEN_READ )
#define gattPermitAuthenWrite( a )       ( (a) & GATT_PERMIT_AUTHEN_WRITE )

/// @endcond // NODOC

/*********************************************************************
 * TYPEDEFS
 */

/** @} */ // end of ATT_GATT

/**
 *  @addtogroup ATT_GATT_Events
 *  @{
 */

/// @brief @brief GATT Find By Type Value Request format.
typedef struct
{
  uint16_t startHandle;  //!< First requested handle number (must be first field)
  uint16_t endHandle;    //!< Last requested handle number
  attAttrType_t value; //!< Primary service UUID value (2 or 16 octets)
} gattFindByTypeValueReq_t;

/// @brief GATT Read By Type Request format.
typedef struct
{
  uint8_t discCharsByUUID;  //!< Whether this is a GATT Discover Characteristics by UUID sub-procedure
  attReadByTypeReq_t req; //!< Read By Type Request
} gattReadByTypeReq_t;

/// @brief GATT Write Long Request format. Do not change the order of the members.
typedef struct
{
  uint8_t reliable;           //!< Whether reliable writes requested (always FALSE for Write Long)
  attPrepareWriteReq_t req; //!< ATT Prepare Write Request
  uint16_t lastOffset;        //!< Offset of last Prepare Write Request sent
} gattWriteLongReq_t;

/// @brief GATT Reliable Writes Request format. Do not change the order of the members.
typedef struct
{
  uint8_t reliable;              //!< Whether reliable writes requested (always TRUE for Reliable Writes)
  attPrepareWriteReq_t *pReqs; //!< Array of Prepare Write Requests (must be allocated)
  uint8_t numReqs;               //!< Number of Prepare Write Requests
  uint8_t index;                 //!< Index of last Prepare Write Request sent
  uint8_t flags;                 //!< 0x00 - cancel all prepared writes.
                               //!< 0x01 - immediately write all pending prepared values.
} gattReliableWritesReq_t;


/**
 * @brief GATT Message format.
 *
 * This is aunion of all attribute protocol/profile messages
 * and locally-generated events used between the attribute protocol/profile and
 * upper layer application.
 */
typedef union
{
  // Request messages
  attExchangeMTUReq_t exchangeMTUReq;              //!< ATT Exchange MTU Request
  attFindInfoReq_t findInfoReq;                    //!< ATT Find Information Request
  attFindByTypeValueReq_t findByTypeValueReq;      //!< ATT Find By Type Value Request
  attReadByTypeReq_t readByTypeReq;                //!< ATT Read By Type Request
  attReadReq_t readReq;                            //!< ATT Read Request
  attReadBlobReq_t readBlobReq;                    //!< ATT Read Blob Request
  attReadMultiReq_t readMultiReq;                  //!< ATT Read Multiple Request
  attReadByGrpTypeReq_t readByGrpTypeReq;          //!< ATT Read By Group Type Request
  attWriteReq_t writeReq;                          //!< ATT Write Request
  attPrepareWriteReq_t prepareWriteReq;            //!< ATT Prepare Write Request
  attExecuteWriteReq_t executeWriteReq;            //!< ATT Execute Write Request
  gattFindByTypeValueReq_t gattFindByTypeValueReq; //!< GATT Find By Type Value Request
  gattReadByTypeReq_t gattReadByTypeReq;           //!< GATT Read By Type Request
  gattWriteLongReq_t gattWriteLongReq;             //!< GATT Long Write Request
  gattReliableWritesReq_t gattReliableWritesReq;   //!< GATT Reliable Writes Request

  // Response messages
  attErrorRsp_t errorRsp;                          //!< ATT Error Response
  attExchangeMTURsp_t exchangeMTURsp;              //!< ATT Exchange MTU Response
  attFindInfoRsp_t findInfoRsp;                    //!< ATT Find Information Response
  attFindByTypeValueRsp_t findByTypeValueRsp;      //!< ATT Find By Type Value Response
  attReadByTypeRsp_t readByTypeRsp;                //!< ATT Read By Type Response
  attReadRsp_t readRsp;                            //!< ATT Read Response
  attReadBlobRsp_t readBlobRsp;                    //!< ATT Read Blob Response
  attReadMultiRsp_t readMultiRsp;                  //!< ATT Read Multiple Response
  attReadByGrpTypeRsp_t readByGrpTypeRsp;          //!< ATT Read By Group Type Response
  attPrepareWriteRsp_t prepareWriteRsp;            //!< ATT Prepare Write Response

  // Indication and Notification messages
  attHandleValueNoti_t handleValueNoti;            //!< ATT Handle Value Notification
  attHandleValueInd_t handleValueInd;              //!< ATT Handle Value Indication

  // Locally-generated event messages
  attFlowCtrlViolatedEvt_t flowCtrlEvt;            //!< ATT Flow Control Violated Event
  attMtuUpdatedEvt_t mtuEvt;                       //!< ATT MTU Updated Event
} gattMsg_t;

/**
 * @brief GATT @ref GATT_MSG_EVENT message format.
 *
 * This message is used to forward an
 * incoming attribute protocol/profile message up to upper layer application.
 */
typedef struct
{
  eventHeader_t  hdr;     //!< @ref GAP_MSG_EVENT and status
  uint16_t connHandle;    //!< Connection message was received on
  uint8_t method;         //!< Type of message
  gattMsg_t msg;          //!< Attribute protocol/profile message
} gattMsgEvent_t;

/** @} */ // end of ATT_GATT_events

/**
 *  @addtogroup ATT_GATT_Structs
 *  @{
 */

/// @brief GATT Attribute Type format.
typedef struct
{
  uint8_t len;         //!< Length of UUID (2 or 6)
  const uint8_t *uuid; //!< Pointer to UUID
} gattAttrType_t;

/**
 * @brief GATT Attribute format.
 *
 * @note
 * The list must start with a Service attribute followed by
 * all attributes associated with this Service attribute.
 */
typedef struct attAttribute_t
{
  gattAttrType_t type; //!< Attribute type (2 or 16 octet UUIDs)
  uint8_t permissions;    //!< Attribute permissions
  uint16_t handle;       //!< Attribute handle - assigned internally by attribute server
  uint8_t* const pValue; //!< Attribute value - encoding of the octet array is defined in
                       //!< the applicable profile. The maximum length of an attribute
                       //!< value shall be 512 octets.
} gattAttribute_t;

/// @brief GATT Service format.
typedef struct
{
  uint16_t numAttrs; //!< Number of attributes in attrs
  uint8_t encKeySize;//!< Minimum encryption key size required by service (7-16 bytes)
  gattAttribute_t *attrs; //!< Array of attribute records;
} gattService_t;

typedef struct{
    uint16_t connHandle; //!< Connection handle of the connected device
    uint16_t handle;     //!< Handle of the
    void *value;
    uint8_t dataSize;
} GattWriteCharValue_t;

typedef struct{
    uint16_t connHandle;
    uint16_t value;
} GattExchangeMTU_t;

/** @} */ // end of ATT_GATT_Structs

/*********************************************************************
 * VARIABLES
 */

/// @cond NODOC
extern uint8_t gattNumConns;
/// @endcond // NODOC

/**
 *  @addtogroup ATT_GATT
 *  @{
 */



/*-------------------------------------------------------------------
 * GATT Client Sub-Procedure APIs
 */

/**
 * @brief   Exchange MTU Request
 *
 * This sub-procedure is used by the client to set the ATT_MTU
 * to the maximum possible value that can be supported by both
 * devices when the client supports a value greater than the
 * default ATT_MTU for the Attribute Protocol. This sub-procedure
 * shall only be initiated once during a connection.
 *
 * @ref ATT_ExchangeMTUReq is used by this sub-procedure.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_EXCHANGE_MTU_RSP of type @ref attExchangeMTURsp_t , with
 * status @ref bleSUCCESS or @ref bleTimeout , if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , with
 * status @ref bleSUCCESS  , if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ExchangeMTU( uint16_t connHandle, attExchangeMTUReq_t *pReq);

/**
 * @brief  Discovery All Primary Services
 *
 * This sub-procedure is used by a client to discover all
 * the primary services on a server.
 *
 * @ref ATT_ReadByGrpTypeReq is used with the Attribute
 * Type parameter set to the UUID for "Primary Service". The
 * Starting Handle is set to 0x0001 and the Ending Handle is
 * set to 0xFFFF.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BY_GRP_TYPE_RSP of type @ref attReadByGrpTypeRsp_t ,
 * if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server

 * @note This sub-procedure is complete when either @ref ATT_READ_BY_GRP_TYPE_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or
 * @ref ATT_ERROR_RSP (with @ref bleSUCCESS status) is received by the calling task
 *
 * @param   connHandle - connection to use
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_DiscAllPrimaryServices( uint16_t connHandle);

/**
 * @brief   Discovery Primary Service by UUID
 *
 * This sub-procedure is used by a client to discover a specific
 * primary service on a server when only the Service UUID is
 * known. The primary specific service may exist multiple times
 * on a server. The primary service being discovered is identified
 * by the service UUID.
 *
 * @ref ATT_FindByTypeValueReq is used with the Attribute
 * Type parameter set to the UUID for "Primary Service" and the
 * Attribute Value set to the 16-bit Bluetooth UUID or 128-bit
 * UUID for the specific primary service. The Starting Handle shall
 * be set to 0x0001 and the Ending Handle shall be set to 0xFFFF.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_FIND_BY_TYPE_VALUE_RSP of type @ref attFindByTypeValueRsp_t ,
 * if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server
 *
 * @note This sub-procedure is complete when either @ref ATT_FIND_BY_TYPE_VALUE_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pUUID - pointer to service UUID to look for
 * @param   len - length of value
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_DiscPrimaryServiceByUUID( uint16_t connHandle, uint8_t *pUUID,
                                                uint8_t len);
/**
 * @brief   This sub-procedure is used by a client to find include
 * service declarations within a service definition on a
 * server. The service specified is identified by the service
 * handle range.
 *
 * @ref ATT_ReadByTypeReq is used with the Attribute
 * Type parameter set to the UUID for "Included Service". The
 * Starting Handle is set to starting handle of the specified
 * service and the Ending Handle is set to the ending handle
 * of the specified service.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BY_TYPE_RSP of type @ref attReadByTypeRsp_t ,if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BY_TYPE_RSP
 *       (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 *       (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   startHandle - starting handle
 * @param   endHandle - end handle
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_FindIncludedServices( uint16_t connHandle, uint16_t startHandle,
                                            uint16_t endHandle);
/**
 * @brief   Discover all Characteristics
 *
 * This sub-procedure is used by a client to find all the
 * characteristic declarations within a service definition on
 * a server when only the service handle range is known. The
 * service specified is identified by the service handle range.
 *
 * @ref ATT_ReadByTypeReq is used with the Attribute Type
 * parameter set to the UUID for "Characteristic". The Starting
 * Handle is set to starting handle of the specified service and
 * the Ending Handle is set to the ending handle of the specified
 * service.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BY_TYPE_RSP of type @ref attReadByTypeRsp_t , if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BY_TYPE_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   startHandle - starting handle
 * @param   endHandle - end handle
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_DiscAllChars( uint16_t connHandle, uint16_t startHandle,
                                    uint16_t endHandle);

/**
 * @brief   Discovery Characteristics by UUID
 *
 * This sub-procedure is used by a client to discover service
 * characteristics on a server when only the service handle
 * ranges are known and the characteristic UUID is known.
 * The specific service may exist multiple times on a server.
 * The characteristic being discovered is identified by the
 * characteristic UUID.
 *
 * @ref ATT_ReadByTypeReq is used with the Attribute Type
 * is set to the UUID for "Characteristic" and the Starting
 * Handle and Ending Handle parameters is set to the service
 * handle range.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BY_TYPE_RSP of type @ref attReadByTypeRsp_t , if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BY_TYPE_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_DiscCharsByUUID( uint16_t connHandle, attReadByTypeReq_t *pReq);

/**
 * @brief   Discovery All Characteristic Descriptors
 *
 * This sub-procedure is used by a client to find all the
 * characteristic descriptor's Attribute Handles and Attribute
 * Types within a characteristic definition when only the
 * characteristic handle range is known. The characteristic
 * specified is identified by the characteristic handle range.
 *
 * @ref ATT_FindInfoReq is used with the Starting
 * Handle set to starting handle of the specified characteristic
 * and the Ending Handle set to the ending handle of the specified
 * characteristic. The UUID Filter parameter is NULL (zero length).
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_FIND_INFO_RSP of type @ref attFindInfoRsp_t , if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_FIND_INFO_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   startHandle - starting handle
 * @param   endHandle - end handle
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_DiscAllCharDescs( uint16_t connHandle, uint16_t startHandle,
                                        uint16_t endHandle);

/**
 * @brief   Read Characteristic Value
 *
 * This sub-procedure is used to read a Characteristic Value
 * from a server when the client knows the Characteristic Value
 * Handle.
 *
 * @ref ATT_ReadReq is used with the Attribute Handle
 * parameter set to the Characteristic Value Handle. The Read
 * Response returns the Characteristic Value in the Attribute
 * Value parameter.
 *
 * @note: The Read Response only contains a Characteristic Value that
 * is less than or equal to (ATT_MTU - 1) octets in length. If
 * the Characteristic Value is greater than (ATT_MTU - 1) octets
 * in length, the Read Long Characteristic Value procedure may
 * be used if the rest of the Characteristic Value is required.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_READ_RSP of type @ref attReadRsp_t , if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t , if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ReadCharValue( uint16_t connHandle, attReadReq_t *pReq);

/**
 * @brief   Read Using Characteristic UUID
 *
 * This sub-procedure is used to read a Characteristic Value
 * from a server when the client only knows the characteristic
 * UUID and does not know the handle of the characteristic.
 *
 * @ref ATT_ReadByTypeReq is used to perform the sub-procedure.
 * The Attribute Type is set to the known characteristic UUID and
 * the Starting Handle and Ending Handle parameters shall be set
 * to the range over which this read is to be performed. This is
 * typically the handle range for the service in which the
 * characteristic belongs.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BY_TYPE_RSP of type @ref attReadByTypeRsp_t ,if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BY_TYPE_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ReadUsingCharUUID( uint16_t connHandle, attReadByTypeReq_t *pReq);
/**
 * @brief   Read Long Characteristic Value
 *
 * This sub-procedure is used to read a Characteristic Value from
 * a server when the client knows the Characteristic Value Handle
 * and the length of the Characteristic Value is longer than can
 * be sent in a single Read Response Attribute Protocol message.
 *
 * @ref ATT_ReadBlobReq is used in this sub-procedure.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive multiple @ref GATT_MSG_EVENT messages with method:
 * - @ref ATT_READ_BLOB_RSP of type @ref attReadBlobRsp_t ,if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BLOB_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */

extern HCI_StatusCodes_t GATT_ReadLongCharValue( uint16_t connHandle, attReadBlobReq_t *pReq);

/**
 * @brief   Read Multiple Characteristic Values
 *
 * This sub-procedure is used to read multiple Characteristic Values
 * from a server when the client knows the Characteristic Value
 * Handles. The Attribute Protocol Read Multiple Requests is used
 * with the Set Of Handles parameter set to the Characteristic Value
 * Handles. The Read Multiple Response returns the Characteristic
 * Values in the Set Of Values parameter.
 *
 * @ref ATT_ReadMultiReq is used in this sub-procedure.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_READ_MULTI_RSP of type @ref attReadMultiRsp_t ,if the procedure was successful
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_MULTI_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ReadMultiCharValues( uint16_t connHandle, attReadMultiReq_t *pReq);

/**
 * @brief   Write No Response
 *
 * This sub-procedure is used to write a Characteristic Value
 * to a server when the client knows the Characteristic Value
 * Handle and the client does not need an acknowledgement that
 * the write was successfully performed. This sub-procedure
 * only writes the first (ATT_MTU - 3) octets of a Characteristic
 * Value. This sub-procedure can not be used to write a long
 * characteristic; instead the Write Long Characteristic Values
 * sub-procedure should be used.
 *
 * @ref ATT_WriteReq is used for this sub-procedure. The
 * Attribute Handle parameter shall be set to the Characteristic
 * Value Handle. The Attribute Value parameter shall be set to
 * the new Characteristic Value.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note No response will be sent to the calling task for this
 * sub-procedure. If the Characteristic Value write request is the
 * wrong size, or has an invalid value as defined by the profile,
 * then the write will not succeed and no error will be generated
 * by the server.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to command to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_WriteNoRsp( uint16_t connHandle, attWriteReq_t *pReq );

/**
 * @brief   Signed Write No Response
 *
 * This sub-procedure is used to write a Characteristic Value
 *  to a server when the client knows the Characteristic Value
 *  Handle and the ATT Bearer is not encrypted. This sub-procedure
 *  shall only be used if the Characteristic Properties authenticated
 *  bit is enabled and the client and server device share a bond as
 *  defined in the GAP.
 *
 *  This sub-procedure only writes the first (ATT_MTU - 15) octets
 *  of an Attribute Value. This sub-procedure cannot be used to
 *  write a long Attribute.
 *
 *  @ref ATT_WriteReq is used for this sub-procedure. The
 *  Attribute Handle parameter shall be set to the Characteristic
 *  Value Handle. The Attribute Value parameter shall be set to
 *  the new Characteristic Value authenticated by signing the
 *  value, as defined in the Security Manager.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref bleMemAllocError
 *  @ref bleLinkEncrypted: Connection is already encrypted.
 *  @ref bleTimeout : Previous transaction timed out.
 *
 *  @note No response will be sent to the calling task for this
 *  sub-procedure. If the authenticated Characteristic Value that is
 *  written is the wrong size, or has an invalid value as defined by
 *  the profile, or the signed value does not authenticate the client,
 *  then the write will not succeed and no error will be generated by
 *  the server.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to command to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_SignedWriteNoRsp( uint16_t connHandle, attWriteReq_t *pReq );

/**
 * @brief   Write Characteristic Value
 *
 * This sub-procedure is used to write a characteristic value
 * to a server when the client knows the characteristic value
 * handle. This sub-procedure only writes the first (ATT_MTU-3)
 * octets of a characteristic value. This sub-procedure can not
 * be used to write a long attribute; instead the Write Long
 * Characteristic Values sub-procedure should be used.
 *
 * @ref ATT_WriteReq is used in this sub-procedure. The
 * Attribute Handle parameter shall be set to the Characteristic
 * Value Handle. The Attribute Value parameter shall be set to
 * the new characteristic.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_WRITE_RSP if the procedure is successfull
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_WRITE_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   para @ref GattWriteCharValue_t struct containing write request info
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_WriteCharValue(GattWriteCharValue_t *para);
/**
 * @brief   Write Long Characteristic Value
 *
 * This sub-procedure is used to write a Characteristic Value to
 * a server when the client knows the Characteristic Value Handle
 * but the length of the Characteristic Value is longer than can
 * be sent in a single Write Request Attribute Protocol message.
 *
 * @ref ATT_PrepareWriteReq and @ref ATT_ExecuteWriteReq are
 * used to perform this sub-procedure.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_WRITE_RSP and @ref ATT_EXECUTE_WRITE_RSP if the procedure is successfull
 * - @ref ATT_EXECUTE_WRITE_RSP of type
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_PREPARE_WRITE_RSP
 * (with @ref bleTimeout status), @ref ATT_EXECUTE_WRITE_RSP (with @ref bleSUCCESS
 * or @ref bleTimeout status), or @ref ATT_ERROR_RSP (with @ref bleSUCCESS status)
 * is received by the calling task.
 *
 * @warning The 'pReq->pValue' pointer will be freed when the sub-procedure is complete.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_WriteLongCharValue( uint16_t connHandle, attPrepareWriteReq_t *pReq);

/**
 * @brief   Read Characteristic Descriptor
 *
 * This sub-procedure is used to read a characteristic descriptor
 * from a server when the client knows the characteristic descriptor
 * declaration's Attribute handle.
 *
 * @ref ATT_ReadReq is used for this sub-procedure. with the Attribute Handle
 * parameter set to the characteristic descriptor handle.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_READ_RSP of type attReadRsp_t if the procedure is successfull
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ReadCharDesc( uint16_t connHandle, attReadReq_t *pReq);

/**
 * @brief   Read Long Characteristic Descriptor
 *
 * This sub-procedure is used to read a characteristic descriptor
 * from a server when the client knows the characteristic descriptor
 * declaration's Attribute handle and the length of the characteristic
 * descriptor declaration is longer than can be sent in a single Read
 * Response attribute protocol message.
 *
 *  @ref ATT_ReadBlobReq is used to perform this sub-procedure.
 *  The Attribute Handle parameter shall be set to the characteristic
 *  descriptor handle. The Value Offset parameter shall be the offset
 *  within the characteristic descriptor to be read.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_READ_BLOB_RSP of type attReadBlobRsp_t if the procedure is successfull
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_READ_BLOB_RSP
 * (with @ref bleProcedureComplete or @ref bleTimeout status) or @ref ATT_ERROR_RSP
 * (with @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_ReadLongCharDesc( uint16_t connHandle, attReadBlobReq_t *pReq);

/**
 * @brief   Write Characteristic Descriptor
 *
 * This sub-procedure is used to write a characteristic
 * descriptor value to a server when the client knows the
 * characteristic descriptor handle.
 *
 * @ref ATT_WriteReq is used for this sub-procedure. The
 * Attribute Handle parameter shall be set to the characteristic
 * descriptor handle. The Attribute Value parameter shall be
 * set to the new characteristic descriptor value.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_WRITE_RSP if the procedure is successfull
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_WRITE_RSP
 * (with @ref bleSUCCESS or @ref bleTimeout status) or @ref ATT_ERROR_RSP (with
 * @ref bleSUCCESS status) is received by the calling task.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_WriteCharDesc( uint16_t connHandle, attWriteReq_t *pReq);

/**
 * @brief   Write Long Characteristic Descriptor
 *
 * This sub-procedure is used to write a Characteristic Value to
 * a server when the client knows the Characteristic Value Handle
 * but the length of the Characteristic Value is longer than can
 * be sent in a single Write Request Attribute Protocol message.
 *
 * @ref ATT_PrepareWriteReq and @ref ATT_ExecuteWriteReq are
 * used to perform this sub-procedure.
 *
 * @par Corresponding Events:
 * If the return status from this function is @ref bleSUCCESS, the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * - @ref ATT_PREPARE_WRITE_RSP of type @ref attPrepareWriteRsp_t and
 * @ref ATT_EXECUTE_WRITE_RSP if the procedure is successfull
 * - @ref ATT_ERROR_RSP of type @ref attErrorRsp_t ,if an error occurred on the server
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Request was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A response is pending with this server.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note This sub-procedure is complete when either @ref ATT_PREPARE_WRITE_RSP
 * (with @ref bleTimeout status), @ref ATT_EXECUTE_WRITE_RSP (with @ref bleSUCCESS
 * or @ref bleTimeout status), or @ref ATT_ERROR_RSP (with @ref bleSUCCESS status)
 * is received by the calling task.
 *
 * @warning The 'pReq->pValue' pointer will be freed when the sub-procedure
 * is complete.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GATT_WriteLongCharDesc( uint16_t connHandle, attPrepareWriteReq_t *pReq);


/*-------------------------------------------------------------------
 * GATT Client and Server Common APIs
 */

/**
 * Notify the stack of an updated MTU size for a given connection.
 *
 * @note It is theoretically possible for the stack to be sucesfully notified of
 * the MTU update but have the subsequent notification event sent to the
 * registered applicaiton GATT task fail due to either a memory allocation
 * failure or if no task was registered with @ref GATT_RegisterForMsgs. In this
 * case, SUCESSS will still be returned from this function.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS
 *  @ref INVALIDPARAMETER
 *  @ref bleMemAllocError
 *
 * @param   connHandle - connection handle.
 * @param   mtuSize - new MTU size.
 *
 * @return bleSUCCESS Stack was notified of updated MTU
 * @return bleFAILURE invalid MTU size or connection not found
 */

extern HCI_StatusCodes_t GATT_UpdateMTU( uint16_t connHandle, uint16_t mtuSize );


/*-------------------------------------------------------------------
 * GATT Server Sub-Procedure APIs
 */

/**
 * @brief   Send a GATT Indication
 *
 * This sub-procedure is used when a server is configured to
 * indicate a characteristic value to a client and expects an
 * attribute protocol layer acknowledgement that the indication
 * was successfully received.
 *
 * @ref ATT_HandleValueInd is used in this sub-procedure.
 *
 * @par Corresponding Events
 * If the return status from this function is @ref bleSUCCESS and the GATT client
 * succesfully sends an acknowledgement,  the calling
 * application task will receive a @ref GATT_MSG_EVENT message with method:
 * @ref ATT_HANDLE_VALUE_CFM of type @ref attHandleValueInd_t , with
 * status @ref bleSUCCESS or @ref bleTimeout . At this point, the procedure
 * is complete.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Indication was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref blePending : A confirmation is pending with this server.
 *  @ref bleInvalidMtuSize : Packet length is larger than connection's MTU size.
 *  @ref bleMemAllocError
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @warning The payload must be dynamically allocated using @ref GATT_bm_alloc
 *
 * @note The client must use @ref GATT_RegisterForInd in order to receive
 * Indications in the application task and use
 * @ref ATT_HandleValueCfm to return the acknowledgement to the server
 *
 * @param   connHandle - connection to use
 * @param   pInd - pointer to indication to be sent
 * @param   authenticated - whether an authenticated link is required
 *                          0x01: LE Legacy authenticated
 *                          0x02: Secure Connections authenticated
 * @param   taskId - task to be notified of response
 *
 * @return bleSUCCESS Stack was notified of updated MTU
 * @return bleFAILURE invalid MTU size or connection not found
 */
extern HCI_StatusCodes_t GATT_Indication( uint16_t connHandle, attHandleValueInd_t *pInd,
                                  uint8_t authenticated);

/**
 * @brief   Send a GATT Notification
 *
 * This sub-procedure is used when a server is configured to
 * notify a characteristic value to a client without expecting
 * any attribute protocol layer acknowledgement that the
 * notification was successfully received.
 *
 * @ref ATT_HandleValueNoti is used in this sub-procedure.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 *  @ref bleSUCCESS : Notification was queued successfully.
 *  @ref INVALIDPARAMETER
 *  @ref MSG_BUFFER_NOT_AVAIL
 *  @ref bleNotConnected
 *  @ref bleInvalidMtuSize : Packet length is larger than connection's MTU size.
 *  @ref bleTimeout : Previous transaction timed out.
 *
 * @note
 * A notification may be sent at any time.
 * No confirmation will be sent to the calling task for
 * this sub-procedure.
 *
 * @warning The payload must be dynamically allocated using @ref GATT_bm_alloc
 *
 * @note The client must use @ref GATT_RegisterForInd in order to receive
 * Notifications in the application task
 *
 * @param   connHandle - connection to use
 * @param   pNoti - pointer to notification to be sent
 * @param   authenticated - whether an authenticated link is required
 *                          0x01: LE Legacy authenticated
 *                          0x02: Secure Connections authenticated
 *
 * @return bleSUCCESS Stack was notified of updated MTU
 * @return bleFAILURE invalid MTU size or connection not found
 */
extern HCI_StatusCodes_t GATT_Notification( uint16_t connHandle, attHandleValueNoti_t *pNoti,
                                    uint8_t authenticated );

/**
 * @brief   Add a new service to the GATT Server on the NWP
 *
 * This command is used to add a new service to the GATT Server on the
 * Network Processor when the GATT Database is implemented on the Application
 * Processor. The GATT_AddAttribute must be used to add additional attributes
 * to the service. The new service will be automatically registered with the
 * GATT Server if it has no additional attribute to be added. Note: The Command
 * Status Event will have the Start Handle and End Handle for the service
 * registered with the GATT Server. No ATT request is used to perform this command.
 *
 * When this command is received, the host will send the CommandStatus Event.
 *
 *
 * @param   uuid - The type of the service to be added (primary or secondary)
 * @param   numAttrs - The number of the attributes in the service (including the
 *                      service attribute)
 * @param   encKeySize - The minimum encryption key size (in octets) required by the service.
 *
 * @return bleSUCCESS Stack was notified of updated MTU
 * @return bleFAILURE invalid MTU size or connection not found
 */
extern HCI_StatusCodes_t GATT_AddService( uint16_t uuid, uint16_t numAttrs,
                                    uint8_t encKeySize );

/**
 * @brief   Delete a service from the GATT Server on the NWP
 *
 * This command is used to delete a service from the GATT Server on the
 * Network Processor when the GATT Database is implemented on the Application
 * Processor.
 *
 * When this command is received, the host will send the CommandStatus Event.
 *
 * @param   handle - The handle of the service to be deleted
 *
 * @return bleSUCCESS Stack was notified of updated MTU
 * @return bleFAILURE invalid MTU size or connection not found
 */
extern HCI_StatusCodes_t GATT_DelService(uint16_t handle);

/**
 * This command is used to add a new attribute to the service being added to
 * the GATT Server on the Network Processor when the GATT Database is implemented
 * on the Application Processor. The service will be automatically registered with
 * the GATT Server when its last attribute is added. Note: The Command Status Event
 * will have the Start Handle and End Handle for the service registered with the
 * GATT Server.
 *
 * @param   uuid            The type of the attribute to be added
 * @param   permissions     Attribute access permissions
 */
extern HCI_StatusCodes_t GATT_AddAttribute(uint16_t uuid, uint8_t permissions);

/**
 * Same as GATT_AddAttribute except takes 128-bit UUID as parameter
 *
 * @param   uuid            The type of the attribute to be added
 * @param   permissions     Attribute access permissions
 */
extern HCI_StatusCodes_t GATT_AddAttribute2(const uint8_t *uuid128, uint8_t permissions);

#endif /* GATT_H */
