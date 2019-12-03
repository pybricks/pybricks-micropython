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
 *  @defgroup GapScan Gap Scanner
 *  @brief This module implements the Host Scanner
 *  @{
 *  @file  gap_scanner.h
 *  @brief      GAP Scanner layer interface
 */

#ifndef GAP_SCANNER_H
#define GAP_SCANNER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------
 * INCLUDES
 */
#include "bcomdef.h"
#include "gap.h"

/*-------------------------------------------------------------------
 * MACROS
 */

// Extended Advertising Primary PHY
// TI Enhancement - Use as the MSB of the PHY parameter!
#define AE_PHY_NONE                                         0x00
#define AE_PHY_1_MBPS                                       0x01
#define AE_PHY_2_MBPS                                       0x02
#define AE_PHY_CODED                                        0x03
#define AE_PHY_CODED_S8                                     0x03
#define AE_PHY_CODED_S2                                     0x83

#define AE_EVT_TYPE_CONNECTABLE_ADVERTISING                 0
#define AE_EVT_TYPE_SCANNABLE_ADVERTISING                   1
#define AE_EVT_TYPE_DIRECTED_ADVERTISING                    2
#define AE_EVT_TYPE_SCAN_RESPONSE                           3
#define AE_EVT_TYPE_LEGACY_PDU                              4
//
#define AE_EVT_TYPE_CONN_ADV                                BV(AE_EVT_TYPE_CONNECTABLE_ADVERTISING)
#define AE_EVT_TYPE_SCAN_ADV                                BV(AE_EVT_TYPE_SCANNABLE_ADVERTISING)
#define AE_EVT_TYPE_DIR_ADV                                 BV(AE_EVT_TYPE_DIRECTED_ADVERTISING)
#define AE_EVT_TYPE_SCAN_RSP                                BV(AE_EVT_TYPE_SCAN_RESPONSE)
#define AE_EVT_TYPE_LEGACY                                  BV(AE_EVT_TYPE_LEGACY_PDU)

#define AE_EVT_TYPE_COMPLETE_MASK                           (~(3 << 5))
#define AE_EVT_TYPE_COMPLETE                                (0 << 5)
#define AE_EVT_TYPE_INCOMPLETE_MORE_TO_COME                 (1 << 5)
#define AE_EVT_TYPE_INCOMPLETE_NO_MORE_TO_COME              (2 << 5)
#define AE_EVT_TYPE_RFU                                     (3 << 5)


/*******************************************************************************
 * Gap Scanner Internal API
 */

/*-------------------------------------------------------------------
 * CONSTANTS
 */

/**
 * @defgroup GapScan_Events GapScan Events
 *
 * Events sent from the GapScan module
 *
 * See @ref GapAdvScan_Event_IDs for the following events which are related to
 * this module:
 * - @ref GAP_EVT_SCAN_ENABLED
 * - @ref GAP_EVT_SCAN_DISABLED
 * - @ref GAP_EVT_SCAN_PRD_ENDED
 * - @ref GAP_EVT_SCAN_DUR_ENDED
 * - @ref GAP_EVT_SCAN_INT_ENDED
 * - @ref GAP_EVT_SCAN_WND_ENDED
 * - @ref GAP_EVT_ADV_REPORT
 * - @ref GAP_EVT_ADV_REPORT_FULL
 * - @ref GAP_EVT_INSUFFICIENT_MEMORY
 *
 * These are set with @ref GapScan_setParam and read with
 * @ref GapScan_getParam
 *
 * @{
 */
/** @} End GapScan_Events */

/**
 * @defgroup GapScan_Callbacks GapScan Callbacks
 *
 * Callbacks used in the GapScan module
 *
 * See @ref pfnGapCB_t for the callbacks used in this module.
 *
 * @{
 */
/** @} End GapScan_Callbacks */

/**
 * @defgroup GapScan_Constants GapScan Constants
 * Other defines used in the GapScan module
 * @{
 */

/// Advertising report event types
enum GapScan_AdvRptTypeNStatus_t {
  /// Connectable
  ADV_RPT_EVT_TYPE_CONNECTABLE = AE_EVT_TYPE_CONN_ADV,
  /// Scannable
  ADV_RPT_EVT_TYPE_SCANNABLE   = AE_EVT_TYPE_SCAN_ADV,
  /// Directed
  ADV_RPT_EVT_TYPE_DIRECTED    = AE_EVT_TYPE_DIR_ADV,
  /// Scan Response
  ADV_RPT_EVT_TYPE_SCAN_RSP    = AE_EVT_TYPE_SCAN_RSP,
  /// Legacy
  ADV_RPT_EVT_TYPE_LEGACY      = AE_EVT_TYPE_LEGACY,
  /// Complete
  ADV_RPT_EVT_STATUS_COMPLETE  = AE_EVT_TYPE_COMPLETE,
  /// More Data To Come
  ADV_RPT_EVT_STATUS_MORE_DATA = AE_EVT_TYPE_INCOMPLETE_MORE_TO_COME,
  /// Truncated
  ADV_RPT_EVT_STATUS_TRUNCATED = AE_EVT_TYPE_INCOMPLETE_NO_MORE_TO_COME,
  /// Reserved for Future Use
  ADV_RPT_EVT_STATUS_RFU       = AE_EVT_TYPE_RFU
};

/// GAP Scanner Primary PHY
enum GapScan_PrimPhy_t {
  SCAN_PRIM_PHY_1M    = 0x01,  //!< Scan on the 1M PHY
  SCAN_PRIM_PHY_CODED = 0x04    //!< Scan on the Coded PHY
};

/// PDU Types for PDU Type Filter
enum GapScan_FilterPduType_t {
  /// Non-connectable only. Mutually exclusive with SCAN_FLT_PDU_CONNECTABLE_ONLY
  SCAN_FLT_PDU_NONCONNECTABLE_ONLY = ADV_RPT_EVT_TYPE_CONNECTABLE,
  /// Connectable only. Mutually exclusive with SCAN_FLT_PDU_NONCONNECTABLE_ONLY
  SCAN_FLT_PDU_CONNECTABLE_ONLY    = ADV_RPT_EVT_TYPE_CONNECTABLE << 1,
  /// Non-scannable only. Mutually exclusive with SCAN_FLT_PDU_SCANNABLE_ONLY
  SCAN_FLT_PDU_NONSCANNABLE_ONLY   = ADV_RPT_EVT_TYPE_SCANNABLE << 1,
   /// Scannable only. Mutually exclusive with SCAN_FLT_PDU_NONSCANNABLE_ONLY
  SCAN_FLT_PDU_SCANNABLE_ONLY      = ADV_RPT_EVT_TYPE_SCANNABLE << 2,
  /// Undirected only. Mutually exclusive with SCAN_FLT_PDU_DIRECTIED_ONLY
  SCAN_FLT_PDU_UNDIRECTED_ONLY     = ADV_RPT_EVT_TYPE_DIRECTED << 2,
  /// Directed only. Mutually exclusive with SCAN_FLT_PDU_UNDIRECTED_ONLY
  SCAN_FLT_PDU_DIRECTED_ONLY       = ADV_RPT_EVT_TYPE_DIRECTED << 3,
  /// Advertisement only. Mutually exclusive with SCAN_FLT_PDU_SCANRSP_ONLY
  SCAN_FLT_PDU_ADV_ONLY            = ADV_RPT_EVT_TYPE_SCAN_RSP << 3,
  /// Scan Response only. Mutually exclusive with SCAN_FLT_PDU_ADV_ONLY
  SCAN_FLT_PDU_SCANRSP_ONLY        = ADV_RPT_EVT_TYPE_SCAN_RSP << 4,
  /// Extended only. Mutually exclusive with SCAN_FLT_PDU_LEGACY_ONLY
  SCAN_FLT_PDU_EXTENDED_ONLY       = ADV_RPT_EVT_TYPE_LEGACY << 4,
  /// Legacy only. Mutually exclusive with SCAN_FLT_PDU_EXTENDED_ONLY
  SCAN_FLT_PDU_LEGACY_ONLY         = ADV_RPT_EVT_TYPE_LEGACY << 5,
  /// Truncated only. Mutually exclusive with SCAN_FLT_PDU_COMPLETE_ONLY
  SCAN_FLT_PDU_TRUNCATED_ONLY      = BV(10),
  /// Complete only. Mutually exclusive with SCAN_FLT_PDU_TRUNCATED_ONLY
  SCAN_FLT_PDU_COMPLETE_ONLY       = BV(11)
};

/// Fields of Adv Report
enum GapScan_AdvRptField_t {
  SCAN_ADVRPT_FLD_EVENTTYPE      = BV( 0), //!< eventType field
  SCAN_ADVRPT_FLD_ADDRTYPE       = BV( 1), //!< addrType field
  SCAN_ADVRPT_FLD_ADDRESS        = BV( 2), //!< address field
  SCAN_ADVRPT_FLD_PRIMPHY        = BV( 3), //!< primPhy field
  SCAN_ADVRPT_FLD_SECPHY         = BV( 4), //!< secPhy field
  SCAN_ADVRPT_FLD_ADVSID         = BV( 5), //!< advSid field
  SCAN_ADVRPT_FLD_TXPOWER        = BV( 6), //!< txPower field
  SCAN_ADVRPT_FLD_RSSI           = BV( 7), //!< RSSI field
  SCAN_ADVRPT_FLD_DIRADDRTYPE    = BV( 8), //!< dirAddrType field
  SCAN_ADVRPT_FLD_DIRADDRESS     = BV( 9), //!< dirAddress field
  SCAN_ADVRPT_FLD_PRDADVINTERVAL = BV(10), //!< prdAdvInterval field
  SCAN_ADVRPT_FLD_DATALEN        = BV(11), //!< dataLen field
};

/// Reason for @ref GapScan_Evt_End_t
enum GapScan_EndReason_t {
  SCAN_END_REASON_USR_REQ = 0, //!< Scanning ended by user request
  SCAN_END_REASON_DUR_EXP      //!< Scanning ended by duration expiration
};

/**
 * @defgroup GAPScan_RSSI GAPScan RSSI Values
 * @{
 */
#define SCAN_RSSI_MAX                      127  //!< Maximum RSSI value. 127 dBm
#define SCAN_RSSI_MIN                      -128 //!< Minimum RSSI value. -128 dBm
/** @} End GAPScan_RSSI */

/**
 * @defgroup GAPScan_Filter_Min_RSSI GAPScan Filter Minimum RSSI
 * @{
 */
#define SCAN_FLT_RSSI_ALL         SCAN_RSSI_MIN //!< -128 dBm meaning 'accept all'
#define SCAN_FLT_RSSI_NONE        SCAN_RSSI_MAX //!< 127 dBm meaning 'don't accept any'
/** @} End GAPScan_Filter_Min_RSSI */

/**
 * @defgroup GAPScan_Default_Param_Value GAPScan Default Parameter Value
 * @{
 */
/// Default scan interval (in 625 us ticks)
#define SCAN_PARAM_DFLT_INTERVAL           800
/// Default scan window   (in 625 us ticks)
#define SCAN_PARAM_DFLT_WINDOW             800
/// Default scan type
#define SCAN_PARAM_DFLT_TYPE               SCAN_TYPE_PASSIVE
/// Default adv report fields to record
#define SCAN_PARAM_DFLT_RPT_FIELDS         (SCAN_ADVRPT_FLD_EVENTTYPE|\
                                            SCAN_ADVRPT_FLD_ADDRTYPE |\
                                            SCAN_ADVRPT_FLD_ADDRESS  |\
                                            SCAN_ADVRPT_FLD_ADVSID   |\
                                            SCAN_ADVRPT_FLD_RSSI)
