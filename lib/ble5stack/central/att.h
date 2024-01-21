/******************************************************************************

 Group: WCS, BTS
 Target Device: cc13x2_26x2

 ******************************************************************************

 Copyright (c) 2009-2019, Texas Instruments Incorporated
 All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License"). You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product. Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.

 ******************************************************************************


 *****************************************************************************/

/**
 *  @defgroup ATT_GATT ATT / GATT
 *  @brief This module implements the Attribute Protocol and Generic Attribute Protocol
 *  @{
 *
 *  @file  att.h
 *  @brief      ATT layer interface
 *
 *  @ref ATT_METHOD_DEFINES
 */

#ifndef ATT_H
#define ATT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
//#include "osal.h"

//#include "l2cap.h"

/*********************************************************************
 * CONSTANTS
 */

/**
 * ATT Vendor Specific APIs - ATT Command Opcodes
 */
#define ATT_CMD_ERROR_RSP               0xFD01
#define ATT_CMD_EXCHANGEMTUREQ          0xFD02
#define ATT_CMD_EXCHANGE_MTU_RSP        0xFD03
#define ATT_CMD_FINDINFOREQ             0xFD04
#define ATT_CMD_FINDINFORSP             0xFD05
#define ATT_CMD_FIND_BY_TYPE_VALUE_REQ  0xFD06
#define ATT_CMD_FIND_BY_TYPE_VALUE_RSP  0xFD07
#define ATT_CMD_READ_BY_TYPE_VALUE_REQ  0xFD08
#define ATT_CMD_READ_BY_TYPE_VALUE_RSP  0xFD09
#define ATT_CMD_READREQ                 0xFD0A
#define ATT_CMD_READ_RSP                0xFD0B
#define ATT_CMD_READBLOBREQ             0xFD0C
#define ATT_CMD_READBLOBRSP             0xFD0D
#define ATT_CMD_READMULTIREQ            0xFD0E
#define ATT_CMD_READMULTIRSP            0xFD0F
#define ATT_CMD_READBYGRPTYPEREQ        0xFD10
#define ATT_CMD_READ_BY_GRP_TYPE_RSP    0xFD11
#define ATT_CMD_WRITEREQ                0xFD12
#define ATT_CMD_WRITE_RSP               0xFD13
#define ATT_CMD_PREPAREWRITEREQ         0xFD16
#define ATT_CMD_PREPAREWRITERSP         0xFD17
#define ATT_CMD_EXECUTEWRITEREQ         0xFD18
#define ATT_CMD_EXECUTE_WRITE_RSP       0xFD19
#define ATT_CMD_HANDLE_VALUE_NOTI       0xFD1B
#define ATT_CMD_HANDLEVALUEIND          0xFD1D
#define ATT_CMD_HANDLEVALUECFM          0xFD1E


/**
 * ATT Vendor Specific APIs - ATT Event Opcodes
 */
#define ATT_EVENT_ERROR_RSP               0x0501
#define ATT_EVENT_EXCHANGE_MTU_REQ        0x0502
#define ATT_EVENT_EXCHANGEMTURSP          0x0503
#define ATT_EVENT_FINDINFOREQ             0x0504
#define ATT_EVENT_FINDINFORSP             0x0505
#define ATT_EVENT_FIND_BY_TYPE_VALUE_REQ  0x0506
#define ATT_EVENT_FIND_BY_TYPE_VALUE_RSP  0x0507
#define ATT_EVENT_READ_BY_TYPE_REQ        0x0508
#define ATT_EVENT_READ_BY_TYPE_RSP        0x0509
#define ATT_EVENT_READ_REQ                0x050A
#define ATT_EVENT_READRSP                 0x050B
#define ATT_EVENT_READBLOBREQ             0x050C
#define ATT_EVENT_READBLOBRSP             0x050D
#define ATT_EVENT_READMULTIREQ            0x050E
#define ATT_EVENT_READMULTIRSP            0x050F
#define ATT_EVENT_READ_BY_GRP_TYPE_REQ    0x0510
#define ATT_EVENT_READ_BY_GRP_TYPE_RSP    0x0511
#define ATT_EVENT_WRITE_REQ               0x0512
#define ATT_EVENT_WRITE_RSP               0x0513
#define ATT_EVENT_PREPARE_WRITE_REQ       0x0516
#define ATT_EVENT_PREPAREWRITERSP         0x0517
#define ATT_EVENT_EXECUTE_WRITE_REQ       0x0518
#define ATT_EVENT_EXECUTEWRITERSP         0x0519
#define ATT_EVENT_HANDLE_VALUE_NOTI       0x051B
#define ATT_EVENT_HANDLEVALUEIND          0x051D
#define ATT_EVENT_HANDLEVALUECFM          0x051E
#define ATT_EVENT_FLOWCTRLVIOLATED        0x057E
#define ATT_EVENT_MTUUPDATEDEVT           0x051F


/**
 * @defgroup ATT_GATT_Constants ATT / GATT Constants
 * @{
 */

/**
 * @defgroup ATT_MTU_Sizes ATT MTU Size
 * @{
 * The Exchanging MTU Size is defined as the maximum size of any packet
 * transmitted between a client and a server. A higher layer specification
 * defines the default ATT MTU value. The ATT MTU value should be within
 * the range 23 to 251 inclusive.
 * Refer to ble_user_config.h for the device-specific maximum MTU value.
 */
#define ATT_MTU_SIZE                     23 //L2CAP_MTU_SIZE //!< Minimum ATT MTU size
#define ATT_MAX_MTU_SIZE                 158 //(255-L2CAP_HDR_SIZE) //!< Maximum ATT MTU size
// Pybricks: official LEGO firmware uses 158
/** @} End ATT_MTU_Sizes */

/**
 * @defgroup ATT_METHOD_DEFINES ATT Methods
 * @{
 */
