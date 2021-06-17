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

#ifndef GAP_H
#define GAP_H

#include "hal_defs.h"
#include "bcomdef.h"
#include "hci_tl.h"
#include "sm.h"

/// @cond NODOC
#ifndef status_t
  #define status_t bStatus_t
#endif
/// @endcond // NODOC


#define GATT_MTU_SIZE                   0xB4
#define MAX_BLE_DEVICES                 30
#define SM_KEYLEN                       16


/**
 * GAP Vendor Specific APIs - GAP Command Opcodes
 */
#define GAP_DEVICE_INIT                 0xFE00
#define GAP_AUTHENTICATE                0xFE0B
#define GAP_TERMINATEAUTH               0xFE10
#define GAP_UPDATELINKPARAMREQ          0xFE11
#define GAP_DEVICE_DISCOVERY_REQUEST    0xFE04
#define GAP_DEVICE_DISCOVERY_CANCEL     0xFE05
#define GAP_MAKE_DISCOVERABLE           0xFE06
#define GAP_UPDATE_ADVERTISING_DATA     0xFE07
#define GAP_END_DISCOVERABLE            0xFE08
#define GAP_ESTABLISH_LINK_REQUEST      0xFE09
#define GAP_TERMINATE_LINK_REQUEST      0xFE0A
#define GAP_UPDATELINKPARAMREQREPLY     0xFFFE
#define GAP_REGISTERCONNEVENT           0xFE13
#define GAP_BOND                        0xFE0F
#define GAP_SIGNABLE                    0xFE0E
#define GAP_PASSKEYUPDATE               0xFE0C
#define GAP_SENDSLAVESECURITYREQUEST    0xFE0D
#define GAPCONFIG_SETPARAMETER          0xFE2F
#define GAP_SETPARAMVALUE               0xFE30
#define GAP_GETPARAMVALUE               0xFE31
#define GAP_BOND_MGR_SET_PARAMETER      0xFE36
#define GAPSCAN_ENABLE                  0xFE51
#define GAPSCAN_DISABLE                 0xFE52
#define GAPSCAN_SETPHYPARAMS            0xFE53
#define GAPSCAN_GETPHYPARAMS            0xFE54
#define GAPSCAN_SETPARAM                0xFE55
#define GAPSCAN_GETPARAM                0xFE56
#define GAPSCAN_SETEVENTMASK            0xFE57
#define GAPSCAN_GETADVREPORT            0xFE58
#define GAPINIT_SETPHYPARAM             0xFE60
#define GAPINIT_GETPHYPARAM             0xFE61
#define GAPINIT_CONNECT                 0xFE62
#define GAPINIT_CONNECTWL               0xFE63
#define GAPINIT_CANCELCONNECT           0xFE64


/**
 * GAP Vendor Specific APIs - GAP Event Opcodes
 */
#define GAP_DEVICE_INIT_DONE            0x0600
#define GAP_DEVICE_DISCOVERY_DONE       0x0601
#define GAP_ADVERT_DATA_UPDATE_DONE     0x0602
#define GAP_MAKE_DISCOVERABLE_DONE      0x0603
#define GAP_END_DISCOVERABLE_DONE       0x0604
#define GAP_LINK_ESTABLISHED            0x0605
#define GAP_LINK_TERMINATED             0x0606
#define GAP_LINK_PARAM_UPDATE           0x0607
#define GAP_SIGNATUREUPDATED            0x0609
#define GAP_PASSKEYNEEDED               0x0609
#define GAP_AUTHENTICATIONCOMPLETE      0x0609
#define GAP_SLAVEREQUESTEDSECURITY      0x0609
#define GAP_BONDCOMPLETE                0x0609
#define GAP_PAIRINGREQUESTED            0x0609
#define GAP_CONNECTINGCANCELLED         0x0609
#define GAP_CONNECTIONEVENTNOTICE       0x0609
#define SM_GETECCKEYS                   0x0609
#define SM_GETDHKEY                     0x0609
#define GAP_LINKPARAMUPDATEREQEST       0x0609
#define GAP_DEVICE_INFORMATION          0x060D
#define GAP_ADVERTISERSCANNEREVENT      0x0613

/**
 * Default GAP Parameter Values
 */


#define DEFAULT_ADDRESS_SIZE            0x06
#define DEFAULT_PEER_ADDRESS_SIZE       16

#define DEFAULT_PASSKEY_SIZE            0x06

// Parameter types for GAP_BOND_MGR_SET_PARAMETER

#define GAPBOND_PAIRING_MODE            0x400
#define GAPBOND_MITM_PROTECTION         0x402
#define GAPBOND_IO_CAPABILITIES         0x403
#define GAPBOND_OOB_ENABLED             0x404
#define GAPBOND_OOB_DATA                0x405
#define GAPBOND_BONDING_ENABLED         0x406
#define GAPBOND_KEY_DIST_LIST           0x407
#define GAPBOND_DEFAULT_PASSCODE        0x408
#define GAPBOND_ERASE_ALLBONDS          0x409
#define GAPBOND_AUTO_FAIL_PAIRING       0x40A
#define GAPBOND_AUTO_FAIL_REASON        0x40B
#define GAPBOND_KEYSIZE                 0x40C
#define GAPBOND_AUTO_SYNC_WL            0x40D
#define GAPBOND_BOND_FAIL_ACTION        0x40F
#define GAPBOND_ERASE_SINGLEBOND        0x410
#define GAPBOND_SECURE_CONNECTION       0x411
#define GAPBOND_ECCKEY_REGEN_POLICY     0x412
#define GAPBOND_GAPBOND_ECC_KEYS        0x413
#define GAPBOND_LRU_BOND_REPLACEMENT    0x418
#define GAPBOND_ERASE_LOCAL_INFO        0x41A

#define GAPBOND_PAIRING_MODE_NO_PAIRING     0x00
#define GAPBOND_PAIRING_MODE_WAIT_FOR_REQ   0x01
#define GAPBOND_PAIRING_MODE_INITIATE       0x02

#define GAPBOND_IO_CAP_DISPLAY_YES_NO       0x01
#define GAPBOND_IO_CAP_DISPLAY_ONLY         0x00
#define GAPBOND_IO_CAP_KEYBOARD_ONLY        0x02
#define GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT   0x03
#define GAPBOND_IO_CAP_KEYBOARD_DISPLAY     0x04

/*-------------------------------------------------------------------
 * MACROS
 */
/// @cond NODOC
#ifndef status_t
  #define status_t bStatus_t
#endif
/// @endcond // NODOC

/**
 * Is the address random private resolvable (RPA)?
 *
 * @param pAddr pointer to address
 *
 * @return TRUE the address is an RPA
 * @return FALSE the address is not an RPA
 */
#define GAP_IS_ADDR_RPR(pAddr) ((pAddr[B_ADDR_LEN-1] & RANDOM_ADDR_HDR_MASK) \
                                 == PRIVATE_RESOLVE_ADDR_HDR)

/**
 * Is the address random private non-resolvable (NRPA)?
 *
 * @param pAddr pointer to address
 *
 * @return TRUE the address is an NRPA
 * @return FALSE the address is not an NRPA
 */
#define GAP_IS_ADDR_RPN(pAddr) ((pAddr[B_ADDR_LEN-1] & RANDOM_ADDR_HDR_MASK) \
                                 == PRIVATE_NON_RESOLVE_ADDR_HDR)

/**
 * Is the address random static?
 *
 * @param pAddr pointer to address
 *
 * @return TRUE the address is random static
 * @return FALSE the address is not random static
 */
#define GAP_IS_ADDR_RS(pAddr)  ((pAddr[B_ADDR_LEN-1] & RANDOM_ADDR_HDR_MASK) \
                                 == STATIC_ADDR_HDR)

/**
 * Is the address any type of random address?
 */
#define GAP_IS_ADDR_RAND(pAddr) (GAP_IS_ADDR_RPR(pAddr) | \
                                 GAP_IS_ADDR_RPN(pAddr) | \
                                 GAP_IS_ADDR_RS(pAddr))

/*-------------------------------------------------------------------
 * CONSTANTS
 */

/**
 * @defgroup GAP_Events GAP Events
 * Event ID's that can be received from the GAP layer
 * @{
 */

/**
 * @defgroup GAP_Event_IDs GAP Event IDs
 * @{
 *
 * These events are received as @ref GAP_MSG_EVENT
 */
/**
 * Sent after a call to @ref GAP_DeviceInit when the Device Initialization is
 * complete as @ref gapDeviceInitDoneEvent_t.
 */
#define GAP_DEVICE_INIT_DONE_EVENT            0x00
/**
 * Sent after a link has been established as  @ref gapEstLinkReqEvent_t.
 */
#define GAP_LINK_ESTABLISHED_EVENT            0x05
/**
 * Sent when a connection was terminated as @ref gapTerminateLinkEvent_t.
 */
#define GAP_LINK_TERMINATED_EVENT             0x06
/**
 * Sent after the completion of a parameter update  @ref gapLinkUpdateEvent_t.
 *
 * This same event is received for both the LL and L2CAP procedures.
 */
#define GAP_LINK_PARAM_UPDATE_EVENT           0x07
/**
 * Sent when the peer device's signature counter is updated as
 * @ref gapSignUpdateEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_SIGNATURE_UPDATED_EVENT           0x09
/**
 * Sent when the pairing process is complete as @ref gapAuthCompleteEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_AUTHENTICATION_COMPLETE_EVENT     0x0A
/**
 * Sent when a passkey is needed during pairing as @ref gapPasskeyNeededEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_PASSKEY_NEEDED_EVENT              0x0B
/**
 * Sent when a Slave Security Request is received as
 * @ref gapSlaveSecurityReqEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_SLAVE_REQUESTED_SECURITY_EVENT    0x0C
/**
 * Sent when the bonding process is complete as @ref gapBondCompleteEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_BOND_COMPLETE_EVENT               0x0E
/**
 * Sent when an unexpected Pairing Request is received as
 * @ref gapPairingReqEvent_t.
 *
 * This event will be consumed by the gapbondmgr if it is present
 */