/// Default phys
#define SCAN_PARAM_DFLT_PHYS               SCAN_PRIM_PHY_1M
/// Default filter policy
#define SCAN_PARAM_DFLT_FLT_POLICY         SCAN_FLT_POLICY_ALL
/// Default setting for PDU type filter
#define SCAN_PARAM_DFLT_FLT_PDU            SCAN_FLT_PDU_COMPLETE_ONLY
/// Default setting for minimum RSSI filter
#define SCAN_PARAM_DFLT_FLT_RSSI           SCAN_FLT_RSSI_ALL
/// Default setting for discoverable mode filter
#define SCAN_PARAM_DFLT_FLT_DISC           SCAN_FLT_DISC_DISABLE
/// Default setting for duplicate filter
#define SCAN_PARAM_DFLT_FLT_DUP            SCAN_FLT_DUP_ENABLE
/// Default advertising report list size
#define SCAN_PARAM_DFLT_NUM_ADV_RPT        0
/** @} End GAPScan_Default_Param_Value */

/** @} End GapScan_Constants */

/**
 * @defgroup GapScan_Params GapScan Params
 * Params used in the GapAdv module
 * @{
 */

/**
 * GAP Scanner Parameters
 *
 * These can be set with @ref GapScan_setParam and read with
 * @ref GapScan_getParam. The default values below refer to the values that are
 * set at initialization.
 */