#define ATT_ERROR_RSP                    0x01 //!< ATT Error Response. This method is passed as a gattMsgEvent_t defined as @ref attErrorRsp_t
#define ATT_EXCHANGE_MTU_REQ             0x02 //!< ATT Exchange MTU Request. This method is passed as a GATT message defined as @ref attExchangeMTUReq_t
#define ATT_EXCHANGE_MTU_RSP             0x03 //!< ATT Exchange MTU Response. This method is passed as a GATT message defined as @ref attExchangeMTURsp_t
#define ATT_FIND_INFO_REQ                0x04 //!< ATT Find Information Request. This method is passed as a GATT message defined as @ref attFindInfoReq_t
#define ATT_FIND_INFO_RSP                0x05 //!< ATT Find Information Response. This method is passed as a GATT message defined as @ref attFindInfoRsp_t
#define ATT_FIND_BY_TYPE_VALUE_REQ       0x06 //!< ATT Find By Type Value Request. This method is passed as a GATT message defined as @ref attFindByTypeValueReq_t
#define ATT_FIND_BY_TYPE_VALUE_RSP       0x07 //!< ATT Find By Type Value Response. This method is passed as a GATT message defined as @ref attFindByTypeValueRsp_t
#define ATT_READ_BY_TYPE_REQ             0x08 //!< ATT Read By Type Request. This method is passed as a GATT message defined as @ref attReadByTypeReq_t
#define ATT_READ_BY_TYPE_RSP             0x09 //!< ATT Read By Type Response. This method is passed as a GATT message defined as @ref attReadByTypeRsp_t
#define ATT_READ_REQ                     0x0a //!< ATT Read Request. This method is passed as a GATT message defined as @ref attReadReq_t
#define ATT_READ_RSP                     0x0b //!< ATT Read Response. This method is passed as a GATT message defined as @ref attReadRsp_t
#define ATT_READ_BLOB_REQ                0x0c //!< ATT Read Blob Request. This method is passed as a GATT message defined as @ref attReadBlobReq_t
#define ATT_READ_BLOB_RSP                0x0d //!< ATT Read Blob Response. This method is passed as a GATT message defined as @ref attReadBlobRsp_t
#define ATT_READ_MULTI_REQ               0x0e //!< ATT Read Multiple Request. This method is passed as a GATT message defined as @ref attReadMultiReq_t
#define ATT_READ_MULTI_RSP               0x0f //!< ATT Read Multiple Response. This method is passed as a GATT message defined as @ref attReadMultiRsp_t
#define ATT_READ_BY_GRP_TYPE_REQ         0x10 //!< ATT Read By Group Type Request. This method is passed as a GATT message defined as @ref attReadByGrpTypeReq_t
#define ATT_READ_BY_GRP_TYPE_RSP         0x11 //!< ATT Read By Group Type Response. This method is passed as a GATT message defined as @ref attReadByGrpTypeRsp_t
#define ATT_WRITE_REQ                    0x12 //!< ATT Write Request. This method is passed as a GATT message defined as @ref attWriteReq_t
#define ATT_WRITE_RSP                    0x13 //!< ATT Write Response. This method is passed as a GATT message
#define ATT_PREPARE_WRITE_REQ            0x16 //!< ATT Prepare Write Request. This method is passed as a GATT message defined as @ref attPrepareWriteReq_t
#define ATT_PREPARE_WRITE_RSP            0x17 //!< ATT Prepare Write Response. This method is passed as a GATT message defined as @ref attPrepareWriteRsp_t
#define ATT_EXECUTE_WRITE_REQ            0x18 //!< ATT Execute Write Request. This method is passed as a GATT message defined as @ref attExecuteWriteReq_t
#define ATT_EXECUTE_WRITE_RSP            0x19 //!< ATT Execute Write Response. This method is passed as a GATT message defines as @ref attHandleValueNoti_t
#define ATT_HANDLE_VALUE_NOTI            0x1b //!< ATT Handle Value Notification. This method is passed as a GATT message defined as @ref attErrorRsp_t
#define ATT_HANDLE_VALUE_IND             0x1d //!< ATT Handle Value Indication. This method is passed as a GATT message defined as @ref attHandleValueInd_t
#define ATT_HANDLE_VALUE_CFM             0x1e //!< ATT Handle Value Confirmation. This method is passed as a GATT message
#define ATT_UNSUPPORTED_METHOD           0x20//!< Garbage ATT opcode used for negative testing

#define ATT_WRITE_CMD                    0x52 //!< ATT Write Command
#define ATT_SIGNED_WRITE_CMD             0xD2 //!< ATT Signed Write Command
/** @} End ATT_METHOD_DEFINES */

/**
 * @defgroup ATT_MSG_EVENT_DEFINES ATT Event IDs
 * @{
 */
#define ATT_TRANSACTION_READY_EVENT     0x7D //!< ATT transaction ready on the specified connection handle.  This event is sent as an OSAL message defined as attTransactionReadyEvt_t.
#define ATT_FLOW_CTRL_VIOLATED_EVENT    0x7E //!< Sent when ATT flow control is violated on a connection.  This event is sent as an OSAL message defined as attFlowCtrlViolatedEvt_t.
#define ATT_MTU_UPDATED_EVENT           0x7F //!< Sent when MTU is updated for a connection.  This event is sent as an OSAL message defined as attMtuUpdatedEvt_t.
/** @} End ATT_MSG_EVENT_DEFINES */


/**
 * @defgroup ATT_Bitmasks ATT Opcode field bitmasks
 * @{
 */
#define ATT_METHOD_BITS                  0x3f //!< Method (bits 5-0)
#define ATT_CMD_FLAG_BIT                 0x40 //!< Command Flag (bit 6)
#define ATT_AUTHEN_SIG_FLAG_BIT          0x80 //!< Authentication Signature Flag (bit 7)
#define ATT_BT_UUID_SIZE                 2 //!< Size of 16-bit Bluetooth UUID
#define ATT_UUID_SIZE                    16 //!< Size of 128-bit UUID
#define ATT_MSG_TIMEOUT                  30 //!< ATT Response or Confirmation timeout
/** @} End ATT_Bitmasks */

/**
 * @defgroup ATT_Sig_Status Authentication Signature status
 * @{
 * Authentication Signature status for received PDU; it's TRUE or FALSE for PDU to be sent
 */
#define ATT_SIG_NOT_INCLUDED             0x00 //!< Signature not included
#define ATT_SIG_VALID                    0x01 //!< Included signature valid
#define ATT_SIG_INVALID                  0x02 //!< Included signature not valid
/** @} End ATT_Sig_Status */

/*********************************************************************
 * Error Response: Error Code
 */

/**
 * @defgroup ATT_ERR_CODE_DEFINES ATT Error Codes
 * @{
 */