#define GAP_PAIRING_REQ_EVENT                 0x0F
/**
 * Sent when pairing fails due a connection termination before the process
 * completed. This event does not have any payload.
 *
 * This event will be consumed by the gapbondmgr if it is present.
 */
#define GAP_AUTHENTICATION_FAILURE_EVT        0x10
/**
 * Sent when a parameter update request is received. This event is only
 * received when @ref GAP_PARAM_LINK_UPDATE_DECISION is set to
 * @ref GAP_UPDATE_REQ_PASS_TO_APP.  This event is sent
 * as @ref gapUpdateLinkParamReqEvent_t.
 */
#define GAP_UPDATE_LINK_PARAM_REQ_EVENT       0x11
/// Sent when an advertising report session ends. This is an internal event.
#define GAP_SCAN_SESSION_END_EVENT            0x12
/// Sent when an advertising set needs to be removed. This is an internal event.
#define GAP_ADV_REMOVE_SET_EVENT              0x13
/**
 * Sent as @ref gapConnCancelledEvent_t when an a connecting attempt is canceled
 */
#define GAP_CONNECTING_CANCELLED_EVENT        0x15

/**
 * Sent as @ref gapBondLostEvent_t when bond has been removed on peer
 */
#define GAP_BOND_LOST_EVENT                   0x17
/** @} End GAP_Event_IDs */

/**
 * @defgroup GapAdvScan_Event_IDs GapAdv Event IDs
 * These are received through the @ref pfnGapCB_t registered in the
 * @ref GapAdv_create
 *
 * See the individual event to see how pBuf in the @ref pfnGapCB_t should be
 * cast.
 * @{
 */
/**
 * Sent on the first advertisement after a @ref GapAdv_enable
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_START_AFTER_ENABLE             (uint32_t)BV(0)
/**
 * Sent after advertising stops due to a @ref GapAdv_disable
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_END_AFTER_DISABLE              (uint32_t)BV(1)
/**
 * Sent at the beginning of each advertisement (for legacy advertising) or at
 * the beginning of each each advertisement set (for extended advertising)
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_START                          (uint32_t)BV(2)
/**
 * Sent after each advertisement (for legacy advertising) or at
 * the end of each each advertisement set (for extended advertising)
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_END                            (uint32_t)BV(3)
/**
 * Sent when an advertisement set is terminated due to a connection
 * establishment
 *
 * pBuf should be cast to @ref GapAdv_setTerm_t
 */
#define GAP_EVT_ADV_SET_TERMINATED                 (uint32_t)BV(4)
/**
 * Sent when a scan request is received
 *
 * pBuf should be cast to @ref GapAdv_scanReqReceived_t
 */
#define GAP_EVT_SCAN_REQ_RECEIVED                  (uint32_t)BV(5)
/**
 * Sent when the advertising data is truncated due to the limited advertisement
 * data length for connectable advertisements.
 *
 * pBuf should be cast to @ref GapAdv_truncData_t
 */
#define GAP_EVT_ADV_DATA_TRUNCATED                 (uint32_t)BV(6)
/// Scanner has been enabled.
#define GAP_EVT_SCAN_ENABLED                       (uint32_t)BV(16)
/**
 * Scanner has been disabled.
 *
 * This event comes with a message of @ref GapScan_Evt_End_t. Application is
 * responsible for freeing the message.
 */
#define GAP_EVT_SCAN_DISABLED                      (uint32_t)BV(17)
/**
 * Scan period has ended.
 *
 * Possibly a new scan period and a new scan duration have started.
 */
#define GAP_EVT_SCAN_PRD_ENDED                     (uint32_t)BV(18)
/// Scan duration has ended.
#define GAP_EVT_SCAN_DUR_ENDED                     (uint32_t)BV(19)
/**
 * Scan interval has ended.
 *
 * Possibly a new scan interval and a new scan window have started.
 */
#define GAP_EVT_SCAN_INT_ENDED                     (uint32_t)BV(20)
/// Scan window has ended.
#define GAP_EVT_SCAN_WND_ENDED                     (uint32_t)BV(21)
/**
 * An Adv or a ScanRsp has been received.
 *
 * This event comes with a message of @ref GapScan_Evt_AdvRpt_t. Application is
 * responsible for freeing both the message and the message data (msg->pData).
 */
#define GAP_EVT_ADV_REPORT                         (uint32_t)BV(22)
/**
 * Maximum number of Adv reports have been recorded.
 *
 * This event comes with a message of @ref GapScan_Evt_AdvRpt_t.
 */
#define GAP_EVT_ADV_REPORT_FULL                    (uint32_t)BV(23)
/// @cond NODOC
/**
 * A Sync with a periodic Adv has been established.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvSyncEst_t.
 * Application is responsible for freeing the message.
 */
#define GAP_EVT_PRD_ADV_SYNC_ESTABLISHED           (uint32_t)BV(24)
/**
 * A periodic Adv has been received.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvRpt_t. Application
 * is responsible for freeing both the message and the message data (msg->pData).
 */
#define GAP_EVT_PRD_ADV_REPORT                     (uint32_t)BV(25)
/**
 * A sync with a periodic has been lost.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvSyncLost_t.
 * Application is responsible for freeing the message.
 */
#define GAP_EVT_PRD_ADV_SYNC_LOST                  (uint32_t)BV(26)
/// @endcond NODOC
/// A memory failure has occurred.
#define GAP_EVT_INSUFFICIENT_MEMORY                (uint32_t)BV(31)

/**
 * Sent on the first advertisement after a @ref GapAdv_enable
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_START_AFTER_ENABLE             (uint32_t)BV(0)
/**
 * Sent after advertising stops due to a @ref GapAdv_disable
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_END_AFTER_DISABLE              (uint32_t)BV(1)
/**
 * Sent at the beginning of each advertisement (for legacy advertising) or at
 * the beginning of each each advertisement set (for extended advertising)
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_START                          (uint32_t)BV(2)
/**
 * Sent after each advertisement (for legacy advertising) or at
 * the end of each each advertisement set (for extended advertising)
 *
 * pBuf should be cast to a uint8_t which will contain the advertising handle
 */
#define GAP_EVT_ADV_END                            (uint32_t)BV(3)
/**
 * Sent when an advertisement set is terminated due to a connection
 * establishment
 *
 * pBuf should be cast to @ref GapAdv_setTerm_t
 */
#define GAP_EVT_ADV_SET_TERMINATED                 (uint32_t)BV(4)
/**
 * Sent when a scan request is received
 *
 * pBuf should be cast to @ref GapAdv_scanReqReceived_t
 */
#define GAP_EVT_SCAN_REQ_RECEIVED                  (uint32_t)BV(5)
/**
 * Sent when the advertising data is truncated due to the limited advertisement
 * data length for connectable advertisements.
 *
 * pBuf should be cast to @ref GapAdv_truncData_t
 */
#define GAP_EVT_ADV_DATA_TRUNCATED                 (uint32_t)BV(6)
/// Scanner has been enabled.
#define GAP_EVT_SCAN_ENABLED                       (uint32_t)BV(16)
/**
 * Scanner has been disabled.
 *
 * This event comes with a message of @ref GapScan_Evt_End_t. Application is
 * responsible for freeing the message.
 */
#define GAP_EVT_SCAN_DISABLED                      (uint32_t)BV(17)
/**
 * Scan period has ended.
 *
 * Possibly a new scan period and a new scan duration have started.
 */
#define GAP_EVT_SCAN_PRD_ENDED                     (uint32_t)BV(18)
/// Scan duration has ended.
#define GAP_EVT_SCAN_DUR_ENDED                     (uint32_t)BV(19)
/**
 * Scan interval has ended.
 *
 * Possibly a new scan interval and a new scan window have started.
 */
#define GAP_EVT_SCAN_INT_ENDED                     (uint32_t)BV(20)
/// Scan window has ended.
#define GAP_EVT_SCAN_WND_ENDED                     (uint32_t)BV(21)
/**
 * An Adv or a ScanRsp has been received.
 *
 * This event comes with a message of @ref GapScan_Evt_AdvRpt_t. Application is
 * responsible for freeing both the message and the message data (msg->pData).
 */
#define GAP_EVT_ADV_REPORT                         (uint32_t)BV(22)
/**
 * Maximum number of Adv reports have been recorded.
 *
 * This event comes with a message of @ref GapScan_Evt_AdvRpt_t.
 */
#define GAP_EVT_ADV_REPORT_FULL                    (uint32_t)BV(23)
/// @cond NODOC
/**
 * A Sync with a periodic Adv has been established.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvSyncEst_t.
 * Application is responsible for freeing the message.
 */
#define GAP_EVT_PRD_ADV_SYNC_ESTABLISHED           (uint32_t)BV(24)
/**
 * A periodic Adv has been received.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvRpt_t. Application
 * is responsible for freeing both the message and the message data (msg->pData).
 */
#define GAP_EVT_PRD_ADV_REPORT                     (uint32_t)BV(25)
/**
 * A sync with a periodic has been lost.
 *
 * This event comes with a message of @ref GapScan_Evt_PrdAdvSyncLost_t.
 * Application is responsible for freeing the message.
 */