typedef enum
{
  /**
   * Advertising Report Fields (R/W)
   *
   * Bit mask of which fields in the @ref GapScan_Evt_AdvRpt_t need to be stored
   * in the AdvRptList
   *
   * @note Change of this parameter shall not be attempted while scanning is
   * active
   *
   * size: uint16_t
   *
   * default: @ref SCAN_PARAM_DFLT_RPT_FIELDS
   *
   * range: combination of individual values in @ref GapScan_AdvRptField_t
   */
  SCAN_PARAM_RPT_FIELDS,

  /**
   * Primary Scanning Channel PHYs (R/W)
   *
   * PHYs on which advertisements should be received on the primary
   * advertising channel.
   *
   * @note Change of this parameter will not affect an ongoing scanning.
   * If changed during scanning, it will take effect when the scanning is
   * re-enabled after disabled.
   *
   * size: uint8_t
   *
   * default: @ref SCAN_PARAM_DFLT_PHYS
   *
   * range: @ref GapScan_PrimPhy_t
   */
  SCAN_PARAM_PRIM_PHYS,

  /**
   * Scanning Filter Policy (R/W)
   *
   * Policy of how to apply white list to filter out unwanted packets
   *
   * @note Change of this parameter will not affect an ongoing scanning.
   * If changed during scanning, it will take effect when the scanning is
   * re-enabled after disabled.
   *
   * size: uint8_t
   *
   * default: @ref SCAN_PARAM_DFLT_FLT_POLICY
   *
   * range: @ref GapScan_FilterPolicy_t
   */
  SCAN_PARAM_FLT_POLICY,

  /**
   * Filter by PDU Type (R/W)
   *
   * This filter value specifies packets of which types in 6 different category
   * sets are wanted. The 6 sets include Connectable/Non-Connectable,
   * Scannable/Non-Scannable, Directed/Undirected, ScanRsp/Adv, Legacy/Extended,
   * and Complete/Incomplete category sets. Each set has two mutually exclusive
   * types. Only one type in a set can be* chosen. For example,
   * @ref SCAN_FLT_PDU_NONSCANNABLE_ONLY and @ref SCAN_FLT_PDU_SCANNABLE_ONLY
   * cannot be chosen together. Only either one can be used. If neither type is
   * selected in a set, the filter will not care about the category. For example,
   * if neither @ref SCAN_FLT_PDU_NONCONNECTABLE_ONLY nor
   * @ref SCAN_FLT_PDU_CONNECTABLE_ONLY is set in the parameter value, the
   * scanner will accept both connectable packets and non-connectable packets.
   *
   * @par Usage example:
   * \code{.c}
   * GapScan_setParam(SCAN_PARAM_FLT_PDU_TYPE,
   *                  SCAN_FLT_PDU_CONNECTABLE_ONLY |
   *                  SCAN_FLT_PDU_UNDIRECTED_ONLY |
   *                  SCAN_FLT_PDU_EXTENDED_ONLY |
   *                  SCAN_FLT_PDU_COMPLETE_ONLY);
   * \endcode
   *
   * With the call above, the scanner will receive only connectable,
   * undirected, extended(non-legacy), and complete packets. It will not be
   * examined whether the packets are scannable or non-scannable and whether
   * they are scan responses or advertisements.
   *
   * size: uint16_t
   *
   * default: @ref SCAN_PARAM_DFLT_FLT_PDU
   *
   * range: combination of individual values in @ref GapScan_FilterPduType_t
   */
  SCAN_PARAM_FLT_PDU_TYPE,

  /**
   * Filter by Minimum RSSI (R/W)
   *
   * Only packets received with the specified RSSI or above will be accepted.
   * @note -128 means to ignore RSSI (to not filter out any packet by RSSI)
   *
   * size: int8_t
   *
   * default: -128 (ignore)
   *
   * range: -128 - 127 (dBm)
   */
  SCAN_PARAM_FLT_MIN_RSSI,

  /**
   * Filter by Discoverable Mode (R/W)
   *
   * @note Change of this parameter shall not be attempted while scanning is
   * active
   *
   * size: uint8_t
   *
   * default: @ref SCAN_PARAM_DFLT_FLT_DISC
   *
   * range: @ref GapScan_FilterDiscMode_t
   */
  SCAN_PARAM_FLT_DISC_MODE,

  /**
   * Duplicate Filtering (R/W)
   *
   * Specify whether and when to filter out duplicated packets
   * by DID and address
   *
   * @note Change of this parameter will not affect an ongoing scanning.
   * If changed during scanning, it will take effect when the scanning is
   * re-enabled after disabled.
   *
   * size: uint8_t
   *
   * default: @ref SCAN_PARAM_DFLT_FLT_DUP
   *
   * range: @ref GapScan_FilterDuplicate_t
   */
  SCAN_PARAM_FLT_DUP,

/// @cond NODOC
  SCAN_NUM_RW_PARAM,
/// @endcond // NODOC

  /**
   * Number of Reports Recorded (R)
   *
   * Number of advertising reports recorded since the current scanning started.
   * @note This number will be the same as numReport that will come with
   * @ref GAP_EVT_SCAN_DISABLED when the scanning has ended.
   *
   * size: uint8_t
   *
   * range: 0 up to maxNumReport specified when @ref GapScan_enable() is called
   */
  SCAN_PARAM_NUM_ADV_RPT = SCAN_NUM_RW_PARAM,

/// @cond NODOC
  SCAN_NUM_PARAM
/// @endcond //NODOC
} GapScan_ParamId_t;
/** @} End GapScan_Params */