#define ATT_ERR_INVALID_HANDLE           0x01 //!< Attribute handle value given was not valid on this attribute server
#define ATT_ERR_READ_NOT_PERMITTED       0x02 //!< Attribute cannot be read
#define ATT_ERR_WRITE_NOT_PERMITTED      0x03 //!< Attribute cannot be written
#define ATT_ERR_INVALID_PDU              0x04 //!< The attribute PDU was invalid
#define ATT_ERR_INSUFFICIENT_AUTHEN      0x05 //!< The attribute requires authentication before it can be read or written
#define ATT_ERR_UNSUPPORTED_REQ          0x06 //!< Attribute server doesn't support the request received from the attribute client
#define ATT_ERR_INVALID_OFFSET           0x07 //!< Offset specified was past the end of the attribute
#define ATT_ERR_INSUFFICIENT_AUTHOR      0x08 //!< The attribute requires an authorization before it can be read or written
#define ATT_ERR_PREPARE_QUEUE_FULL       0x09 //!< Too many prepare writes have been queued
#define ATT_ERR_ATTR_NOT_FOUND           0x0a //!< No attribute found within the given attribute handle range
#define ATT_ERR_ATTR_NOT_LONG            0x0b //!< Attribute cannot be read or written using the Read Blob Request or Prepare Write Request
#define ATT_ERR_INSUFFICIENT_KEY_SIZE    0x0c //!< The Encryption Key Size used for encrypting this link is insufficient
#define ATT_ERR_INVALID_VALUE_SIZE       0x0d //!< The attribute value length is invalid for the operation
#define ATT_ERR_UNLIKELY                 0x0e //!< The attribute request that was requested has encountered an error that was very unlikely, and therefore could not be completed as requested
#define ATT_ERR_INSUFFICIENT_ENCRYPT     0x0f //!< The attribute requires encryption before it can be read or written
#define ATT_ERR_UNSUPPORTED_GRP_TYPE     0x10 //!< The attribute type is not a supported grouping attribute as defined by a higher layer specification
#define ATT_ERR_INSUFFICIENT_RESOURCES   0x11 //!< Insufficient Resources to complete the request

/*** Reserved for future use: 0x12 - 0x7F ***/

/*** Application error code defined by a higher layer specification: 0x80-0x9F ***/

#define ATT_ERR_INVALID_VALUE            0x80 //!< The attribute value is invalid for the operation
/** @} End ATT_ERR_CODE_DEFINES */

/*********************************************************************
 * Find Information Response: UUID Format
 */
#define ATT_HANDLE_BT_UUID_TYPE          0x01 //!< Handle and 16-bit Bluetooth UUID
#define ATT_HANDLE_UUID_TYPE             0x02 //!< Handle and 128-bit UUID

/*********************************************************************
 * Read Multiple Request: Handles
 */
#define ATT_MIN_NUM_HANDLES              2 //!< Minimum number of handles in a single Read Multiple Request

/*********************************************************************
 * Execute Write Request: Flags
 */
#define ATT_CANCEL_PREPARED_WRITES       0x00 //!< Cancel all prepared writes
#define ATT_WRITE_PREPARED_VALUES        0x01 //!< Immediately write all pending prepared values

#if defined ( TESTMODES )
  //!< ATT Test Modes
  #define ATT_TESTMODE_OFF               0 //!< Test mode off
  #define ATT_TESTMODE_UNAUTHEN_SIG      1 //!< Do not authenticate incoming signature
  #define ATT_TESTMODE_SEND_UNS_REQ      2 //!< Send ATT request with unsupported opcode
  #define ATT_TESTMODE_SEND_UNS_CMD      3 //!< Send ATT command with unsupported opcode
#endif

/**
 * @defgroup ATT_Request_Sizes Size of mandatory fields of ATT requests
 * @{
 */
#define ATT_OPCODE_SIZE                        1 //!< Length of opcode field: Opcode (1)
#define ATT_EXCHANGE_MTU_REQ_SIZE              2 //!< Length of Exchange MTU Request: client receive MTU size (2)
#define ATT_EXCHANGE_MTU_RSP_SIZE              2 //!< Length of Exchange MTU Response: server receive MTU size (2)
#define ATT_ERROR_RSP_SIZE                     4 //!< Length of Error Response: Command opcode in error (1) + Attribute handle in error (2) + Status code (1)
#define ATT_FIND_INFO_REQ_FIXED_SIZE           4 //!< Length of Find Information Request's fixed fields: First handle number (2) + Last handle number (2)
#define ATT_FIND_INFO_RSP_FIXED_SIZE           1 //!< Length of Find Information Response's fixed field: Format (1)
#define ATT_FIND_INFO_RSP_HDR_SIZE             ( ATT_OPCODE_SIZE + ATT_FIND_INFO_RSP_FIXED_SIZE ) //!< Length of Find Information Response's Header
#define ATT_FIND_BY_TYPE_VALUE_REQ_FIXED_SIZE  6 //!< Length of Find By Type Value Request's fixed fields: Start handle (2) + End handle (2) + Type (2)
#define ATT_FIND_BY_TYPE_VALUE_REQ_HDR_SIZE    ( ATT_OPCODE_SIZE + ATT_FIND_BY_TYPE_VALUE_REQ_FIXED_SIZE ) //!< Length of Find By Type Value Request's Header
#define ATT_READ_BY_TYPE_REQ_FIXED_SIZE        4 //!< Length of Read By Type Request's fixed fields: First handle number (2) + Last handle number (2)
#define ATT_READ_BY_TYPE_REQ_HDR_SIZE          ( ATT_OPCODE_SIZE + ATT_READ_BY_TYPE_REQ_FIXED_SIZE ) //!< Size of Read by Type Request Header
#define ATT_READ_BY_TYPE_RSP_FIXED_SIZE        1 //!< Length of Read By Type Response's fixed fields: Length (1)
#define ATT_READ_BY_TYPE_RSP_HDR_SIZE          ( ATT_OPCODE_SIZE + ATT_READ_BY_TYPE_RSP_FIXED_SIZE ) //!< Size of Read by Type Response Header
#define ATT_READ_REQ_SIZE                      2 //!< Length of Read Request: Attribute Handle (2)
#define ATT_READ_BY_GRP_TYPE_RSP_FIXED_SIZE    1 //!< Length of Read By Type Response's fixed fields: Length (1)
#define ATT_READ_BY_GRP_TYPE_RSP_HDR_SIZE      ( ATT_OPCODE_SIZE + ATT_READ_BY_GRP_TYPE_RSP_FIXED_SIZE ) //!< Size of Read by Group Type Response Header
#define ATT_WRITE_REQ_FIXED_SIZE               2 //!< Length of Write Request's fixed field: Attribute Handle (2)
#define ATT_READ_BLOB_REQ_SIZE                 4 //!< Length of Read Blob Request: Attribute Handle (2) + Value Offset (2)
#define ATT_WRITE_REQ_FIXED_SIZE               2 //!< Length of Write Request's fixed field: Attribute Handle (2)
#define ATT_WRITE_REQ_HDR_SIZE                 ( ATT_OPCODE_SIZE + ATT_WRITE_REQ_FIXED_SIZE ) //!< Size of Write Request Header
#define ATT_PREPARE_WRITE_REQ_FIXED_SIZE       4 //!< Length of Prepare Write Request's fixed fields: Attribute Handle (2) + Value Offset (2)
#define ATT_PREPARE_WRITE_REQ_HDR_SIZE         ( ATT_OPCODE_SIZE + ATT_PREPARE_WRITE_REQ_FIXED_SIZE ) //!< Size of Prepare Write Request Header
#define ATT_PREPARE_WRITE_RSP_FIXED_SIZE       4 //!< Length of Prepare Write Response's fixed size: Attribute Handle (2) + Value Offset (2)
#define ATT_EXECUTE_WRITE_REQ_SIZE             1 //!< Length of Execute Write Request: Flags (1)
#define ATT_HANDLE_VALUE_IND_FIXED_SIZE        2 //!< Length of Handle Value Indication's fixed size: Attribute Handle (2)
#define ATT_HANDLE_VALUE_IND_HDR_SIZE          ( ATT_OPCODE_SIZE + ATT_HANDLE_VALUE_IND_FIXED_SIZE ) //!< Size of Indication header
#define ATT_AUTHEN_SIG_LEN                     12 //!< Length of Authentication Signature field
/** @} End ATT_Request_Sizes */