#define GAP_EVT_PRD_ADV_SYNC_LOST                  (uint32_t)BV(26)
/// @endcond NODOC
/// A memory failure has occurred.
#define GAP_EVT_INSUFFICIENT_MEMORY                (uint32_t)BV(31)

/// Mask for all advertising events
#define GAP_EVT_ADV_EVT_MASK   (GAP_EVT_ADV_START_AFTER_ENABLE |   \
                                GAP_EVT_ADV_END_AFTER_DISABLE |    \
                                GAP_EVT_ADV_START |                \
                                GAP_EVT_ADV_END |                  \
                                GAP_EVT_ADV_SET_TERMINATED |       \
                                GAP_EVT_SCAN_REQ_RECEIVED |        \
                                GAP_EVT_ADV_DATA_TRUNCATED |       \
                                GAP_EVT_INSUFFICIENT_MEMORY)

/// Mask for all scan events
#define GAP_EVT_SCAN_EVT_MASK  (GAP_EVT_SCAN_ENABLED |                    \
                                GAP_EVT_SCAN_DISABLED |                   \
                                GAP_EVT_SCAN_PRD_ENDED |                  \
                                GAP_EVT_SCAN_DUR_ENDED |                  \
                                GAP_EVT_SCAN_INT_ENDED |                  \
                                GAP_EVT_SCAN_WND_ENDED |                  \
                                GAP_EVT_ADV_REPORT |                      \
                                GAP_EVT_ADV_REPORT_FULL |                 \
                                GAP_EVT_PRD_ADV_SYNC_ESTABLISHED |        \
                                GAP_EVT_PRD_ADV_REPORT |                  \
                                GAP_EVT_PRD_ADV_SYNC_LOST |               \
                                GAP_EVT_INSUFFICIENT_MEMORY)

/** @} End GapAdvScan_Event_IDs */

/**
 * GAP Configuration Parameters
 *
 * These can be set with @ref GapConfig_SetParameter
 */
typedef enum
{
  /**
   * @brief Can be used by the application to set the IRK
   *
   * It is not necessary to set this parameter. If it is not set, a random IRK
   * will be generated unless there is a valid IRK in NV.
   *
   * @warning This must be set before calling @ref GAP_DeviceInit
   *
   * The priority is:
   * 1. Set manually with GapConfig_SetParameter before @ref GAP_DeviceInit
   * 2. Previously stored in NV by the gapbondmgr
   * 3. Set randomly during @ref GAP_DeviceInit
   *
   * size: 16 bytes
   *
   * @note The IRK can be read with @ref GAP_GetIRK
   */
  GAP_CONFIG_PARAM_IRK,

  /**
   * @brief Can be used by the application to set the SRK
   *
   * It is not necessary to set this parameter. If it is not set, a random SRK
   * will be generated unless there is a valid SRK in NV.
   *
   * @warning This must be set before calling @ref GAP_DeviceInit
   *
   * The priority is:
   * 1. Set manually with GapConfig_SetParameter before @ref GAP_DeviceInit
   * 2. Previously stored in NV by the gapbondmgr
   * 3. Set randomly during @ref GAP_DeviceInit
   *
   * size: 16 bytes
   *
   * @note The SRK can be read with @ref GAP_GetSRK
   */
  GAP_CONFIG_PARAM_SRK,

/// @cond NODOC
  GAP_CONFIG_PARAM_COUNT
/// @endcond //NODOC
} Gap_configParamIds_t;

/**
 * GAP Parameter IDs
 *
 * Parameters set via @ref GAP_SetParamValue
 */

// Values from http://dev.ti.com/tirex/content/simplelink_cc2640r2_sdk_1_35_00_33/docs/blestack/TI_BLE_Vendor_Specific_HCI_Guide.pdf
enum {
    TGAP_GEN_DISC_ADV_MIN,
    TGAP_LIM_ADV_TIMEOUT,
    TGAP_GEN_DISC_SCAN,
    TGAP_LIM_DISC_SCAN,
    TGAP_CONN_EST_ADV_TIMEOUT,
    TGAP_CONN_PARAM_TIMEOUT,
    TGAP_LIM_DISC_ADV_INT_MIN,
    TGAP_LIM_DISC_ADV_INT_MAX,
    TGAP_GEN_DISC_ADV_INT_MIN,
    TGAP_GEN_DISC_ADV_INT_MAX,
    TGAP_CONN_ADV_INT_MIN,
    TGAP_CONN_ADV_INT_MAX,
    TGAP_CONN_SCAN_INT,
    TGAP_CONN_SCAN_WIND,
    TGAP_CONN_HIGH_SCAN_INT,
    TGAP_CONN_HIGH_SCAN_WIND,
    TGAP_GEN_DISC_SCAN_INT,
    TGAP_GEN_DISC_SCAN_WIND,
    TGAP_LIM_DISC_SCAN_INT,
    TGAP_LIM_DISC_SCAN_WIND,
    TGAP_CONN_EST_ADV,
    TGAP_CONN_EST_INT_MIN,
    TGAP_CONN_EST_INT_MAX,
    TGAP_CONN_EST_SCAN_INT,
    TGAP_CONN_EST_SCAN_WIND,
    TGAP_CONN_EST_SUPERV_TIMEOUT,
    TGAP_CONN_EST_LATENCY,
    TGAP_CONN_EST_MIN_CE_LEN,
    TGAP_CONN_EST_MAX_CE_LEN,
    TGAP_PRIVATE_ADDR_INT,
    TGAP_CONN_PAUSE_CENTRAL,
    TGAP_CONN_PAUSE_PERIPHERAL,
    TGAP_SM_TIMEOUT,
    TGAP_SM_MIN_KEY_LEN,
    TGAP_SM_MAX_KEY_LEN,
    TGAP_FILTER_ADV_REPORTS,
    TGAP_SCAN_RSP_RSSI_MIN,
    TGAP_REJECT_CONN_PARAMS,
#if defined TESTMODES
    TGAP_GAP_TESTCODE,
    TGAP_SM_TESTCODE,
    TGAP_GATT_TESTCODE = 100,
    TGAP_ATT_TESTCODE,
    TGAP_GGS_TESTCODE,
    TGAP_L2CAP_TESTCODE,
#endif
};

// Alternate values?
enum Gap_ParamIDs_t
{
  /**
   * Action to take upon receiving a parameter update request.
   *
   * default: @ref GAP_UPDATE_REQ_PASS_TO_APP
   *
   * range: @ref Gap_updateDecision_t
   */
  GAP_PARAM_LINK_UPDATE_DECISION,

  /**
   * Connection Parameter timeout.
   *
   * Minimum time after an L2CAP Connection Parameter Update Response has been
   * received that a L2CAP Connection Parameter Update Request can be sent.
   *
   * See section Version 5.0 Vol 3, Part C, Section 9.3.9.2 of the BT Core Spec
   *
   * default: 30000
   *
   * range: 1-65535
   */
  GAP_PARAM_CONN_PARAM_TIMEOUT,

  /**
   * Minimum Time Interval between private (resolvable) address changes
   * (minutes)
   *
   * @note No event is received when the address changes as per the Core Spec.
   *
   * default: 15
   *
   * range: 1-65535
   */
  GAP_PARAM_PRIVATE_ADDR_INT,

  /**
   * Time to wait for security manager response before returning
   * bleTimeout (ms)
   *
   * default: 30000
   *
   * range: 1-65535
   */
  GAP_PARAM_SM_TIMEOUT,

  /**
   * SM Minimum Key Length supported
   *
   * default: 7
   *
   * range: 1-65535
   */
  GAP_PARAM_SM_MIN_KEY_LEN,

  /**
   * SM Maximum Key Length supported
   *
   * default: 16
   *
   * range: 1-65535
   */
  GAP_PARAM_SM_MAX_KEY_LEN,

/// @cond NODOC
  /**
   * Task ID override for Task Authentication control (for stack internal use
   * only)
   */
  GAP_PARAM_AUTH_TASK_ID,

/**
 * This parameter is deprecated. This value is to avoid modifying the
 * following values.
 */
  GAP_PARAM_DEPRECATED,

  /**
   * Used to set GAP GATT Server (GGS) parameters. This is only used by the
   * transport layer
   *
   * default: 5
   *
   */
  GAP_PARAM_GGS_PARAMS,

#if defined ( TESTMODES )
  /**
   * GAP TestCodes - puts GAP into a test mode
   */
  GAP_PARAM_GAP_TESTCODE,

  /**
   * SM TestCodes - puts SM into a test mode
   */
  GAP_PARAM_SM_TESTCODE,

  /**
   * GATT TestCodes - puts GATT into a test mode (paramValue maintained by GATT)
   */
  GAP_PARAM_GATT_TESTCODE,

  /**
   * ATT TestCodes - puts ATT into a test mode (paramValue maintained by ATT)
   */
  GAP_PARAM_ATT_TESTCODE,

  /**
   * L2CAP TestCodes - puts L2CAP into a test mode (paramValue maintained by
   * L2CAP)
   */
  GAP_PARAM_L2CAP_TESTCODE,
#endif // TESTMODES
  /**
   * ID MAX-valid Parameter ID
   */
  GAP_PARAMID_MAX
/// @endcond //NODOC
};

/** @} End GAP_Params */

/**
 * @defgroup GAP_Constants GAP Constants
 * Other defines used in the GAP layer
 * @{
 */

/**
 * @defgroup GAP_Profile_Roles GAP Profile Roles
 * Bit mask values
 * @{
 */