/*-------------------------------------------------------------------
 * Structures
 */

/**
 * @defgroup GapScan_Structs GapScan Structures
 * Data structures used in the GapScan module
 * @{
 */

/// Scanned PHY
typedef enum
{
  SCANNED_PHY_NONE  = 0,               //!< No PHY is used
  SCANNED_PHY_1M    = AE_PHY_1_MBPS,   //!< Scanned on the 1M PHY
  SCANNED_PHY_2M    = AE_PHY_2_MBPS,   //!< Scanned on the 2M PHY
  SCANNED_PHY_CODED = AE_PHY_CODED     //!< Scanned on the Coded PHY
} GapScan_ScannedPhy_t;

/// GAP Scanner Scan Type
typedef enum {
  SCAN_TYPE_PASSIVE = 0, //!< Scan for non-scannable advertisements
  SCAN_TYPE_ACTIVE  = 1   //!< Scan for scannable advertisements
} GapScan_ScanType_t;

/// GAP Scanner Filter Policy
typedef enum {
  /**
   * Accept all advertising packets except directed advertising packets
   * not addressed to this device.
   */
  SCAN_FLT_POLICY_ALL     = 0,
  /**
   * Accept only advertising packets from devices where the advertiser's address
   * is in the whitelist. Directed advertising packets which are not addressed
   * to this device shall be ignored.
   */
  SCAN_FLT_POLICY_WL      = 1,
  /**
   * Accept all advertising packets except directed advertising packets where
   * the TargetA does not addrress this device. Note that directed advdertising
   * packets where the TargetA is a resolvable private address that cannot be
   * resolved are also accepted.
   */
  SCAN_FLT_POLICY_ALL_RPA = 2,
  /**
   * Accept all advertising packets except advertising packets where the
   * advertiser's identity address is not in the whitelist and directed
   * advertising packets where the TargetA does not address this device.
   * Note that directed advertising packets where the TargetA is a resolvable
   * private address that cannot be resolved are also accepted.
   */
  SCAN_FLT_POLICY_WL_RPA  = 3
} GapScan_FilterPolicy_t;