/*********************************************************************
 * VARIABLES
 */

///// @cond NODOC
//extern CONST uint8_t btBaseUUID[ATT_UUID_SIZE];
///// @endcond // NODOC

/*********************************************************************
 * MACROS
 */
/// @brief ATT Write Command
#define ATT_WRITE_COMMAND( method, cmd ) ( ( (method) == ATT_WRITE_REQ ) && ( (cmd) == TRUE ) )

/*********************************************************************
 * ATT Find Info Response macros
 */

/// @cond NODOC

// Handle and 16-bit Bluetooth UUID pair indexes
#define ATT_BT_PAIR_HANDLE_IDX( i )        ( (i) * (2 + ATT_BT_UUID_SIZE) )
#define ATT_BT_PAIR_UUID_IDX( i )          ( ATT_BT_PAIR_HANDLE_IDX( (i) ) + 2 )

#define ATT_BT_PAIR_HANDLE( info, i )      ( BUILD_UINT16( (info)[ATT_BT_PAIR_HANDLE_IDX((i))], \
                                                           (info)[ATT_BT_PAIR_HANDLE_IDX((i))+1] ) )
#define ATT_BT_PAIR_UUID( info, i )        ( BUILD_UINT16( (info)[ATT_BT_PAIR_UUID_IDX((i))], \
                                                           (info)[ATT_BT_PAIR_UUID_IDX((i))+1] ) )

// Handle and 128-bit UUID pair indexes
#define ATT_PAIR_HANDLE_IDX( i )           ( (i) * (2 + ATT_UUID_SIZE) )
#define ATT_PAIR_UUID_IDX( i )             ( ATT_PAIR_HANDLE_IDX( (i) ) + 2 )

#define ATT_PAIR_HANDLE( info, i )         ( BUILD_UINT16( (info)[ATT_PAIR_HANDLE_IDX((i))], \
                                                           (info)[ATT_PAIR_HANDLE_IDX((i))+1] ) )

/*********************************************************************
 * ATT Find By Type Value Response macros
 */
// Attribute Handle and Group End Handle pair indexes
#define ATT_ATTR_HANDLE_IDX( i )           ( (i) * (2 + 2) )
#define ATT_GRP_END_HANDLE_IDX( i )        ( ATT_ATTR_HANDLE_IDX( (i) ) + 2 )

#define ATT_ATTR_HANDLE( info, i )         ( BUILD_UINT16( (info)[ATT_ATTR_HANDLE_IDX((i))], \
                                                           (info)[ATT_ATTR_HANDLE_IDX((i))+1] ) )
#define ATT_GRP_END_HANDLE( info, i )      ( BUILD_UINT16( (info)[ATT_GRP_END_HANDLE_IDX((i))], \
                                                           (info)[ATT_GRP_END_HANDLE_IDX((i))+1] ) )

/*********************************************************************
 * ATT Read Multiple Request macros
 */
// Attribute Handle index
#define ATT_HANDLE_IDX( i )                ( (i) * 2 )

#define ATT_HANDLE( handles, i )           ( BUILD_UINT16( (handles)[ATT_HANDLE_IDX((i))], \
                                                           (handles)[ATT_HANDLE_IDX((i))+1] ) )

/// @endcond // NODOC

/** @} End ATT_GATT_Constants */

/*********************************************************************
 * TYPEDEFS
 */

/**
 * @defgroup ATT_GATT_Structs ATT / GATT Stuctures
 * @{
 */

/// @brief Attribute Protocol PDU format.
typedef struct
{
  uint8_t sig;      //!< Authentication Signature status (not included (0), valid (1), invalid (2))
  uint8_t cmd;      //!< Command Flag
  uint8_t method;   //!< Method
  uint16_t len;     //!< Length of Attribute Parameters
  uint8_t *pParams; //!< Attribute Parameters
} attPacket_t;

/// @brief Attribute Type format (2 or 16 octet UUID).
typedef struct
{
  uint8_t len;                 //!< Length of UUID (2 or 16)
  uint8_t uuid[ATT_UUID_SIZE]; //!< 16 or 128 bit UUID
} attAttrType_t;

/// @brief Attribute Type format (2-octet Bluetooth UUID).
typedef struct
{
  uint8_t len;                    //!< Length of UUID (2)
  uint8_t uuid[ATT_BT_UUID_SIZE]; //!< 16 bit UUID
} attAttrBtType_t;