/// A device that sends advertising events only.
#define GAP_PROFILE_BROADCASTER   0x01
/// A device that receives advertising events only.
#define GAP_PROFILE_OBSERVER      0x02
/**
 * A device that accepts the establishment of an LE physical link using the
 * establishment procedure.
 */
#define GAP_PROFILE_PERIPHERAL    0x04
/**
 * A device that supports the Central role initiates the establishment of a
 * physical connection.
 */
#define GAP_PROFILE_CENTRAL       0x08
/** @} End GAP_Profile_Roles */

/**
 * Used by GAP_UPDATEADVERTISINGDATA adData parameter.
 */
typedef enum
{
    GAP_AD_TYPE_SCAN_RSP_DATA,
    GAP_AD_TYPE_ADVERTISEMNT_DATA,
} Gap_adType_t;

typedef enum
{
    ADV_IND,
    ADV_DIRECT_IND,
    ADV_SCAN_IND,
    ADV_NONCONN_IND,
    SCAN_RSP,
} Gap_eventType_t;

typedef enum
{
    GAP_INITIATOR_ADDR_TYPE_PUBLIC,
    GAP_INITIATOR_ADDR_TYPE_STATIC,
    GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE,
    GAP_INITIATOR_ADDR_TYPE_PRIVATE_RESOLVE,
} Gap_initiatorAddrType_t;

typedef enum
{
    GAP_CHANNEL_MAP_CH_37 = 1 << 0,
    GAP_CHANNEL_MAP_CH_38 = 1 << 1,
    GAP_CHANNEL_MAP_CH_39 = 1 << 2,
    GAP_CHANNEL_MAP_ALL = GAP_CHANNEL_MAP_CH_37 | GAP_CHANNEL_MAP_CH_38 | GAP_CHANNEL_MAP_CH_39
} Gap_channelMap_t;

typedef enum {
    GAP_DEVICE_DISCOVERY_MODE_NONDISCOVERABLE   = 0x00,    //!< No discoverable setting
    GAP_DEVICE_DISCOVERY_MODE_GENERAL           = 0x01,    //!< General Discoverable devices
    GAP_DEVICE_DISCOVERY_MODE_LIMITED           = 0x02,    //!< Limited Discoverable devices
    GAP_DEVICE_DISCOVERY_MODE_ALL               = 0x03,    //!< Not filtered
} Gap_deviceDiscoveryMode_t;

typedef enum
{
    GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY,
    GAP_FILTER_POLICY_SCAN_WHITELIST_CONNECT_ANY,
    GAP_FILTER_POLICY_SCAN_ANY_CONNECT_WHITELIST,
    GAP_FILTER_POLICY_SCAN_WHITELIST_CONNECT_WHITELIST,
} Gap_filterPolicy_t;

/**
 * Options for responding to connection parameter update requests
 *
 * These are used by @ref GAP_PARAM_LINK_UPDATE_DECISION
 */
typedef enum
{
  GAP_UPDATE_REQ_ACCEPT_ALL,     //!< Accept all parameter update requests
  GAP_UPDATE_REQ_DENY_ALL,       //!< Deny all parameter update requests
  /**
   * Pass a @ref GAP_UPDATE_LINK_PARAM_REQ_EVENT to the app for it to decide by
   * responding with @ref GAP_UpdateLinkParamReqReply
   */
  GAP_UPDATE_REQ_PASS_TO_APP
} Gap_updateDecision_t;


/// Address modes to initialize the local device
typedef enum
{
  ADDRMODE_PUBLIC            = 0x00,  //!< Always Use Public Address
  ADDRMODE_RANDOM            = 0x01,  //!< Always Use Random Static Address
  /// Always Use Resolvable Private Address with Public Identity Address
  ADDRMODE_RP_WITH_PUBLIC_ID = 0x02,
  /// Always Use Resolvable Private Address with Random Identity Address
  ADDRMODE_RP_WITH_RANDOM_ID = 0x03,
} GAP_Addr_Modes_t;

/// Address types used for identifying peer address type
typedef enum
{
  ADDRTYPE_PUBLIC    = 0x00,  //!< Public Device Address
  ADDRTYPE_RANDOM    = 0x01,  //!< Random Device Address
  /// Public Identity Address (corresponds to peer's RPA)
  ADDRTYPE_PUBLIC_ID = 0x02,
  /// Random (static) Identity Address (corresponds to peer's RPA)
  ADDRTYPE_RANDOM_ID = 0x03,
  /// Random Device Address (controller unable to resolve)
  ADDRTYPE_RANDOM_NR = 0xFE,
  ADDRTYPE_NONE      = 0xFF   //!< No address provided
} GAP_Addr_Types_t;

/// Address types used for specifying peer address type
typedef enum
{
  PEER_ADDRTYPE_PUBLIC_OR_PUBLIC_ID = 0x00,  //!< Public or Public ID Address
  PEER_ADDRTYPE_RANDOM_OR_RANDOM_ID = 0x01   //!< Random or Random ID Address
} GAP_Peer_Addr_Types_t;

/**
 * @defgroup Address_IDs Masks for setting and getting ID type
 * @{
 */
/// OR with addrtype to change addr type to ID
#define SET_ADDRTYPE_ID               0x02
/// AND with addrtype to remove ID from type
#define MASK_ADDRTYPE_ID              0x01
/** @} End Address_IDs */

/**
 * @defgroup Random_Addr_Bitfields Random Address bit-field mask and types
 * @{
 */
/// Get top 2 bits of address
#define RANDOM_ADDR_HDR_MASK          0xC0
/// Random Static Address (b11)
#define STATIC_ADDR_HDR               0xC0
/// Random Private Non-Resolvable Address (b10)
#define PRIVATE_NON_RESOLVE_ADDR_HDR  0x80
/// Random Private Resolvable Address (b01)
#define PRIVATE_RESOLVE_ADDR_HDR      0x40
/** @} End Random_Addr_Bitfields */

/**
 * @defgroup GAP_ADTypes GAP Advertisement Data Types
 * These are the data type identifiers for the data tokens in the advertisement
 * data field.
 * @{
 */
/// Gap Advertising Flags
#define GAP_ADTYPE_FLAGS                        0x01
/// Service: More 16-bit UUIDs available
#define GAP_ADTYPE_16BIT_MORE                   0x02
/// Service: Complete list of 16-bit UUIDs
#define GAP_ADTYPE_16BIT_COMPLETE               0x03
/// Service: More 32-bit UUIDs available
#define GAP_ADTYPE_32BIT_MORE                   0x04
/// Service: Complete list of 32-bit UUIDs
#define GAP_ADTYPE_32BIT_COMPLETE               0x05
/// Service: More 128-bit UUIDs available
#define GAP_ADTYPE_128BIT_MORE                  0x06
/// Service: Complete list of 128-bit UUIDs
#define GAP_ADTYPE_128BIT_COMPLETE              0x07
/// Shortened local name
#define GAP_ADTYPE_LOCAL_NAME_SHORT             0x08
/// Complete local name
#define GAP_ADTYPE_LOCAL_NAME_COMPLETE          0x09
/// TX Power Level: 0xXX: -127 to +127 dBm
#define GAP_ADTYPE_POWER_LEVEL                  0x0A
/// Simple Pairing OOB Tag: Class of device (3 octets)
#define GAP_ADTYPE_OOB_CLASS_OF_DEVICE          0x0D
/// Simple Pairing OOB Tag: Simple Pairing Hash C (16 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_HASHC     0x0E
/// Simple Pairing OOB Tag: Simple Pairing Randomizer R (16 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_RANDR     0x0F
/// Security Manager TK Value
#define GAP_ADTYPE_SM_TK                        0x10
/// Security Manager OOB Flags
#define GAP_ADTYPE_SM_OOB_FLAG                  0x11
/**
 * Min and Max values of the connection interval (2 octets Min, 2 octets Max)
 * (0xFFFF indicates no conn interval min or max)
 */
#define GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE    0x12
/// Signed Data field
#define GAP_ADTYPE_SIGNED_DATA                  0x13
/// Service Solicitation: list of 16-bit Service UUIDs
#define GAP_ADTYPE_SERVICES_LIST_16BIT          0x14
/// Service Solicitation: list of 128-bit Service UUIDs
#define GAP_ADTYPE_SERVICES_LIST_128BIT         0x15
/// Service Data - 16-bit UUID
#define GAP_ADTYPE_SERVICE_DATA                 0x16
/// Public Target Address
#define GAP_ADTYPE_PUBLIC_TARGET_ADDR           0x17
/// Random Target Address
#define GAP_ADTYPE_RANDOM_TARGET_ADDR           0x18
/// Appearance
#define GAP_ADTYPE_APPEARANCE                   0x19
/// Advertising Interval
#define GAP_ADTYPE_ADV_INTERVAL                 0x1A
/// LE Bluetooth Device Address
#define GAP_ADTYPE_LE_BD_ADDR                   0x1B
/// LE Role
#define GAP_ADTYPE_LE_ROLE                      0x1C
/// Simple Pairing Hash C-256
#define GAP_ADTYPE_SIMPLE_PAIRING_HASHC_256     0x1D
/// Simple Pairing Randomizer R-256
#define GAP_ADTYPE_SIMPLE_PAIRING_RANDR_256     0x1E
/// Service Data - 32-bit UUID
#define GAP_ADTYPE_SERVICE_DATA_32BIT           0x20
/// Service Data - 128-bit UUID
#define GAP_ADTYPE_SERVICE_DATA_128BIT          0x21
/// 3D Information Data
#define GAP_ADTYPE_3D_INFO_DATA                 0x3D
/**
 * Manufacturer Specific Data: first 2 octets contain the Company Identifier
 * Code followed by the additional manufacturer specific data
 */