/// Choices for GAP Scanner Discoverable Mode Filter
typedef enum {
  SCAN_FLT_DISC_NONE    = 0, //!< Non-discoverable mode
  SCAN_FLT_DISC_GENERAL = 1, //!< General discoverable mode
  SCAN_FLT_DISC_LIMITED = 2, //!< Limited discoverable mode
  SCAN_FLT_DISC_ALL     = 3, //!< General or Limited discoverable mode
  SCAN_FLT_DISC_DISABLE = 4  //!< Disable discoverable mode filter
} GapScan_FilterDiscMode_t;

/// Choices for GAP Scanner Duplicate Filter
typedef enum {
  SCAN_FLT_DUP_DISABLE = 0, //!< Duplicate filtering disabled
  SCAN_FLT_DUP_ENABLE  = 1, //!< Duplicate filtering enabled
  /// Duplicate filtering enabled, reset for each scan period
  SCAN_FLT_DUP_RESET   = 2
} GapScan_FilterDuplicate_t;

/// Event for scanning end
typedef struct {
  uint8_t    reason;    //!< End reason - @ref GapScan_EndReason_t
  uint8_t    numReport; //!< Number of recorded advertising reports
} GapScan_Evt_End_t;

/// Event for advertising report
typedef struct {
  /**
   * Bits 0 to 4 indicate connectable, scannable, directed, scan response, and
   * legacy respectively
   */
  uint8_t  evtType;
  /// Public, random, public ID, random ID, or anonymous
  GAP_Addr_Types_t addrType;
  /// Address of the advertising device
  uint8_t  addr[B_ADDR_LEN];
  /// PHY of the primary advertising channel
  GapScan_ScannedPhy_t primPhy;
  /// PHY of the secondary advertising channel
  GapScan_ScannedPhy_t secPhy;
  /// SID (0x00-0x0f) of the advertising PDU. 0xFF means no ADI field in the PDU
  uint8_t  advSid;
  /// -127 dBm <= TX power <= 126 dBm
  int8_t   txPower;
  /// -127 dBm <= RSSI <= 20 dBm
  int8_t   rssi;
  /// Type of TargetA address in the directed advertising PDU
  GAP_Addr_Types_t directAddrType;
  /// TargetA address
  uint8_t  directAddr[B_ADDR_LEN];
  /// Periodic advertising interval. 0 means no periodic advertising.
  uint16_t periodicAdvInt;
  /// Length of the data
  uint16_t dataLen;
  /// Pointer to advertising or scan response data
  uint8_t  *pData;
} GapScan_Evt_AdvRpt_t;