/**
 * @defgroup ATT_GATT_Events ATT / GATT Events
 * @{
 */

/**
* @brief Error Response format.
*
* for method @ref ATT_ERROR_RSP
*/
typedef struct
{
  uint8_t reqOpcode; //!< Request that generated this error response
  uint16_t handle;   //!< Attribute handle that generated error response
  uint8_t errCode;   //!< Reason why the request has generated error response
} attErrorRsp_t;

/**
 * @brief Exchange MTU Request format.
 *
 * for method @ref ATT_EXCHANGE_MTU_REQ
 */
typedef struct
{
  uint16_t clientRxMTU; //!< Client receive MTU size
} attExchangeMTUReq_t;

/**
 * @brief Exchange MTU Response format.
 *
 * for method @ref ATT_EXCHANGE_MTU_REQ
 */
typedef struct
{
  uint16_t serverRxMTU; //!< Server receive MTU size
} attExchangeMTURsp_t;

/**
 * @brief Find Information Request format.
 *
 * for method @ref ATT_FIND_INFO_REQ
 */
typedef struct
{
  uint16_t startHandle;       //!< First requested handle number (must be first field)
  uint16_t endHandle;         //!< Last requested handle number
} attFindInfoReq_t;

/// @brief Handle and its 16-bit Bluetooth UUIDs.
typedef struct
{
  uint16_t handle;                //!< Handle
  uint8_t uuid[ATT_BT_UUID_SIZE]; //!< 2-octet Bluetooth UUID
} attHandleBtUUID_t;

/// @brief Handle and its 128-bit UUID.
typedef struct
{
  uint16_t handle;             //!< Handle
  uint8_t uuid[ATT_UUID_SIZE]; //!< 16-octect UUID
} attHandleUUID_t;

/**
 * @brief Find Information Response format.
 *
 * for method @ref ATT_FIND_INFO_RSP
 */
typedef struct
{
  uint16_t numInfo; //!< Number of attribute handle-UUID pairs found
  uint8_t format;   //!< Format of information data
  uint8_t *pInfo;   //!< Information data whose format is determined by format field (4 to ATT_MTU_SIZE-2)
} attFindInfoRsp_t;

/**
 * @brief Find By Type Value Request format.
 *
 * for method @ref ATT_FIND_BY_TYPE_VALUE_REQ
 */
typedef struct
{
  uint16_t startHandle;   //!< First requested handle number (must be first field)
  uint16_t endHandle;     //!< Last requested handle number
  attAttrBtType_t type; //!< 2-octet UUID to find
  uint16_t len;           //!< Length of value
  uint8_t *pValue;        //!< Attribute value to find (0 to ATT_MTU_SIZE-7)
} attFindByTypeValueReq_t;

/// @brief Handles Information format.
typedef struct
{
  uint16_t handle;       //!< Found attribute handle
  uint16_t grpEndHandle; //!< Group end handle
} attHandlesInfo_t;

/**
 * @brief Find By Type Value Response format.
 *
 * for method @ref ATT_FIND_BY_TYPE_VALUE_RSP
 */
typedef struct
{
  uint16_t numInfo;      //!< Number of handles information found
  uint8_t *pHandlesInfo; //!< List of 1 or more handles information (4 to ATT_MTU_SIZE-1)
} attFindByTypeValueRsp_t;

/**
 * @brief Read By Type Request format.
 *
 * for method @ref ATT_READ_BY_TYPE_REQ
 */
typedef struct
{
  uint16_t startHandle; //!< First requested handle number (must be first field)
  uint16_t endHandle;   //!< Last requested handle number
  attAttrType_t type; //!< Requested type (2 or 16 octet UUID)
} attReadByTypeReq_t;

/**
 * @brief Read By Type Response format.
 *
 * for method @ref ATT_READ_BY_TYPE_RSP
 */
typedef struct
{
//   uint16_t numPairs;  //!< Number of attribute handle-UUID pairs found
//   uint16_t len;       //!< Size of each attribute handle-value pair
  uint8_t *pDataList; //!< List of 1 or more attribute handle-value pairs (2 to ATT_MTU_SIZE-2)
  uint16_t dataLen;   //!< Length of data written into pDataList. Not part of actual ATT Response
} attReadByTypeRsp_t;

/**
 * @brief Read Request
 *
 * for method @ref ATT_READ_REQ
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute to be read (must be first field)
} attReadReq_t;

/**
 * @brief Read Response
 *
 * for method @ref ATT_READ_RSP
 */
typedef struct
{
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Value of the attribute with the handle given (0 to ATT_MTU_SIZE-1)
} attReadRsp_t;

/**
 * @brief Read Blob Request format.
 *
 * for method @ref ATT_READ_BLOB_REQ
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute to be read (must be first field)
  uint16_t offset; //!< Offset of the first octet to be read
} attReadBlobReq_t;

/**
 * @brief Read Blob Response format.
 *
 * for method @ref ATT_READ_BLOB_RSP
 */
typedef struct
{
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Part of the value of the attribute with the handle given (0 to ATT_MTU_SIZE-1)
} attReadBlobRsp_t;

/**
 * @brief Read Multiple Request format.
 *
 * for method @ref ATT_READ_MULTI_REQ
 */
typedef struct
{
  uint8_t *pHandles;   //!< Set of two or more attribute handles (4 to ATT_MTU_SIZE-1) - must be first field
  uint16_t numHandles; //!< Number of attribute handles
} attReadMultiReq_t;

/**
 * @brief Read Multiple Response format.
 *
 * for method @ref ATT_READ_MULTI_RSP
 */
typedef struct
{
  uint16_t len;     //!< Length of values
  uint8_t *pValues; //!< Set of two or more values (0 to ATT_MTU_SIZE-1)
} attReadMultiRsp_t;

/**
 * @brief Read By Group Type Request format.
 *
 * for method @ref ATT_READ_BY_GRP_TYPE_REQ
 */
typedef struct
{
  uint16_t startHandle; //!< First requested handle number (must be first field)
  uint16_t endHandle;   //!< Last requested handle number
  attAttrType_t type; //!< Requested group type (2 or 16 octet UUID)
} attReadByGrpTypeReq_t;

/**
 * @brief Read By Group Type Response format.
 *
 * for method @ref ATT_READ_BY_GRP_TYPE_RSP
 */