#define GAP_ADTYPE_MANUFACTURER_SPECIFIC        0xFF
/// Discovery Mode: LE Limited Discoverable Mode
#define GAP_ADTYPE_FLAGS_LIMITED                0x01
/// Discovery Mode: LE General Discoverable Mode
#define GAP_ADTYPE_FLAGS_GENERAL                0x02
/// Discovery Mode: BR/EDR Not Supported
#define GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED    0x04
/** @} End GAP_ADTypes */

/**
 * @defgroup GAP_State_Flags GAP State Flags
 * @{
 */
#define GAP_STATE_IDLE                          0x00 //!< Device is idle
#define GAP_STATE_ADV                           0x01 //!< Device is advertising
#define GAP_STATE_SCAN                          0x02 //!< Device is scanning
#define GAP_STATE_INIT                          0x04 //!< Device is establishing a connection
/** @} End GAP_State_Flags */

/**
 * @defgroup GAP_Appearance_Values GAP Appearance Values
 * @{
 */
#define GAP_APPEARE_UNKNOWN                     0x0000 //!< Unknown
#define GAP_APPEARE_GENERIC_PHONE               0x0040 //!< Generic Phone
#define GAP_APPEARE_GENERIC_COMPUTER            0x0080 //!< Generic Computer
#define GAP_APPEARE_GENERIC_WATCH               0x00C0 //!< Generic Watch
#define GAP_APPEARE_WATCH_SPORTS                0x00C1 //!< Watch: Sports Watch
#define GAP_APPEARE_GENERIC_CLOCK               0x0100 //!< Generic Clock
#define GAP_APPEARE_GENERIC_DISPLAY             0x0140 //!< Generic Display
#define GAP_APPEARE_GENERIC_RC                  0x0180 //!< Generic Remote Control
#define GAP_APPEARE_GENERIC_EYE_GALSSES         0x01C0 //!< Generic Eye-glasses
#define GAP_APPEARE_GENERIC_TAG                 0x0200 //!< Generic Tag
#define GAP_APPEARE_GENERIC_KEYRING             0x0240 //!< Generic Keyring
#define GAP_APPEARE_GENERIC_MEDIA_PLAYER        0x0280 //!< Generic Media Player
#define GAP_APPEARE_GENERIC_BARCODE_SCANNER     0x02C0 //!< Generic Barcode Scanner
#define GAP_APPEARE_GENERIC_THERMOMETER         0x0300 //!< Generic Thermometer
#define GAP_APPEARE_GENERIC_THERMO_EAR          0x0301 //!< Thermometer: Ear
#define GAP_APPEARE_GENERIC_HR_SENSOR           0x0340 //!< Generic Heart rate Sensor
#define GAP_APPEARE_GENERIC_HRS_BELT            0x0341 //!< Heart Rate Sensor: Heart Rate Belt
#define GAP_APPEARE_GENERIC_BLOOD_PRESSURE      0x0380 //!< Generic Blood Pressure
#define GAP_APPEARE_GENERIC_BP_ARM              0x0381 //!< Blood Pressure: Arm
#define GAP_APPEARE_GENERIC_BP_WRIST            0x0382 //!< Blood Pressure: Wrist
#define GAP_APPEARE_GENERIC_HID                 0x03C0 //!< Generic Human Interface Device (HID)
#define GAP_APPEARE_HID_KEYBOARD                0x03C1 //!< HID Keyboard
#define GAP_APPEARE_HID_MOUSE                   0x03C2 //!< HID Mouse
#define GAP_APPEARE_HID_JOYSTIC                 0x03C3 //!< HID Joystick
#define GAP_APPEARE_HID_GAMEPAD                 0x03C4 //!< HID Gamepad
#define GAP_APPEARE_HID_DIGITIZER_TYABLET       0x03C5 //!< HID Digitizer Tablet
#define GAP_APPEARE_HID_DIGITAL_CARDREADER      0x03C6 //!< HID Card Reader
#define GAP_APPEARE_HID_DIGITAL_PEN             0x03C7 //!< HID Digital Pen
#define GAP_APPEARE_HID_BARCODE_SCANNER         0x03C8 //!< HID Barcode Scanner
/** @} End GAP_Appearance_Values */

/**
 * @defgroup GAP_PRIVACY_MODES GAP Privacy Modes
 * @{
 */
#define GAP_PRIVACY_MODE_NETWORK                0 //!< Device Privacy Mode
#define GAP_PRIVACY_MODE_DEVICE                 1 //!< Network Privacy Mode
/** @} End GAP_PRIVACY_MODES */

/**
 * Connection Event Notice PHY's
 */
typedef enum
{
  GAP_CONN_EVT_PHY_1MBPS = 1, //!< 1 MBPS
  GAP_CONN_EVT_PHY_2MBPS = 2, //!< 2 MBPS
  GAP_CONN_EVT_PHY_CODED = 4, //!< Coded-S2 or Coded-S8
} GAP_ConnEvtPhy_t;

/**
 * Status of connection events returned via @ref pfnGapConnEvtCB_t
 */
typedef enum
{
  /// Connection event occurred successfully
  GAP_CONN_EVT_STAT_SUCCESS   = 0,
  /// Connection event failed because all packets had CRC errors
  GAP_CONN_EVT_STAT_CRC_ERROR = 1,
  /// No data was received during connection event
  GAP_CONN_EVT_STAT_MISSED    = 2
} GAP_ConnEvtStat_t;

/**
 * Task type for next scheduled BLE task
 */
typedef enum
{
  /// Advertiser
  GAP_CONN_EVT_TASK_TYPE_ADV    = 0x01,
  /// Initiating a connection
  GAP_CONN_EVT_TASK_TYPE_INIT   = 0x02,
  /// Connection event in slave role
  GAP_CONN_EVT_TASK_TYPE_SLAVE  = 0x04,
  /// Scanner
  GAP_CONN_EVT_TASK_TYPE_SCAN   = 0x40,
  /// Connection event in master role
  GAP_CONN_EVT_TASK_TYPE_MASTER = 0x80,
  // No task
  GAP_CONN_EVT_TASK_TYPE_NONE   = 0xFF
} GAP_ConnEvtTaskType_t;

/**
 * Action to take for callback registration API's
 */
typedef enum
{
  GAP_CB_REGISTER, //!< Register a callback
  GAP_CB_UNREGISTER //!> Unregister a callback
} GAP_CB_Action_t;

/** @} End GAP_Constants */

/*-------------------------------------------------------------------
 * TYPEDEFS
 */

/// GAP event header format.
typedef struct
{
  eventHeader_t  hdr;              //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                  //!< GAP type of command. @ref GAP_Event_IDs
} gapEventHdr_t;

/**
 * Peripheral Preferred Connection Parameters.
 *
 * This is used to set the @ref GGS_PERI_CONN_PARAM_ATT param with
 * @ref GGS_SetParameter
 */
typedef struct
{
  /// Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms)
  uint16_t intervalMin;
  /// Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms)
  uint16_t intervalMax;
  /// Number of LL latency connection events (0x0000 - 0x03e8)
  uint16_t latency;
  /// Connection Timeout (0x000A - 0x0C80 * 10 ms)
  uint16_t timeout;
} gapPeriConnectParams_t;

/**
 * @ref GAP_DEVICE_INIT_DONE_EVENT message format.
 *
 * This message is sent to the
 * app when the Device Initialization is done [initiated by calling
 * @ref GAP_DeviceInit ].
 */
typedef struct
{
  eventHeader_t  hdr;                   //!< @ref GAP_MSG_EVENT and status
  uint8_t  opcode;                      //!< @ref GAP_DEVICE_INIT_DONE_EVENT
  uint8_t  numDataPkts;                 //!< HC_Total_Num_LE_Data_Packets
  uint16_t dataPktLen;                  //!< HC_LE_Data_Packet_Length
  uint8_t  devAddr[B_ADDR_LEN];         //!< Device's public or random static address
} gapDeviceInitDoneEvent_t;

/**
 * @ref GAP_SIGNATURE_UPDATED_EVENT message format.
 *
 * This message is sent to the
 * app when the signature counter has changed.  This message is to inform the
 * application in case it wants to save it to be restored on reboot or reconnect.
 * This message is sent to update a connection's signature counter and to update
 * this device's signature counter.  If devAddr == BD_ADDR, then this message
 * pertains to this device.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t  hdr;               //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                   //!< @ref GAP_SIGNATURE_UPDATED_EVENT
  uint8_t addrType;                 //!< Device's address type for devAddr
  uint8_t devAddr[B_ADDR_LEN];      //!< Device's BD_ADDR, could be own address
  uint32_t signCounter;             //!< new Signed Counter
} gapSignUpdateEvent_t;

/**
 * Establish Link Request parameters
 *
 * This is used by @ref GAP_UpdateLinkParamReq
 */
typedef struct
{
  uint16_t connectionHandle; //!< Connection handle of the update
  uint16_t intervalMin;      //!< Minimum Connection Interval
  uint16_t intervalMax;      //!< Maximum Connection Interval
  uint16_t connLatency;      //!< Connection Latency
  uint16_t connTimeout;      //!< Connection Timeout
  uint8_t  signalIdentifier; //!< L2CAP Signal Identifier. Must be 0 for LL Update
} gapUpdateLinkParamReq_t;

/**
 * Update Link Parameters Request Reply parameters
 *
 * This is used by @ref GAP_UpdateLinkParamReqReply
 */
