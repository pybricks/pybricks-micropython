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
 *  @defgroup GapInit Gap Initiator
 *  @brief This module implements the Host Initiator
 *  @{
 *  @file  gap_initiator.h
 *  @brief      GAP Initiator layer interface
 */

#ifndef GAP_INITIATOR_H
#define GAP_INITIATOR_H

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
 * CONSTANTS
 */

/**
 * @defgroup GapInit_Constants GapInit Constants
 *
 * Other defines used in the GapInit module
 * @{
 */

/// GAP Initiator Initiating PHYs
enum GapInit_InitPhy_t
{
  INIT_PHY_1M    = 0x01,   //!< 1Mbps PHY. Bit 0
  INIT_PHY_2M    = 0x02,   //!< 2Mbps PHY. Bit 1
  INIT_PHY_CODED = 0x04     //!< Coded PHY. Bit 2
};

/**
 * @defgroup GAPInit_PHY_Param_defaults GAPInit PHY Parameter Default Value
 * @{
 */
#define INIT_PHYPARAM_DFLT_SCAN_INTERVAL   16     //!< Default scan interval (in 625us)
#define INIT_PHYPARAM_DFLT_SCAN_WINDOW     16     //!< Default scan interval (in 625us)
#define INIT_PHYPARAM_DFLT_CONN_LATENCY    0      //!< Default connection latency
#define INIT_PHYPARAM_DFLT_SUP_TIMEOUT     2000   //!< Default supervision timeout (in 10ms)
#define INIT_PHYPARAM_DFLT_MIN_CE_LEN      0      //!< Default minimum connection event length (currently ignored)
#define INIT_PHYPARAM_DFLT_MAX_CE_LEN      0xFFFF //!< Default maximum connection event length (currently ignored)
/** @} End GAPInit_PHY_Param_defaults */

/** @} End GapInit_Constants */

/**
 * @defgroup GapInit_Params GapInit Params
 *
 * Params used in the GapInit module
 * @{
 */

/**
 * GAP Initiator PHY Parameters
 *
 * These can be set with @ref GapInit_setPhyParam and read with
 * @ref GapInit_getPhyParam. The default values below refer to the values
 * that are set at initialization.
 */
typedef enum
{
  /**
   * Scan Interval
   *
   * default: @ref INIT_PHYPARAM_DFLT_SCAN_INTERVAL
   *
   * range: 4 - 16384
   */
  INIT_PHYPARAM_SCAN_INTERVAL,

  /**
   * Scan Window
   *
   * default: @ref INIT_PHYPARAM_DFLT_SCAN_INTERVAL
   *
   * range: 4 - 16384
   */
  INIT_PHYPARAM_SCAN_WINDOW,

  /**
   * Minimum Connection Interval
   *
   * @note This should be equal to or smaller than @ref INIT_PHYPARAM_CONN_INT_MAX
   *
   * default: @ref INIT_PHYPARAM_DFLT_CONN_INT_MIN
   *
   * range: 6 - 3200
   */
  INIT_PHYPARAM_CONN_INT_MIN,

  /**
   * Maximum Connection Interval
   *
   * @note This should be equal to or greater than @ref INIT_PHYPARAM_CONN_INT_MIN
   *
   * default: @ref INIT_PHYPARAM_DFLT_CONN_INT_MAX
   *
   * range: 6 - 3200
   */
  INIT_PHYPARAM_CONN_INT_MAX,

  /**
   * Slave Latency
   *
   * default: @ref INIT_PHYPARAM_DFLT_CONN_LATENCY
   *
   * range: 0 - 499
   */
  INIT_PHYPARAM_CONN_LATENCY,

  /**
   * Supervision Timeout
   *
   * default: @ref INIT_PHYPARAM_DFLT_SUP_TIMEOUT
   *
   * range: 10 - 3200
   */
  INIT_PHYPARAM_SUP_TIMEOUT,

  /**
   * Minimum Length of Connection Event
   *
   * @warning This is not used by the controller
   *
   * default: @ref INIT_PHYPARAM_DFLT_MIN_CE_LEN
   *
   * range: 0 - 0xFFFF
   */
  INIT_PHYPARAM_MIN_CE_LEN,

  /**
   * Maximum Length of Connection Event
   *
   * @note This should be equal to or greater than @ref INIT_PHYPARAM_DFLT_MIN_CE_LEN
   *
   * @warning This is not used by the controller
   *
   * default: @ref INIT_PHYPARAM_DFLT_MAX_CE_LEN
   *
   * range: 0 - 0xFFFF
   */
  INIT_PHYPARAM_MAX_CE_LEN,

/// @cond NODOC
  INIT_NUM_PHYPARAM,
/// @endcond // NODOC
} GapInit_PhyParamId_t;
/** @} End GapInit_Params */

/*-------------------------------------------------------------------
 * Structures
 */

/**
 * @defgroup GapInit_Structs GapInit Structures
 *
 * Data structures used in the GapInit module
 * @{
 */

/**
 * @ref GAP_CONNECTING_CANCELLED_EVENT message format.
 *
 * This message is sent to the app
 * when the CreateConnection request is cancelled.
 */
typedef struct
{
  eventHeader_t  hdr;                //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                    //!< @ref GAP_CONNECTING_CANCELLED_EVENT
} gapConnCancelledEvent_t;

/** @} End GapInit_Structs */

/*-------------------------------------------------------------------
 * API's
 */

/**
 * Set parameters dependent on PHY.
 *
 * @note that if phys contains more than one PHY, the same parameter of those
 * PHYs will be set with the same value.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref bleInvalidRange
 *
 * @param phys connection channel PHY(s): Individual PHY values of
 *        @ref GapInit_InitPhy_t can be OR'ed.
 * @param paramId parameter ID
 * @param value parameter value
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapInit_setPhyParam(uint8_t phys, GapInit_PhyParamId_t paramId,
                             uint16_t value);

/**
 * Get parameters of the specified PHY.
 *
 * @note Phy shall indicate only one PHY.
 *
 * The host will send the CommandStatus Event to return the parameter value
 * in question.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref INVALIDPARAMETER
 * @ref bleInvalidRange
 *
 * @param phy connection channel PHY: shall be one from @ref GapInit_InitPhy_t.
 * @param paramId parameter ID
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapInit_getPhyParam(uint8_t phy, GapInit_PhyParamId_t paramId);

/**
 * Initiate connection with the specified peer device
 *
 * The host will send the CommandStatus Event to indicate whether the connect
 * process has started successfully. If a connection is formed, the
 * GAP_LinkEstablished event will be returned.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref bleNotReady
 * @ref bleInvalidRange
 * @ref bleMemAllocError
 * @ref bleAlreadyInRequestedMode
 *
 * @param peerAddrType peer device's address type.
 * @param pPeerAddress peer device's address
 * @param phys PHY to try making connection on: shall be one from
 *        @ref GapInit_InitPhy_t.
 * @param timeout If there is no chance to initiate a connection within timeout
 *        ms, this connect request will be canceled automatically. if timeout is
 *        0, the initiator will keep trying to get a chance to make a connection
 *        until @ref GapInit_cancelConnect is called.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapInit_connect(GAP_Peer_Addr_Types_t peerAddrType,
                         uint8_t* pPeerAddress, uint8_t phys, uint16_t timeout);

/**
 *
 *  Initiate connection with a device in the whitelist.
 *
 * The host will send the CommandStatus Event to indicate whether the connect
 * process has started successfully. If a connection is formed, the
 * GAP_LinkEstablished event will be returned.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref bleNotReady
 * @ref bleInvalidRange
 * @ref bleMemAllocError
 * @ref bleAlreadyInRequestedMode
 *
 * @param phys PHY to try making connection on: shall be one from
 *        @ref GapInit_InitPhy_t.
 * @param timeout If there is no chance to initiate a connection within timeout
 *        ms, this connect request will be canceled automatically. if timeout is
 *        0, the initiator will keep trying to get a chance to make a connection
 *        until @ref GapInit_cancelConnect is called.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapInit_connectWl(uint8_t phys, uint16_t timeout);

/**
 * Cancel the ongoing connection process.
 *
 * The host will send the CommandStatus Event to indicate whether the connect
 * cancel process has started successfully. If the cancel completes, the
 * GAP_ConnectingCancelled event will be returned.
 * @ref bleSUCCESS
 * @ref bleNotReady
 * @ref bleIncorrectMode
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GapInit_cancelConnect(void);

/// @endcond // NODOC

/*-------------------------------------------------------------------
-------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* GAP_INITIATOR_H */

/** @} End GapInit */