typedef struct
{
//   uint16_t numGrps;   //!< Number of attribute handle, end group handle and value sets found
//   uint16_t len;       //!< Length of each attribute handle, end group handle and value set
  uint8_t *pDataList; //!< List of 1 or more attribute handle, end group handle and value (4 to ATT_MTU_SIZE-2)
  uint16_t dataLen;   //!< Length of data written into pDataList. Not part of actual ATT Response
} attReadByGrpTypeRsp_t;

/**
 * @brief Write Request format.
 *
 * for method @ref ATT_WRITE_REQ
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute to be written (must be first field)
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Value of the attribute to be written (0 to ATT_MTU_SIZE-3)
  uint8_t sig;     //!< Authentication Signature status (not included (0), valid (1), invalid (2))
  uint8_t cmd;     //!< Command Flag
} attWriteReq_t;

/**
 * @brief Prepare Write Request format.
 *
 * for method @ref ATT_PREPARE_WRITE_REQ
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute to be written (must be first field)
  uint16_t offset; //!< Offset of the first octet to be written
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Part of the value of the attribute to be written (0 to ATT_MTU_SIZE-5) - must be allocated
} attPrepareWriteReq_t;

/**
 * @brief Prepare Write Response format.
 *
 * for method @ref ATT_PREPARE_WRITE_RSP
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute that has been read
  uint16_t offset; //!< Offset of the first octet to be written
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Part of the value of the attribute to be written (0 to ATT_MTU_SIZE-5)
} attPrepareWriteRsp_t;

/**
 * @brief Execute Write Request format.
 *
 * for method @ref ATT_EXECUTE_WRITE_REQ
 */
typedef struct
{
  uint8_t flags; //!< 0x00 - cancel all prepared writes.
               //!< 0x01 - immediately write all pending prepared values.
} attExecuteWriteReq_t;

/**
 * @brief Handle Value Notification format.
 *
 * for method @ref ATT_HANDLE_VALUE_NOTI
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute that has been changed (must be first field)
  uint16_t len;    //!< Length of value
  const uint8_t *pValue; //!< Current value of the attribute (0 to ATT_MTU_SIZE-3)
} attHandleValueNoti_t;

/**
 * @brief Handle Value Indication format.
 *
 * for method @ref ATT_HANDLE_VALUE_IND
 */
typedef struct
{
  uint16_t handle; //!< Handle of the attribute that has been changed (must be first field)
  uint16_t len;    //!< Length of value
  uint8_t *pValue; //!< Current value of the attribute (0 to ATT_MTU_SIZE-3)
} attHandleValueInd_t;

/**
 * The following two ATT events are generated locally (not received OTA) by
 * the ATT Server or Client.
 */

/**
 * @brief ATT Flow Control Violated Event message format.
 *
 * This message is sent as a @ref GATT_MSG_EVENT of method @ref ATT_FLOW_CTRL_VIOLATED_EVENT
 * to the app by the local ATT Server or Client when a sequential ATT Request-Response
 * or Indication-Confirmation protocol flow control is violated for a connection.
 * All subsequent ATT Requests and Indications received by the local ATT Server
 * and Client respectively will be dropped.
 *
 * This message is to inform the app (that has registered with GAP by calling
 * @ref GAP_RegisterForMsgs) in case it wants to drop the connection.
 */
typedef struct
{
  uint8_t opcode;        //!< opcode of message that caused flow control violation
  uint8_t pendingOpcode; //!< opcode of pending message
} attFlowCtrlViolatedEvt_t;

/**
 * @brief ATT MTU Updated Event message format.
 *
 * This message is sent to the app
 * by the local ATT Server or Client when the ATT MTU size is updated for a
 * connection. The default ATT MTU size is 23 octets.
 *
 * This message is to inform the app (that has registered with GAP by calling
 * @brief GAP_RegisterForMsgs about the new ATT MTU size negotiated for a connection.
 */
typedef struct
{
  uint16_t MTU; //!< new MTU size
} attMtuUpdatedEvt_t;

/** @} End ATT_Events */

/**
 * @brief ATT Message format.
 *
 * This is union of all attribute protocol messages and
 * locally-generated events used between the attribute protocol and upper
 * layer profile/application.
 */
typedef union
{
  // Request messages
  attExchangeMTUReq_t exchangeMTUReq;         //!< ATT Exchange MTU Request
  attFindInfoReq_t findInfoReq;               //!< ATT Find Information Request
  attFindByTypeValueReq_t findByTypeValueReq; //!< ATT Find By Type Value Request
  attReadByTypeReq_t readByTypeReq;           //!< ATT Read By Type Request
  attReadReq_t readReq;                       //!< ATT Read Request
  attReadBlobReq_t readBlobReq;               //!< ATT Read Blob Request
  attReadMultiReq_t readMultiReq;             //!< ATT Read Multiple Request
  attReadByGrpTypeReq_t readByGrpTypeReq;     //!< ATT Read By Group Type Request
  attWriteReq_t writeReq;                     //!< ATT Write Request
  attPrepareWriteReq_t prepareWriteReq;       //!< ATT Prepare Write Request
  attExecuteWriteReq_t executeWriteReq;       //!< ATT Execute Write Request

  // Response messages
  attErrorRsp_t errorRsp;                     //!< ATT Error Response
  attExchangeMTURsp_t exchangeMTURsp;         //!< ATT Exchange MTU Response
  attFindInfoRsp_t findInfoRsp;               //!< ATT Find Information Response
  attFindByTypeValueRsp_t findByTypeValueRsp; //!< ATT Find By Type Value Response
  attReadByTypeRsp_t readByTypeRsp;           //!< ATT Read By Type Response
  attReadRsp_t readRsp;                       //!< ATT Read Response
  attReadBlobRsp_t readBlobRsp;               //!< ATT Read Blob Response
  attReadMultiRsp_t readMultiRsp;             //!< ATT Read Multiple Response
  attReadByGrpTypeRsp_t readByGrpTypeRsp;     //!< ATT Read By Group Type Response
  attPrepareWriteRsp_t prepareWriteRsp;       //!< ATT Prepare Write Response

  // Indication and Notification messages
  attHandleValueNoti_t handleValueNoti;       //!< ATT Handle Value Notification
  attHandleValueInd_t handleValueInd;         //!< ATT Handle Value Indication

  // Locally-generated event messages
  attFlowCtrlViolatedEvt_t flowCtrlEvt;       //!< ATT Flow Control Violated Event
  attMtuUpdatedEvt_t mtuEvt;                  //!< ATT MTU Updated Event
} attMsg_t;