typedef struct
{
  uint16_t connectionHandle; //!< Connection handle of the update
  uint16_t intervalMin;      //!< Minimum Connection Interval
  uint16_t intervalMax;      //!< Maximum Connection Interval
  uint16_t connLatency;      //!< Connection Latency
  uint16_t connTimeout;      //!< Connection Timeout
  uint8_t  signalIdentifier; //!< L2CAP Signal Identifier.
  uint8_t  accepted;         //!< TRUE if host accepts parameter update, FALSE otherwise.
} gapUpdateLinkParamReqReply_t;

/**
 *  @brief @ref GAP_UPDATE_LINK_PARAM_REQ_EVENT message format.
 *
 *  Connection parameters received by the remote device during a connection
 *  update procedure.
 */
typedef struct
{
  eventHeader_t  hdr;              //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                     //!< @ref GAP_UPDATE_LINK_PARAM_REQ_EVENT
  gapUpdateLinkParamReq_t req;        //!< Remote device's requested parameters
} gapUpdateLinkParamReqEvent_t;

/**
 * @ref GAP_LINK_ESTABLISHED_EVENT message format.
 *
 * This message is sent to the app when a link is established (with status
 * bleSUCCESS). For a Central, this is after @ref GapInit_connect or
 * @ref GapInit_connectWl completes successfully. For a Peripheral, this message
 * is sent to indicate that a link has been created.
 *
 * A status of something other than bleSUCCESS is possible in the following cases
 * - LL_STATUS_ERROR_UNKNOWN_CONN_HANDLE (0x02): As a master, connection
 * creation has been canceled.
 * - LL_STATUS_ERROR_DIRECTED_ADV_TIMEOUT (0x3C): As a slave, directed
 * advertising ended without a connection being formed.
 * - LL_STATUS_ERROR_UNACCEPTABLE_CONN_INTERVAL (0x3B): Slave received a
 * connection request with an invalid combination of connection parameters.
 */
typedef struct
{
  eventHeader_t  hdr;          //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;              //!< @ref GAP_LINK_ESTABLISHED_EVENT
  uint8_t devAddrType;         //!< Device address type: @ref GAP_Addr_Types_t
  uint8_t devAddr[B_ADDR_LEN]; //!< Device address of link
  uint16_t connectionHandle;   //!< Connection Handle for this connection
  uint8_t connRole;            //!< Role connection was formed as, @ref GAP_Profile_Roles
  uint16_t connInterval;       //!< Connection Interval
  uint16_t connLatency;        //!< Connection Latency
  uint16_t connTimeout;        //!< Connection Timeout
  uint8_t clockAccuracy;       //!< Clock Accuracy
} gapEstLinkReqEvent_t;

/**
 * @ref GAP_LINK_PARAM_UPDATE_EVENT message format.
 *
 * This message is sent to the app
 * when the connection parameters update request is complete.
 */
typedef struct
{
  eventHeader_t hdr;          //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;             //!< @ref GAP_LINK_PARAM_UPDATE_EVENT
  uint8_t status;             //!< status from link layer, defined in ll.h
  uint16_t connectionHandle;  //!< Connection handle of the update
  uint16_t connInterval;      //!< Requested connection interval
  uint16_t connLatency;       //!< Requested connection latency
  uint16_t connTimeout;       //!< Requested connection timeout
} gapLinkUpdateEvent_t;

/**
 * @ref GAP_LINK_TERMINATED_EVENT message format.
 *
 * This message is sent to the app when connection is terminated.
 */
typedef struct
{
  eventHeader_t hdr;         //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;            //!< @ref GAP_LINK_TERMINATED_EVENT
  uint16_t connectionHandle; //!< connection Handle
  uint8_t reason;            //!< termination reason from LL, defined in ll.h
} gapTerminateLinkEvent_t;

/**
 * @ref GAP_PASSKEY_NEEDED_EVENT message format.
 *
 * This message is sent to the app when a Passkey is needed from the
 * app's user interface.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;              //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                 //!< @ref GAP_PASSKEY_NEEDED_EVENT
  uint8_t deviceAddr[B_ADDR_LEN]; //!< address of device to pair with, and could be either public or random.
  uint16_t connectionHandle;      //!< Connection handle
  uint8_t uiInputs;               //!< Pairing User Interface Inputs - Ask user to input passcode
  uint8_t uiOutputs;              //!< Pairing User Interface Outputs - Display passcode
  uint32_t numComparison;         //!< Numeric Comparison value to be displayed.
} gapPasskeyNeededEvent_t;

/**
 * @ref GAP_AUTHENTICATION_COMPLETE_EVENT message format.
 *
 * This message is sent to the app when the authentication request is complete.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;               //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                  //!< @ref GAP_AUTHENTICATION_COMPLETE_EVENT
  uint16_t connectionHandle;       //!< Connection Handle from controller used to ref the device
  uint8_t authState;               //!< TRUE if the pairing was authenticated (MITM)
  smSecurityInfo_t *pSecurityInfo; //!< security information from this device
  smSigningInfo_t *pSigningInfo;   //!< Signing information
  smSecurityInfo_t *pDevSecInfo;   //!< security information from connected device
  smIdentityInfo_t *pIdentityInfo; //!< identity information
} gapAuthCompleteEvent_t;

/**
 * Authentication Parameters for @ref GAP_Authenticate which should only be used
 * if the gapbondmgr does not exist
 */
typedef struct
{
  uint16_t connectionHandle;    //!< Connection Handle from controller,
  smLinkSecurityReq_t  secReqs; //!< Pairing Control info
} gapAuthParams_t;

/**
 * @ref GAP_SLAVE_REQUESTED_SECURITY_EVENT message format.
 *
 * This message is sent to the app when a Slave Security Request is received.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;              //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                 //!< @ref GAP_SLAVE_REQUESTED_SECURITY_EVENT
  uint16_t connectionHandle;      //!< Connection Handle
  uint8_t deviceAddr[B_ADDR_LEN]; //!< address of device requesting security
  /**
   *  Authentication Requirements
   *
   *  Bit 2: MITM, Bits 0-1: bonding (0 - no bonding, 1 - bonding)
   */
  uint8_t authReq;
} gapSlaveSecurityReqEvent_t;

/**
 * @ref GAP_BOND_COMPLETE_EVENT message format.
 *
 * This message is sent to the app when a bonding is complete.  This means that
 * a key is loaded and the link is encrypted.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;         //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;            //!< @ref GAP_BOND_COMPLETE_EVENT
  uint16_t connectionHandle; //!< connection Handle
} gapBondCompleteEvent_t;

/**
 * Pairing Request fields for @ref GAP_Authenticate which should only be used
 * if the gapbondmgr does not exist
 */
typedef struct
{
  uint8_t enable;        //!< Pairing Request enable field
  uint8_t ioCap;         //!< Pairing Request ioCap field
  uint8_t oobDataFlag;   //!< Pairing Request OOB Data Flag field
  uint8_t authReq;       //!< Pairing Request Auth Req field
  uint8_t maxEncKeySize; //!< Pairing Request Maximum Encryption Key Size field
  keyDist_t keyDist;     //!< Pairing Request Key Distribution field
} gapPairingReq_t;

/**
 * @ref GAP_PAIRING_REQ_EVENT message format.
 *
 * This message is sent when an unexpected Pairing Request is
 * received and pairing must be initiated with @ref GAP_Authenticate using
 * the pairReq field received here
 *
 * @note This message should only be sent to peripheral devices.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;         //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;            //!< @ref GAP_PAIRING_REQ_EVENT
  uint16_t connectionHandle; //!< connection Handle
  gapPairingReq_t pairReq;   //!< The Pairing Request fields received.
} gapPairingReqEvent_t;

/**
 * Report describing connection event Returned via a @ref pfnGapConnEvtCB_t.
 */
typedef struct
{
  GAP_ConnEvtStat_t     status;   //!< status of connection event
  uint16_t              handle;   //!< connection handle
  uint8_t               channel;  //!< BLE RF channel index (0-39)
  GAP_ConnEvtPhy_t      phy;      //!< PHY of connection event
  int8_t                lastRssi; //!< RSSI of last packet received
  /// Number of packets received for this connection event
  uint16_t              packets;
  /// Total number of CRC errors for the entire connection
  uint16_t              errors;
  /// Type of next BLE task
  GAP_ConnEvtTaskType_t nextTaskType;
  /// Time to next BLE task (in us). 0xFFFFFFFF if there is no next task.
  uint32_t              nextTaskTime;
} Gap_ConnEventRpt_t;

/**
 * @ref GAP_BOND_LOST_EVENT message format.
 *
 * This message is sent to the app as indication that the bond has been removed on peer.
 *
 * This event will be consumed by the gapbondmgr if it exists.
 */
typedef struct
{
  eventHeader_t hdr;         //!< @ref GAP_MSG_EVENT and status
  uint8_t opcode;                 //!< @ref GAP_BOND_LOST_EVENT
  uint16_t connectionHandle;      //!< Connection Handle
  uint8_t deviceAddr[B_ADDR_LEN]; //!< address of device requesting pairing
} gapBondLostEvent_t;

typedef struct
{
    HCI_StatusCodes_t status;
    uint16_t opCode;
    uint8_t  dataLen;
    uint8_t *payLoad;
}commandStatus_t;

typedef struct
{
    HCI_StatusCodes_t status;
    uint8_t devAddrType;        /* Refer to enum GAP_AddrType */
    uint8_t devAddr[6];         /* Address of connected device */
    uint16_t connHandle;        /* Handle of the connection */
    uint16_t connInterval;      /* Connection interval used on this connection */
    uint16_t connLatency;       /* Connection latency used on this connection */
    uint16_t connTimeout;       /* Connection supervision timeout */
    uint8_t  clockAccuracy;     /* Refer to enum GAP_ClockAccuracy */
} GapLinkEstablished_t;
typedef struct
{
    HCI_StatusCodes_t status;
    uint16_t connHandle;
    uint8_t reason;
}GapTerminateLinkRequest_t;
typedef struct
{
    uint8_t RSSI;
    uint8_t advertData[35];
    uint8_t peerAddr[6];
} GapDeviceInformation_t;