/// Event Mask
typedef uint32_t GapScan_EventMask_t;

/** @} End GapScam_Structs */

/*-------------------------------------------------------------------
 * API's
 */

/**
 * Set parameters dependent on PHY.
 *
 * @note that if primPhys contains more than one PHY, the same parameters of
 * those PHYs will be set with the same values.
 *
 * @note Change of the parameters will not affect an ongoing scanning.
 * If changed during scanning, it will take effect when the scanning is
 * re-enabled after disabled.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref bleInvalidRange
 *
 * @param primPhys Primary advertising channel PHY(s). Individual values of
 *         @ref GapScan_PrimPhy_t can be OR'ed.
 * @param type Scanning type
 * @param interval Scanning interval. This shall be equal to or greater than
 *        window
 * @param window Scanning window
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_setPhyParams(uint8_t primPhys, GapScan_ScanType_t type,
                              uint16_t interval, uint16_t window);

/**
 * Get parameters of the specified PHY.
 *
 * @note that primPhy shall indicate only one PHY.
 *
 * The host will send the CommandStatus Event to return the parameter value
 * in question.
 * The following can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref bleInvalidRange
 * @ref INVALIDPARAMETER
 *
 * The following parameters can be returned from the CommandStatus Event:
 * pType - type value
 * pInterval - scanning interval
 * pWindow pointer - scanning window value
 *
 * @param primPhy Primary advertising channel PHY.Shall be one from
 *        @ref GapScan_PrimPhy_t.

 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_getPhyParams(uint8_t primPhy);

/**
 * Set a parameter.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @return @ref SUCCESS
 * @return @ref bleInvalidRange
 * @return @ref INVALIDPARAMETER
 * @return @ref bleIncorrectMode
 *
 * @param paramId parameter ID
 * @param pValue pointer to the value to set the parameter with
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_setParam(GapScan_ParamId_t paramId, void* pValue);

/**
 * Get a parameter.
 *
 * The host will send the CommandStatus Event to return the parameter value
 * in question.
 * The following can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref bleInvalidRange
 * @ref INVALIDPARAMETER
 *
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_getParam(GapScan_ParamId_t paramId);

/**
 * Set which events to receive through the callback.
 *
 * One bit per event. If a bit is set to 1, the callback provided by
 * @ref GapScan_registerCb will be called upon corresponding event.
 *
 * When this command is received, the host will send the CommandStatus Event.
 *
 * @param eventMask bit mask of the events
 */