/** @} End ATT_GATT_Structs */

/// @cond NODOC

// Function prototype to notify GATT Server or Client about an outgoing ATT message.
typedef void (*attNotifyTxCB_t)( uint16_t connHandle, uint8_t opcode );

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * API FUNCTIONS
 */

/*-------------------------------------------------------------------
 *  General Utility APIs
 */

/*
 * Parse an attribute protocol message.
 */
//extern uint8_t ATT_ParsePacket( l2capDataEvent_t *pL2capMsg, attPacket_t *pPkt );

/*
 * Compare two UUIDs. The UUIDs are converted if necessary.
 */
extern uint8_t ATT_CompareUUID( const uint8_t *pUUID1, uint16_t len1,
                              const uint8_t *pUUID2, uint16_t len2 );
/*
 * Convert a 16-bit UUID to 128-bit UUID.
 */
extern uint8_t ATT_ConvertUUIDto128( const uint8_t *pUUID16, uint8_t *pUUID128 );

/*
 * Convert a 128-bit UUID to 16-bit UUID.
 */
extern uint8_t ATT_ConvertUUIDto16( const uint8_t *pUUID128, uint8_t *pUUID16 );


/*-------------------------------------------------------------------
 *  Attribute Client Utility APIs
 */

/*
 * Build Error Response.
 */
extern uint16_t ATT_BuildErrorRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Error Response.
 */
extern bStatus_t ATT_ParseErrorRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Exchange MTU Request.
 */
extern uint16_t ATT_BuildExchangeMTUReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Exchange MTU Response.
 */
extern uint16_t ATT_BuildExchangeMTURsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Exchange MTU Response.
 */
extern bStatus_t ATT_ParseExchangeMTURsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Find Information Request.
 */
extern uint16_t ATT_BuildFindInfoReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Find Information Response.
 */
extern bStatus_t ATT_ParseFindInfoRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Find Information Response.
 */
extern uint16_t ATT_BuildFindInfoRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Find By Type Value Request.
 */
extern uint16_t ATT_BuildFindByTypeValueReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Find By Type Value Response.
 */
extern uint16_t ATT_BuildFindByTypeValueRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Find By Type Value Response.
 */
extern bStatus_t ATT_ParseFindByTypeValueRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Read By Type Request.
 */
extern uint16_t ATT_BuildReadByTypeReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Read By Type Response.
 */
extern uint16_t ATT_BuildReadByTypeRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Read By Type Response.
 */
extern bStatus_t ATT_ParseReadByTypeRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Read Request.
 */
extern uint16_t ATT_BuildReadReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Read Response.
 */
extern uint16_t ATT_BuildReadRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Read Response.
 */
extern bStatus_t ATT_ParseReadRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Read Blob Request.
 */
extern uint16_t ATT_BuildReadBlobReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Read Blob Response.
 */
extern uint16_t ATT_BuildReadBlobRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Read Blob Response.
 */
extern bStatus_t ATT_ParseReadBlobRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Read Multiple Request.
 */
extern uint16_t ATT_BuildReadMultiReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Read Multiple Response.
 */
extern uint16_t ATT_BuildReadMultiRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Read Multiple Response.
 */
extern bStatus_t ATT_ParseReadMultiRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Read By Group Type Response.
 */
extern uint16_t ATT_BuildReadByGrpTypeRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Read By Group Type Response.
 */
extern bStatus_t ATT_ParseReadByGrpTypeRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Write Request.
 */
extern uint16_t ATT_BuildWriteReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Write Response.
 */
extern bStatus_t ATT_ParseWriteRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Prepare Write Request.
 */
extern uint16_t ATT_BuildPrepareWriteReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Build Prepare Write Response.
 */