/** @} End GAP_Structs */

/*-------------------------------------------------------------------
 * CALLBACKS
 */

/**
 * @defgroup GAP_CBs GAP Callbacks
 * @{
 */

/// Central Address Resolution (CAR) Support Callback Function
typedef uint8_t(*pfnSuppCentAddrRes_t)
(
  uint8_t *deviceAddr,           //!< address of device to check for CAR
  GAP_Peer_Addr_Types_t addrType //!< peer device's address type
);

/// GAP Idle Callback Function
typedef void (*pfnGapIdleCB_t)();

/// GAP Device Privacy Mode Callback Function
typedef uint8_t(*pfnGapDevPrivModeCB_t)
(
  GAP_Peer_Addr_Types_t  addrType,    //!< address type of device to check
  uint8_t                *pAddr       //!< address of device to check if Device Privacy mode is permissible
);

/// Callback Registration Structure
typedef struct
{
  pfnSuppCentAddrRes_t   suppCentAddrResCB;  //!< Supports Central Address Resolution
  pfnGapIdleCB_t         gapIdleCB;          //!< GAP Idle callback
  pfnGapDevPrivModeCB_t  gapDevPrivModeCB;   //!< GAP Device Privacy Mode callback
} gapBondMgrCBs_t;

/**
 * GAP Callback function pointer type for the advertising and scan modules.
 *
 * This callback will return @ref GapAdvScan_Event_IDs from the advertising module,
 * some of which can be masked with @ref GapAdv_eventMaskFlags_t. See the
 * respective event in @ref GapAdvScan_Event_IDs for the type that pBuf should be
 * cast to.
 *
 * This callback will also return @ref GapAdvScan_Event_IDs from the advertising
 * module. See the respective event in @ref GapAdvScan_Event_IDs for the type that
 * pBuf should be cast to.
 */
typedef void (*pfnGapCB_t)
(
  uint32_t event,   //!< see @ref GapAdvScan_Event_IDs and GapAdvScan_Event_IDs
  void *pBuf,       //!< data potentially accompanying event
  uintptr_t arg     //!< custom application argument that can be return through this callback
);

/**
 * GAP Callback function pointer type for Connection Event notifications.
 *
 * When registered via @ref Gap_RegisterConnEventCb, this callback will return
 * a pointer to a @ref Gap_ConnEventRpt_t from the controller after each
 * connection event
 *
 * @warning The application owns the memory pointed to by pReport. That is, it
 * is responsible for freeing this memory.
 *
 * @warning This is called from the stack task context. Therefore, processing
 * in this callback should be minimized. Any excessive processing should be
 * done by posting an event to the application task context.
 */
typedef void (*pfnGapConnEvtCB_t)
(
  /// Pointer to report describing the connection event
  Gap_ConnEventRpt_t *pReport
);

/** @} End GAP_CBs */

/*-------------------------------------------------------------------
 * FUNCTIONS - Initialization and Configuration
 */

/**
 * GAP Device Initialization
 *
 * This command is used to setup the device in a GAP Role and should only be
 * called once per reboot.Toenable multiple combinations setup multiple GAP
 * Roles (profileRole parameter).
 *
 * Multiple Role settings examples:
 *
 * - GAP_PROFILE_PERIPHERAL and GAP_PROFILE_BROADCASTER - allows a connection
 *   and advertising (non-connectable) at the same time.
 * - GAP_PROFILE_PERIPHERAL and GAP_PROFILE_OBSERVER - allows a connection (with
 *   master) and scanning at the same time.
 * - GAP_PROFILE_PERIPHERAL, GAP_PROFILE_OBSERVER and GAP_PROFILE_BROADCASTER -
 *   allows a connection (with master) and scanning or advertising at the same
 *   time.
 * - GAP_PROFILE_CENTRAL and GAP_PROFILE_BROADCASTER - allows connections and
 *   advertising (non-connectable) at the same time.
 *
 * Note: If the device’s BLE address (BDADDR) is to be set then this command
 * must be executed after HCI_EXT_SetBDADDRCmd for the generated security keys
 * to be valid.
 *
 * @par Corresponding Events:
 * @ref GAP_DEVICE_INIT_DONE_EVENT of type @ref gapDeviceInitDoneEvent_t
 *
 * When this command is received, the host will send the CommandStatus
 * Event. When initialization task is complete, the host will send the
 * GAP_DeviceInitDone event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : initialization started
 * @ref INVALIDPARAMETER : invalid profile role, role combination,
 *         or invalid Random Static Address,
 * @ref bleIncorrectMode : initialization has already occurred
 * @ref bleInternalError : error erasing NV
 *
 * @param profileRole GAP Profile Roles: @ref GAP_Profile_Roles
 * @param maxScanResponses Central or Observer only: The device will allocate
 *        buffer space for received advertisement packets. The default is 3.
 *        The larger the number, the more RAM that is needed and maintained.
 * @param irk 16 byte Identity Resolving Key (IRK). If this value is all 0’s,
 *        the GAP will randomly generate all 16 bytes. This key is used to
 *        generate Resolvable Private Addresses
 * @param csrk 16 byte Connection Signature Resolving Key (CSRK). If this value
 *        is all 0’s, the GAP will randomly generate all 16 bytes. This key is
 *        used to generate data Signature.
 * @param signCounter 32 bit Signature Counter. Initial signature counter.
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 *
 */
extern HCI_StatusCodes_t GAP_deviceInit(uint8_t profileRole,
                                 uint8_t maxScanResponses,
                                 uint8_t *irk, uint8_t *csrk,
                                 uint32_t signCounter);

/**
 * Send this command to start the device advertising.
 *
 * When this command is received, the host will send the HCI Ext Command Status
 * Event with the Statusparameter, then, when the device starts advertising
 * the GAP Make Discoverable Done Event is generated. When advertising is
 * completed (limited mode advertising has a time limit), the GAPEnd Discoverable
 * Event is generated.
 *
 * @param eventType
 * @param initiatorAddrType
 * @param initiatorAddr
 * @param channelMap
 * @param filterPolicy
 */
extern HCI_StatusCodes_t GAP_makeDiscoverable(Gap_eventType_t eventType,
                                Gap_initiatorAddrType_t initiatorAddrType,
                                uint8_t *initiatorAddr,
                                Gap_channelMap_t channelMap,
                                Gap_filterPolicy_t filterPolicy);

/**
 * Send this command to end advertising.
 *
 * When this command is received, the host will send the HCI Ext Command Status
 * Event with the Statusparameter, then issue a GAP End Discoverable Done Event
 * advertising has stopped
 */
extern HCI_StatusCodes_t GAP_endDiscoverable(void);

/**
 * Send this command to set the raw advertising or scan response data.
 *
 * When this command is received, the host will send the HCI Ext Command Status
 * Event with the Statusparameter, then,when the task is complete the GAP Advert
 * Data UpdateDoneEvent is generated.
 *
 * @param adType        the type of @ref advertData
 * @param dataLen       the size of @ref advertData in bytes
 * @param advertData    raw advertisting data
 */
extern HCI_StatusCodes_t GAP_updateAdvertistigData(Gap_adType_t adType,
                                         uint8_t dataLen, uint8_t *advertData);

/**
 * Set a GAP Parameter value
 *
 * Use this function to change the default GAP parameter values.
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref INVALIDPARAMETER
 *
 * @param paramID parameter ID: @ref Gap_ParamIDs_t
 * @param paramValue new param value
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_SetParamValue(uint8_t paramID, uint16_t paramValue);

/**
 * Get a GAP Parameter value.
 *
 * The host will send the CommandStatus Event to return the parameter value in question.
 * The following can be received from the CommandStatus Event:
 * @ref GAP Parameter Value
 * @ref 0xFFFF if invalid
 *
 * @param paramID parameter ID: @ref Gap_ParamIDs_t
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_GetParamValue(uint8_t paramID);


/*-------------------------------------------------------------------
 * FUNCTIONS - Device Discovery
 */

/**
 * Send this command to start a scan for advertisement packets.
 *
 * This command is valid for a central or a peripheral device.
 *
 * When this command is received, the host will send the HCI Ext Command Status
 * Event with the Status parameter. During the scan, the device will generate
 * GAP Device Information Events for advertising devices, then issue a GAP
 * Device Discovery Event when the scan is completed.
 *
 * @param mode          0 = Non-Discoverable Scan
 *                      1 = General Mode Scan
 *                      2 = Limited Mode Scan
 *                      3 = Scan for all devices
 * @param activeScan    0 = Turn off active scanning (SCAN_REQ)
 *                      1 = Turn on active scanning (SCAN_REQ)
 * @param filterPolicy  Filter policy.
 */
HCI_StatusCodes_t GAP_DeviceDiscoveryRequest(Gap_deviceDiscoveryMode_t mode, uint8_t activeScan, Gap_filterPolicy_t filterPolicy);

/**
 * Send this command to end a scan for advertisement packets.
 *
 * This command is valid for a central or a peripheral device.
 *
 * When this command is received, the host will send the HCI Ext Command Status
 * Event with the Status parameter, then issue a GAP Device Discovery Event to
 * display the scan progress since the start of the scan.
 */
HCI_StatusCodes_t GAP_DeviceDiscoveryCancel(void);