HCI_StatusCodes_t GapScan_setEventMask(GapScan_EventMask_t eventMask);

/**
 * Start scanning.
 *
 * If duration is zero or both the duration and period are non-zero, the scanner
 * will continue scanning until @ref GapScan_disable is called. If period is zero
 * and duration is non-zero, the scanner will continue scanning until duration
 * has expired or @ref GapScan_disable is called.
 *
 * The host will send the CommandStatus Event to indicate the scan has started
 * The following status values can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref bleNotReady
 * @ref bleInvalidRange
 * @ref bleMemAllocError
 * @ref bleAlreadyInRequestedMode
 * @ref bleIncorrectMode
 *
 * @param period scan period. ignored if duration is zero. 1.28 sec unit
 * @param duration scan duration. 10 ms unit. The time of duration shall be
 *        greater than the time of scan interval set by @ref GapScan_setPhyParams.
 * @param maxNumReport If non-zero, the list of advertising reports (the number
 *        of which is up to maxNumReport) will be generated and come with
 *        @ref GAP_EVT_SCAN_DISABLED.
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_Enable(uint16_t period, uint16_t duration,
                    uint8_t maxNumReport);

/**
 * Stop currently running scanning operation.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref FAILURE
 * @ref bleIncorrectMode
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_disable(void);

/**
 * Get a specific advertising report from the advertising report list.
 *
 * Only the fields specified by @ref GapScan_setParam (@ref SCAN_PARAM_RPT_FIELDS,
 * etc) will be filled out. All other fields will be filled with 0's.
 *
 * The host will send the CommandStatus Event to return the advertising report.
 * The following status values can be received from the CommandStatus Event:
 * @ref SUCCESS
 * @ref bleInvalidRange
 * @ref INVALIDPARAMETER
 *
 * @param rptIdx Index of the advertising report in the list
 *
 * @return @ref SUCCESS : command sent successfully over HCI transport layer
 * @return @ref FAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapScan_getAdvReport(uint8_t rptIdx);

/*-------------------------------------------------------------------
-------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* GAP_SCANNER_H */

/** @} End GapScan */