extern uint16_t ATT_BuildPrepareWriteRsp( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Prepare Write Response.
 */
extern bStatus_t ATT_ParsePrepareWriteRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Execute Write Request.
 */
extern uint16_t ATT_BuildExecuteWriteReq( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Execute Write Response.
 */
extern bStatus_t ATT_ParseExecuteWriteRsp( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Build Handle Value Indication.
 */
extern uint16_t ATT_BuildHandleValueInd( uint8_t *pBuf, uint8_t *pMsg );

/*
 * Parse Handle Value Indication.
 */
extern bStatus_t ATT_ParseHandleValueInd( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );


/*-------------------------------------------------------------------
 *  Attribute Server Utility APIs
 */

/*
 * Parse Exchange MTU Request.
 */
extern bStatus_t ATT_ParseExchangeMTUReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Find Information Request.
 */
extern bStatus_t ATT_ParseFindInfoReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Find By Type Value Request.
 */
extern bStatus_t ATT_ParseFindByTypeValueReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Read By Type Request.
 */
extern bStatus_t ATT_ParseReadByTypeReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Read Request.
 */
extern bStatus_t ATT_ParseReadReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Write Blob Request.
 */
extern bStatus_t ATT_ParseReadBlobReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Read Multiple Request.
 */
extern bStatus_t ATT_ParseReadMultiReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Write Request.
 */
extern bStatus_t ATT_ParseWriteReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Execute Write Request.
 */
extern bStatus_t ATT_ParseExecuteWriteReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Prepare Write Request.
 */
extern bStatus_t ATT_ParsePrepareWriteReq( uint8_t sig, uint8_t cmd, uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/*
 * Parse Handle Value Confirmation.
 */
extern bStatus_t ATT_ParseHandleValueCfm( uint8_t *pParams, uint16_t len, attMsg_t *pMsg );

/// @endcond // NODOC

/*-------------------------------------------------------------------
 *  Attribute Client Public APIs
 */

/**
 * @defgroup ATT_APIs ATT API Functions
 *
 * @{
 */

/**
 * @brief   Send Exchange MTU Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ExchangeMTUReq( uint16_t connHandle, attExchangeMTUReq_t *pReq );

/**
 * @brief   Send Find Information Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_FindInfoReq( uint16_t connHandle, attFindInfoReq_t *pReq );

/**
 * @brief   Send Find By Type Value Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 *          @ref INVALIDPARAMETER : Invalid request field.
 *          @ref MSG_BUFFER_NOT_AVAIL
 *          @ref bleNotConnected
 *          @ref bleMemAllocError
 */
extern bStatus_t ATT_FindByTypeValueReq( uint16_t connHandle, attFindByTypeValueReq_t *pReq );

/**
 * @brief   Send Read By Type Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadByTypeReq( uint16_t connHandle, attReadByTypeReq_t *pReq );

/**
 * @brief   Send Read Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadReq( uint16_t connHandle, attReadReq_t *pReq );

/**
 * @brief   Send Read Blob Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadBlobReq( uint16_t connHandle, attReadBlobReq_t *pReq );

/**
 * @brief   Send Read Multiple Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadMultiReq( uint16_t connHandle, attReadMultiReq_t *pReq );

/**
 * @brief   Send Read By Group Type Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadByGrpTypeReq( uint16_t connHandle, attReadByGrpTypeReq_t *pReq );

/**
 * @brief   Send Write Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 * @return  bleLinkEncrypted: Connection is already encrypted.
 */
extern bStatus_t ATT_WriteReq( uint16_t connHandle, attWriteReq_t *pReq );

/**
 * @brief   Send Prepare Write Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_PrepareWriteReq( uint16_t connHandle, attPrepareWriteReq_t *pReq );

/**
 * @brief   Send Execute Write Request.
 *
 * @param   connHandle - connection to use
 * @param   pReq - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Request was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid request field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ExecuteWriteReq( uint16_t connHandle, attExecuteWriteReq_t *pReq );

/**
 * @brief   Send Handle Value Confirmation.
 *
 * @param   connHandle - connection to use
 *
 * @return  @ref bleSUCCESS : Confirmation was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid confirmation field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_HandleValueCfm( uint16_t connHandle );


/*-------------------------------------------------------------------
 *  Attribute Server Public APIs
 */

/**
 * @brief   Send Error Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to error response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ErrorRsp( uint16_t connHandle, attErrorRsp_t *pRsp );

/**
 * @brief   Send Exchange MTU Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to request to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ExchangeMTURsp( uint16_t connHandle, attExchangeMTURsp_t *pRsp );

/**
 * @brief   Send Find Information Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_FindInfoRsp( uint16_t connHandle, attFindInfoRsp_t *pRsp );

/**
 * @brief   Send Find By Type Value Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_FindByTypeValueRsp( uint16_t connHandle, attFindByTypeValueRsp_t *pRsp );

/**
 * @brief   Send Read By Type Respond.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadByTypeRsp( uint16_t connHandle, attReadByTypeRsp_t *pRsp );

/**
 * @brief   Send Read Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadRsp( uint16_t connHandle, attReadRsp_t *pRsp );

/**
 * @brief   Send Read Blob Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadBlobRsp( uint16_t connHandle, attReadBlobRsp_t *pRsp );

/**
 * @brief   Send Read Multiple Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadMultiRsp( uint16_t connHandle, attReadMultiRsp_t *pRsp ) ;

/**
 * @brief   Send Read By Group Type Respond.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ReadByGrpTypeRsp( uint16_t connHandle, attReadByGrpTypeRsp_t *pRsp );

/**
 * @brief   Send Write Response.
 *
 * @param   connHandle - connection to use
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_WriteRsp( uint16_t connHandle );

/**
 * @brief   Send Prepare Write Response.
 *
 * @param   connHandle - connection to use
 * @param   pRsp - pointer to response to be sent
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_PrepareWriteRsp( uint16_t connHandle, attPrepareWriteRsp_t *pRsp );

/**
 * @brief   Send Execute Write Response.
 *
 * @param   connHandle - connection to use
 *
 * @return  @ref bleSUCCESS : Response was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid response field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_ExecuteWriteRsp( uint16_t connHandle );

/**
 * @brief   Send Handle Value Notification.
 *
 * @param   connHandle - connection to use
 * @param   pNoti - pointer to notification to be sent
 *
 * @return  @ref bleSUCCESS : Notification was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid notification field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_HandleValueNoti( uint16_t connHandle, attHandleValueNoti_t *pNoti );

/**
 * @brief   Send Handle Value Indication.
 *
 * @param   connHandle - connection to use
 * @param   pInd - pointer to indication to be sent
 *
 * @return  @ref bleSUCCESS : Indication was sent successfully.
 * @return  @ref INVALIDPARAMETER : Invalid indication field.
 * @return  @ref MSG_BUFFER_NOT_AVAIL
 * @return  @ref bleNotConnected
 * @return  @ref bleMemAllocError
 */
extern bStatus_t ATT_HandleValueInd( uint16_t connHandle, attHandleValueInd_t *pInd );

/*-------------------------------------------------------------------
 *  Attribute Common Public APIs
 */

/**
 * @brief   This function is used to update the MTU size of a connection.
 *
 * @param   connHandle - connection handle.
 * @param   MTU - new MTU.
 *
 * @return  @ref bleSUCCESS : MTU was updated successfully.
 * @return  @ref bleFAILURE : MTU wasn't updated.
 */
extern uint8_t ATT_UpdateMTU( uint16_t connHandle, uint16_t MTU );

/**
 * @brief   This function is used to get the MTU size of a connection.
 *
 * @param   connHandle - connection handle.
 *
 * @return  connection MTU size.
 */
extern uint16_t ATT_GetMTU( uint16_t connHandle );

/**
 * @brief   Set a ATT Parameter value.  Use this function to change
 *          the default ATT parameter values.
 *
 * @param   value - new param value
 */
extern void ATT_SetParamValue( uint16_t value );

/**
 * @brief   Get a ATT Parameter value.
 *
 * @return  ATT Parameter value
 */
extern uint16_t ATT_GetParamValue( void );

/**
 * @brief   Register the server's notify Tx function with the ATT layer.
 *
 * @param   pfnNotifyTx - pointer to notify Tx function
 */
extern void ATT_RegisterServer( attNotifyTxCB_t pfnNotifyTx );

/**
 * @brief   Register the client's notify Tx function with the ATT layer.
 *
 * @param   pfnNotifyTx - pointer to notify Tx function
 */
extern void ATT_RegisterClient( attNotifyTxCB_t pfnNotifyTx );

/** @} End ATT_APIs */

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ATT_H */

/** @} End ATT_GATT */