/*-------------------------------------------------------------------
 * FUNCTIONS - Link Establishment
 */

/**
 * Send this command to initiate a connection with a peripheral device.
 *
 * Only central devices can issue this command.
 *
 * @param highDutyCycle A central device may use high duty cycle scan parameters
 *      in order to achieve low latency connection time with a peripheral device
 *      using directed link establishment. 0 = disabled, 1 = enabled.
 * @param whiteList 0 = Don’t use the white list, 1 = Only connect to a device in the white list.
 * @param addrPeerType Address type of @p peerAddr.
 * @param peerAddr Bluetooth address.
 */
HCI_StatusCodes_t GAP_EstablishLinkReq(uint8_t highDutyCycle, uint8_t whiteList, GAP_Addr_Types_t addrTypePeer, uint8_t *peerAddr);

/**
 * Terminate a link connection.
 *
 * @par Corresponding Events:
 * @ref GAP_LINK_TERMINATED_EVENT of type @ref gapTerminateLinkEvent_t
 *
 * When this command is received, the host will send the CommandStatus
 * Event. When the connection is terminated, the GAP_LinkTerminated Event
 * will be generated.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : termination request sent to stack
 * @ref bleIncorrectMode : No Link to terminate
 * @ref bleInvalidTaskID : not app that established link
 *
 * @param connectionHandle connection handle of link to terminate
 *        or @ref LINKDB_CONNHANDLE_ALL
 * @param reason terminate reason.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
HCI_StatusCodes_t GAP_TerminateLinkReq(uint16_t connectionHandle, uint8_t reason);

/**
 * Update the link parameters to a Master or Slave device.
 *
 * As long as LL connection updates are supported on the own device (which is
 * the case by default), an LL Connection Update procedure will be attempted.
 * If this fails, the stack will automatically attempt an L2CAP parameter update
 * request.
 *
 * @par Corresponding Events:
 * After the update procedure is complete, the calling task will receive a
 * @ref GAP_LINK_PARAM_UPDATE_EVENT of type @ref gapLinkUpdateEvent_t regardless
 * of the connection parameter update procedure that occurred.
 *
 * When this command is received, the host will send the CommandStatus
 * Event. When the connection is terminated, the GAP_LinkParamUpdate Event
 * will be generated.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : update request sent to stack
 * @ref INVALIDPARAMETER : one of the parameters were invalid
 * @ref bleIncorrectMode : invalid profile role
 * @ref bleAlreadyInRequestedMode : already updating link parameters
 * @ref bleNotConnected : not in a connection
 *
 * @param pParams link update parameters
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_UpdateLinkParamReq(gapUpdateLinkParamReq_t *pParams);

/**
 * Reply to a Connection Parameter Update Request that was received from a
 * remote device.
 *
 * This API should be used in response to a @ref GAP_UPDATE_LINK_PARAM_REQ_EVENT
 * which will only be received when @ref GAP_PARAM_LINK_UPDATE_DECISION is set
 * to @ref GAP_UPDATE_REQ_PASS_TO_APP.
 *
 * @par Corresponding Events:
 * The calling task should call this API in response to a
 * @ref GAP_UPDATE_LINK_PARAM_REQ_EVENT
 * of type @ref gapUpdateLinkParamReqEvent_t <br>
 * After the update procedure is complete, the calling task will receive a
 * @ref GAP_LINK_PARAM_UPDATE_EVENT of type @ref gapLinkUpdateEvent_t
 *
 * When this command is received, the host will send the CommandStatus
 * Event. When the connection is terminated, the GAP_LinkParamUpdate Event
 * will be generated.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : reply sent successfully
 * @ref INVALIDPARAMETER : one of the parameters were invalid
 * @ref bleIncorrectMode : invalid profile role
 * @ref bleAlreadyInRequestedMode : already updating link parameters
 * @ref bleNotConnected : not in a connection
 *
 * @param pParams local device's desired connection parameters.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_UpdateLinkParamReqReply(gapUpdateLinkParamReqReply_t *pParams);

/*-------------------------------------------------------------------
 * FUNCTIONS - GAP Configuration
 */

/**
 * Set a GAP Configuration Parameter
 *
 * Use this function to write a GAP configuration parameter. These parameters
 * must be set before @ref GAP_DeviceInit
 *
 * When this command is received, the host will send the CommandStatu Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref INVALIDPARAMETER
 * @ref bleInvalidRange NULL pointer was passed
 * @ref bleIncorrectMode Device is already initialized
 *
 * @param param parameter ID: @ref Gap_configParamIds_t
 * @param pValue pointer to parameter value. Cast based on the type defined in
 *        @ref Gap_configParamIds_t
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GapConfig_SetParameter(Gap_configParamIds_t param,
                                        void *pValue);


/*-------------------------------------------------------------------
 * FUNCTIONS - Pairing
 */

/**
 * Terminate Authentication
 *
 * Send a Pairing Failed message and end any existing pairing.
 *
 * @par Corresponding Events:
 * @ref GAP_AUTHENTICATION_FAILURE_EVT
 *
 * When this command is received, the host will send the CommandStatus Event.
 * When the existing pairing had ended, the GAP_AuthenticationComplete with
 * be generated.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : function was successful
 * @ref bleMemAllocError : memory allocation error
 * @ref INVALIDPARAMETER : one of the parameters were invalid
 * @ref bleNotConnected : link not found
 * @ref bleInvalidRange : one of the parameters were not within range
 *
 * @param  connectionHandle connection handle.
 * @param  reason Pairing Failed reason code.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_TerminateAuth(uint16_t connectionHandle, uint8_t reason);

/**
 * Update the passkey in string format.
 *
 * This API should be called in response to receiving a
 * @ref GAP_PASSKEY_NEEDED_EVENT
 *
 * @note This function is the same as @ref GAP_PasscodeUpdate, except that
 * the passkey is passed in as a string format.
 *
 * @warning This API should not be called by the application if the
 * gapbondmgr exists as it is abstracted through @ref GAPBondMgr_PasscodeRsp
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : will start pairing with this entry
 * @ref bleIncorrectMode : Link not found
 * @ref INVALIDPARAMETER : passkey == NULL or passkey isn't formatted
 * properly
 *
 * @param pPasskey new passkey - pointer to numeric string (ie. "019655" )
 *        This string's range is "000000" to "999999"
 * @param connectionHandle connection handle.
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_PasskeyUpdate(uint8_t *pPasskey, uint16_t connectionHandle);

/**
 * Generate a Slave Requested Security message to the master.
 *
 * @warning This API should not be called by the application if the
 * gapbondmgr exists as it will be used automatically based on
 * @ref GAPBOND_PAIRING_MODE and the GAP role of the device
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS : will send
 * @ref bleNotConnected : Link not found
 * @ref bleIncorrectMode : wrong GAP role, must be a Peripheral Role
 *
 * @param connectionHandle connection handle.
 * @param authReq Authentication Requirements: Bit 2: MITM,
 *        Bits 0-1: bonding (0 - no bonding, 1 - bonding)
 *        Bit 3: Secure Connections
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_SendSlaveSecurityRequest(uint16_t connectionHandle,
                                              uint8_t authReq);

/**
 * Set up the connection to accept signed data.
 *
 * @warning This API should not be called by the application if the
 * gapbondmgr exists as it will be used automatically when signing occurs
 *
 * When this command is received, the host will send the CommandStatus Event.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref bleIncorrectMode : Not correct profile role
 * @ref INVALIDPARAMETER
 * @ref bleNotConnected
 * @ref bleFAILURE : not workable
 *
 * @param connectionHandle connection handle of the signing information
 * @param authenticated TRUE if the signing information is authenticated,
 *        FALSE otherwise
 * @param pParams signing parameters
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_Signable(uint16_t connectionHandle, uint8_t authenticated,
                              smSigningInfo_t *pParams);

/**
 * Set up the connection's bound parameters.
 *
 * @warning This API should not be called by the application if the
 * gapbondmgr exists as it will be used automatically when a connection is
 * formed to a previously bonded device
 *
 * @par Corresponding Events:
 * @ref GAP_BOND_COMPLETE_EVENT of type @ref gapBondCompleteEvent_t
 *
 * When this command is received, the host will send the CommandStatus Event.
 * When both connected devices have setup encryption, the GAP_BondComplete
 * is generated.
 * The following status values can be received from the CommandStatus Event:
 * @ref bleSUCCESS
 * @ref bleIncorrectMode : Not correct profile role
 * @ref INVALIDPARAMETER
 * @ref bleNotConnected
 * @ref bleFAILURE : not workable
 *
 * @param connectionHandle connection handle of the signing information
 * @param authenticated TRUE if bond is authenticated.
 * @param secureConnections TRUE if bond has Secure Connections strength.
 * @param pParams the connected device's security parameters
 * @param startEncryption whether or not to start encryption
 *
 * @return @ref bleSUCCESS : command sent successfully over HCI transport layer
 * @return @ref bleFAILURE : command failed to send over HCI transport layer
 */
extern HCI_StatusCodes_t GAP_Bond(uint16_t connectionHandle, uint8_t authenticated,
                          uint8_t secureConnections, smSecurityInfo_t *pParams,
                          uint8_t startEncryption);

/**
 * Set a bond manager parameter
 *
 * @param paramID one of the GAPBOND_* constants
 * @param paramDataLen length of paramData in bytes
 * @param paramData parameter-specific data (value/size depends on paramID)
 */
extern HCI_StatusCodes_t GAP_BondMgrSetParameter(uint16_t paramID, uint8_t paramDataLen, uint8_t *paramData);

#endif /* GAP_H_ */
