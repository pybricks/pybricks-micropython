/******************************************************************************

 @file  hci_ext.h

 @brief HCI Extensions Message Definitions.

 Group: WCS, BTS
 Target Device: cc13x2_26x2

 ******************************************************************************

 Copyright (c) 2009-2019, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************


 *****************************************************************************/

#ifndef HCI_EXT_H
#define HCI_EXT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

#include <stdint.h>

#include "hci_tl.h"

/*********************************************************************
 * CONSTANTS
 */

#define HCI_EXT_HDR_LEN                         5

/*** HCI Extension Commands ***/

// The 10-bit OCF (Opcode Command Field) of the HCI Opcode is further
// divided into two subsections:
// - Subgroup (3 bits):
//   - 0 (LL)
//   - 1 (L2CAP)
//   - 2 (ATT)
//   - 3 (GATT)
//   - 4 (GAP)
//   - 5 (UTIL)
//   - 6 (Reserved)
//   - 7 (User Profile)
// - Command (7 bits) or Profile (7 bits) if Subgroup value is set to
//   User Profile (i.e., all ones) in which case, another octet is
//   required to represent user profile commands.
//
#define HCI_EXT_LL_SUBGRP                       0x00
#define HCI_EXT_L2CAP_SUBGRP                    0x01
#define HCI_EXT_ATT_SUBGRP                      0x02
#define HCI_EXT_GATT_SUBGRP                     0x03
#define HCI_EXT_GAP_SUBGRP                      0x04
#define HCI_EXT_UTIL_SUBGRP                     0x05
#define HCI_EXT_PROFILE_SUBGRP                  0x07

#define HCI_EXT_UTIL_RESERVED                   0x00
#define HCI_EXT_UTIL_NV_READ                    0x01
#define HCI_EXT_UTIL_NV_WRITE                   0x02
#define HCI_EXT_UTIL_RESERVED2                  0x03
#define HCI_EXT_UTIL_BUILD_REV                  0x04
#define HCI_EXT_UTIL_GET_TRNG                   0x05
#define UTIL_EXT_GATT_GET_NEXT_HANDLE           0x06
#define HCI_EXT_UTIL_GET_MEM_STATS              0x07


// GAP Initialization and Configuration
#define HCI_EXT_GAP_DEVICE_INIT                 0x00
#define HCI_EXT_GAP_CONFIG_DEVICE_ADDR          0x03

// GAP Device Discovery
#define HCI_EXT_GAP_MAKE_DISCOVERABLE           0x06
#define HCI_EXT_GAP_UPDATE_ADV_DATA             0x07
#define HCI_EXT_GAP_END_DISC                    0x08

// GAP Link Establishment
#define HCI_EXT_GAP_EST_LINK_REQ                0x09
#define HCI_EXT_GAP_TERMINATE_LINK              0x0A
#define HCI_EXT_GAP_AUTHENTICATE                0x0B
#define HCI_EXT_GAP_PASSKEY_UPDATE              0x0C
#define HCI_EXT_GAP_SLAVE_SECURITY_REQ_UPDATE   0x0D
#define HCI_EXT_GAP_SIGNABLE                    0x0E
#define HCI_EXT_GAP_BOND                        0x0F
#define HCI_EXT_GAP_TERMINATE_AUTH              0x10
#define HCI_EXT_GAP_UPDATE_LINK_PARAM_REQ       0x11
#define HCI_EXT_GAP_UPDATE_LINK_PARAM_REQ_REPLY 0x12
#define HCI_EXT_GAP_REGISTER_CONN_EVT           0x13

// GAP Parameters
#define HCI_EXT_GAP_CONFIG_SET_PARAM            0x2F
#define HCI_EXT_GAP_SET_PARAM                   0x30
#define HCI_EXT_GAP_GET_PARAM                   0x31
#define HCI_EXT_GAP_RESOLVE_PRIVATE_ADDR        0x32
#define HCI_EXT_GAP_SET_ADV_TOKEN               0x33
#define HCI_EXT_GAP_REMOVE_ADV_TOKEN            0x34
#define HCI_EXT_GAP_UPDATE_ADV_TOKENS           0x35
#define HCI_EXT_GAP_BOND_SET_PARAM              0x36
#define HCI_EXT_GAP_BOND_GET_PARAM              0x37
#define HCI_EXT_GAP_BOND_SERVICE_CHANGE         0x38
#define HCI_EXT_GAP_BOND_PAIR                   0x48
#define HCI_EXT_GAP_BOND_FIND_ADDR              0x49
#define HCI_EXT_GAP_BOND_PASSCODE_RSP           0x4A

// GAP SM sub-procedures
#define HCI_EXT_SM_REGISTER_TASK                0x39
#define HCI_EXT_SM_GET_ECCKEYS                  0x3A
#define HCI_EXT_SM_GET_DHKEY                    0x3B
#define HCI_EXT_SM_GET_CONFIRM_OOB              0x3C

// GAP Privacy
#define HCI_EXT_GAP_SET_PRIVACY_MODE            0x3D

// GAP, Advertising Extension
#define HCI_EXT_GAP_ADV_CREATE                  0x3E
#define HCI_EXT_GAP_ADV_ENABLE                  0x3F
#define HCI_EXT_GAP_ADV_DISABLE                 0x40
#define HCI_EXT_GAP_ADV_DESTROY                 0x41
#define HCI_EXT_GAP_ADV_SET_PARAM               0x42
#define HCI_EXT_GAP_ADV_GET_PARAM               0x43
#define HCI_EXT_GAP_ADV_LOAD_DATA               0x44
#define HCI_EXT_GAP_ADV_SET_EVENT_MASK          0x45

#define HCI_EXT_GAP_ADV_PERIODIC_ENABLE         0x46
#define HCI_EXT_GAP_ADV_PERIODIC_DISABLE        0x47

#define HCI_EXT_GAP_SCAN_INIT                   0x50
#define HCI_EXT_GAP_SCAN_ENABLE                 0x51
#define HCI_EXT_GAP_SCAN_DISABLE                0x52
#define HCI_EXT_GAP_SCAN_SET_PHY_PARAMS         0x53
#define HCI_EXT_GAP_SCAN_GET_PHY_PARAMS         0x54
#define HCI_EXT_GAP_SCAN_SET_PARAM              0x55
#define HCI_EXT_GAP_SCAN_GET_PARAM              0x56
#define HCI_EXT_GAP_SCAN_SET_EVENT_MASK         0x57
#define HCI_EXT_GAP_SCAN_GET_ADV_REPORT         0x58

#define HCI_EXT_GAP_INIT_SET_PHY_PARAM          0x60
#define HCI_EXT_GAP_INIT_GET_PHY_PARAM          0x61
#define HCI_EXT_GAP_INIT_CONNECT                0x62
#define HCI_EXT_GAP_INIT_CONNECT_WL             0x63
#define HCI_EXT_GAP_INIT_CANCEL_CONNECT         0x64

// GATT Sub-Procedure Commands
// #define GATT_FIND_INCLUDED_SERVICES             0x30
// #define GATT_DISC_ALL_CHARS                     0x32
// #define GATT_READ_USING_CHAR_UUID               0x34
// #define GATT_WRITE_NO_RSP                       0x36
// #define GATT_SIGNED_WRITE_NO_RSP                0x38
// #define GATT_RELIABLE_WRITES                    0x3a
// #define GATT_READ_CHAR_DESC                     0x3c
// #define GATT_READ_LONG_CHAR_DESC                0x3e
// #define GATT_WRITE_CHAR_DESC                    0x40
// #define GATT_WRITE_LONG_CHAR_DESC               0x42
// #define GATT_CCC_UPDATE                         0x43

// GATT HCI Extension messages (0x7C - 0x7F)
#define HCI_EXT_GATT_ADD_SERVICE                ( GATT_BASE_METHOD | 0x3C ) // 0x7C
#define HCI_EXT_GATT_DEL_SERVICE                ( GATT_BASE_METHOD | 0x3D ) // 0x7D
#define HCI_EXT_GATT_ADD_ATTRIBUTE              ( GATT_BASE_METHOD | 0x3E ) // 0x7E
#define HCI_EXT_GATT_UPDATE_MTU                 ( GATT_BASE_METHOD | 0x3F ) // 0x7F


// L2CAP HCI Extension Commands (0x70-0x7F)
#define HCI_EXT_L2CAP_DATA                      0x70
#define HCI_EXT_L2CAP_REGISTER_PSM              0x71
#define HCI_EXT_L2CAP_DEREGISTER_PSM            0x72
#define HCI_EXT_L2CAP_PSM_INFO                  0x73
#define HCI_EXT_L2CAP_PSM_CHANNELS              0x74
#define HCI_EXT_L2CAP_CHANNEL_INFO              0x75


// LE Vendor Specific LL Extension Commands
#define HCI_EXT_SET_RX_GAIN                            0xFC00
#define HCI_EXT_SET_TX_POWER                           0xFC01
#define HCI_EXT_ONE_PKT_PER_EVT                        0xFC02
#define HCI_EXT_CLK_DIVIDE_ON_HALT                     0xFC03
#define HCI_EXT_DECLARE_NV_USAGE                       0xFC04
#define HCI_EXT_DECRYPT                                0xFC05
#define HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES           0xFC06
#define HCI_EXT_SET_FAST_TX_RESP_TIME                  0xFC07
#define HCI_EXT_MODEM_TEST_TX                          0xFC08
#define HCI_EXT_MODEM_HOP_TEST_TX                      0xFC09
#define HCI_EXT_MODEM_TEST_RX                          0xFC0A
#define HCI_EXT_END_MODEM_TEST                         0xFC0B
#define HCI_EXT_SET_BDADDR                             0xFC0C
#define HCI_EXT_SET_SCA                                0xFC0D
#define HCI_EXT_ENABLE_PTM                             0xFC0E // Not a supported HCI command! Application only.
#define HCI_EXT_SET_FREQ_TUNE                          0xFC0F
#define HCI_EXT_SAVE_FREQ_TUNE                         0xFC10
#define HCI_EXT_SET_MAX_DTM_TX_POWER                   0xFC11
#define HCI_EXT_MAP_PM_IO_PORT                         0xFC12
#define HCI_EXT_DISCONNECT_IMMED                       0xFC13
#define HCI_EXT_PER                                    0xFC14
#define HCI_EXT_PER_BY_CHAN                            0xFC15 // Not a supported HCI command! Application only.
#define HCI_EXT_EXTEND_RF_RANGE                        0xFC16
#define HCI_EXT_ADV_EVENT_NOTICE                       0xFC17 // Not a supported HCI command! Application only.
#define HCI_EXT_CONN_EVENT_NOTICE                      0xFC18 // Not a supported HCI command! Application only.
#define HCI_EXT_HALT_DURING_RF                         0xFC19
#define HCI_EXT_OVERRIDE_SL                            0xFC1A
#define HCI_EXT_BUILD_REVISION                         0xFC1B
#define HCI_EXT_DELAY_SLEEP                            0xFC1C
#define HCI_EXT_RESET_SYSTEM                           0xFC1D
#define HCI_EXT_OVERLAPPED_PROCESSING                  0xFC1E
#define HCI_EXT_NUM_COMPLETED_PKTS_LIMIT               0xFC1F
#define HCI_EXT_GET_CONNECTION_INFO                    0xFC20
#
#define HCI_EXT_LL_TEST_MODE                           0xFC70

// parameters for HCI_EXT_SET_TX_POWER

#define HCI_EXT_CC254X_TX_POWER_N23_DBM     0   // -23 dBm
#define HCI_EXT_CC254X_TX_POWER_N6_DBM      1   //  -6 dBm
#define HCI_EXT_CC254X_TX_POWER_0_DBM       2   //   0 dBm
#define HCI_EXT_CC254X_TX_POWER_4_DBM       3   //   4 dBm

#define HCI_EXT_CC26XX_TX_POWER_N21_DBM     0   // -21 dBm
#define HCI_EXT_CC26XX_TX_POWER_N18_DBM     1   // -18 dBm
#define HCI_EXT_CC26XX_TX_POWER_N15_DBM     2   // -15 dBm
#define HCI_EXT_CC26XX_TX_POWER_N12_DBM     3   // -12 dBm
#define HCI_EXT_CC26XX_TX_POWER_N9_DBM      4   //  -9 dBm
#define HCI_EXT_CC26XX_TX_POWER_N6_DBM      5   //  -6 dBm
#define HCI_EXT_CC26XX_TX_POWER_N3_DBM      6   //  -3 dBm
#define HCI_EXT_CC26XX_TX_POWER_0_DBM       7   //   0 dBm
#define HCI_EXT_CC26XX_TX_POWER_1_DBM       8   //   1 dBm
#define HCI_EXT_CC26XX_TX_POWER_2_DBM       9   //   2 dBm
#define HCI_EXT_CC26XX_TX_POWER_3_DBM       10  //   3 dBm
#define HCI_EXT_CC26XX_TX_POWER_4_DBM       11  //   4 dBm
#define HCI_EXT_CC26XX_TX_POWER_5_DBM       12  //   5 dBm

// parameters for HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES
// NB: technically, these are 64-bit flags, but all fit in 32-bits for now

#define HCI_EXT_LOCAL_FEATURE_NONE                      0x00000000UL
#define HCI_EXT_LOCAL_FEATURE_ENCRYTION                 0x00000001UL
#define HCI_EXT_LOCAL_FEATURE_CONNECTION_PARAM_REQ      0x00000002UL
#define HCI_EXT_LOCAL_FEATURE_REJECT_EXT_INDICATION     0x00000004UL
#define HCI_EXT_LOCAL_FEATURE_SLAVE_FEATURE_EXCHANGE    0x00000008UL
#define HCI_EXT_LOCAL_FEATURE_PING                      0x00000010UL
#define HCI_EXT_LOCAL_FEATURE_DATA_PACKET_LENGTH_EXT    0x00000020UL
#define HCI_EXT_LOCAL_FEATURE_PRIVACY                   0x00000040UL
#define HCI_EXT_LOCAL_FEATURE_EXT_SCANNER_FILTER_POLICY 0x00000080UL
#define HCI_EXT_LOCAL_FEATURE_2M_PHY                    0x00000100UL
#define HCI_EXT_LOCAL_FEATURE_STABLE_MOD_INDEX_TX       0x00000200UL
#define HCI_EXT_LOCAL_FEATURE_STABLE_MOD_INDEX_RX       0x00000400UL
#define HCI_EXT_LOCAL_FEATURE_CODED_PHY                 0x00000800UL
#define HCI_EXT_LOCAL_FEATURE_EXT_ADVERTISING           0x00001000UL
#define HCI_EXT_LOCAL_FEATURE_PERIODIC_ADVERTISING      0x00002000UL
#define HCI_EXT_LOCAL_FEATURE_CHAN_SELECT_ALGO_2        0x00004000UL
#define HCI_EXT_LOCAL_FEATURE_LE_POWER_CLASS_1          0x00008000UL
#define HCI_EXT_LOCAL_FEATURE_MIN_NUM_USED_CHAN         0x00010000UL

/*** HCI Extension Events ***/

// HCI extension events must start from 0x0400. The upper 6 bits of all
// zeros is reserved for the HCI extension embedded commands. The rest of
// the event bits should follow the HCI extension command format (i.e.,
// 3-bit Subgroup + 7-bit Event).
//
#define HCI_EXT_BASE_EVENT                      ( 0x0001 << 10 )  // 0x0400

#define HCI_EXT_LL_EVENT                        ( HCI_EXT_BASE_EVENT | (HCI_EXT_LL_SUBGRP << 7) )    // 0x0400
#define HCI_EXT_L2CAP_EVENT                     ( HCI_EXT_BASE_EVENT | (HCI_EXT_L2CAP_SUBGRP << 7) ) // 0x0480
#define HCI_EXT_ATT_EVENT                       ( HCI_EXT_BASE_EVENT | (HCI_EXT_ATT_SUBGRP << 7) )   // 0x0500
#define HCI_EXT_GATT_EVENT                      ( HCI_EXT_BASE_EVENT | (HCI_EXT_GATT_SUBGRP << 7) )  // 0x0580
#define HCI_EXT_GAP_EVENT                       ( HCI_EXT_BASE_EVENT | (HCI_EXT_GAP_SUBGRP << 7) )   // 0x0600
#define HCI_EXT_UTIL_EVENT                      ( HCI_EXT_BASE_EVENT | (HCI_EXT_UTIL_SUBGRP << 7) )  // 0x0680

// GAP Events
#define HCI_EXT_GAP_DEVICE_INIT_DONE_EVENT          ( HCI_EXT_GAP_EVENT | 0x00 )
#define HCI_EXT_GAP_ADV_DATA_UPDATE_DONE_EVENT      ( HCI_EXT_GAP_EVENT | 0x02 )
#define HCI_EXT_GAP_MAKE_DISCOVERABLE_DONE_EVENT    ( HCI_EXT_GAP_EVENT | 0x03 )
#define HCI_EXT_GAP_LINK_ESTABLISHED_EVENT          ( HCI_EXT_GAP_EVENT | 0x05 )
#define HCI_EXT_GAP_LINK_TERMINATED_EVENT           ( HCI_EXT_GAP_EVENT | 0x06 )
#define HCI_EXT_GAP_LINK_PARAM_UPDATE_EVENT         ( HCI_EXT_GAP_EVENT | 0x07 )
#define HCI_EXT_GAP_SIGNATURE_UPDATED_EVENT         ( HCI_EXT_GAP_EVENT | 0x09 )
#define HCI_EXT_GAP_AUTH_COMPLETE_EVENT             ( HCI_EXT_GAP_EVENT | 0x0A )
#define HCI_EXT_GAP_PASSKEY_NEEDED_EVENT            ( HCI_EXT_GAP_EVENT | 0x0B )
#define HCI_EXT_GAP_SLAVE_REQUESTED_SECURITY_EVENT  ( HCI_EXT_GAP_EVENT | 0x0C )
#define HCI_EXT_GAP_DEVICE_INFO_EVENT               ( HCI_EXT_GAP_EVENT | 0x0D )
#define HCI_EXT_GAP_BOND_COMPLETE_EVENT             ( HCI_EXT_GAP_EVENT | 0x0E )
#define HCI_EXT_GAP_PAIRING_REQ_EVENT               ( HCI_EXT_GAP_EVENT | 0x0F )
// SM subset of GAP Events
#define HCI_EXT_SM_GET_ECC_KEYS_EVENT               ( HCI_EXT_GAP_EVENT | 0x10 )
#define HCI_EXT_SM_GET_DH_KEY_EVENT                 ( HCI_EXT_GAP_EVENT | 0x11 )
// GAP Events (continued)
#define HCI_EXT_GAP_LINK_PARAM_UPDATE_REQ_EVENT     ( HCI_EXT_GAP_EVENT | 0x12 )
#define HCI_EXT_GAP_ADV_SCAN_EVENT                  ( HCI_EXT_GAP_EVENT | 0x13 )
#define HCI_EXT_GAP_CONNECTING_CANCELLED_EVENT      ( HCI_EXT_GAP_EVENT | 0x15 )
#define HCI_EXT_GAP_CONN_EVT_NOTICE                 ( HCI_EXT_GAP_EVENT | 0x16 )
#define HCI_EXT_GAP_BOND_LOST_EVENT                 ( HCI_EXT_GAP_EVENT | 0x17 )
// UTIL Events (continued)
#define HCI_EXT_UTIL_MEM_STATS_EVENT                ( HCI_EXT_UTIL_EVENT | 0x01 )
#define HCI_EXT_UTIL_SYSTEM_ERROR                   ( HCI_EXT_UTIL_EVENT | 0x02 )
// Command Status Events
#define HCI_EXT_GAP_CMD_STATUS_EVENT                ( HCI_EXT_GAP_EVENT | 0x7F )

// LE Vendor Specific LL Extension Events
#define HCI_EXT_SET_RX_GAIN_EVENT                      0x0400
#define HCI_EXT_SET_TX_POWER_EVENT                     0x0401
#define HCI_EXT_ONE_PKT_PER_EVT_EVENT                  0x0402
#define HCI_EXT_CLK_DIVIDE_ON_HALT_EVENT               0x0403
#define HCI_EXT_DECLARE_NV_USAGE_EVENT                 0x0404
#define HCI_EXT_DECRYPT_EVENT                          0x0405
#define HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES_EVENT     0x0406
#define HCI_EXT_SET_FAST_TX_RESP_TIME_EVENT            0x0407
#define HCI_EXT_MODEM_TEST_TX_EVENT                    0x0408
#define HCI_EXT_MODEM_HOP_TEST_TX_EVENT                0x0409
#define HCI_EXT_MODEM_TEST_RX_EVENT                    0x040A
#define HCI_EXT_END_MODEM_TEST_EVENT                   0x040B
#define HCI_EXT_SET_BDADDR_EVENT                       0x040C
#define HCI_EXT_SET_SCA_EVENT                          0x040D
#define HCI_EXT_ENABLE_PTM_EVENT                       0x040E // Not a supported HCI command! Application only.
#define HCI_EXT_SET_FREQ_TUNE_EVENT                    0x040F
#define HCI_EXT_SAVE_FREQ_TUNE_EVENT                   0x0410
#define HCI_EXT_SET_MAX_DTM_TX_POWER_EVENT             0x0411
#define HCI_EXT_MAP_PM_IO_PORT_EVENT                   0x0412
#define HCI_EXT_DISCONNECT_IMMED_EVENT                 0x0413
#define HCI_EXT_PER_EVENT                              0x0414
#define HCI_EXT_PER_BY_CHAN_EVENT                      0x0415 // Not a supported HCI command! Application only.
#define HCI_EXT_EXTEND_RF_RANGE_EVENT                  0x0416
#define HCI_EXT_ADV_EVENT_NOTICE_EVENT                 0x0417 // Not a supported HCI command! Application only.
#define HCI_EXT_CONN_EVENT_NOTICE_EVENT                0x0418 // Not a supported HCI command! Application only.
#define HCI_EXT_HALT_DURING_RF_EVENT                   0x0419
#define HCI_EXT_OVERRIDE_SL_EVENT                      0x041A
#define HCI_EXT_BUILD_REVISION_EVENT                   0x041B
#define HCI_EXT_DELAY_SLEEP_EVENT                      0x041C
#define HCI_EXT_RESET_SYSTEM_EVENT                     0x041D
#define HCI_EXT_OVERLAPPED_PROCESSING_EVENT            0x041E
#define HCI_EXT_NUM_COMPLETED_PKTS_LIMIT_EVENT         0x041F
#define HCI_EXT_GET_CONNECTION_INFO_EVENT              0x0420
#
#define HCI_EXT_LL_TEST_MODE_EVENT                     0x0470

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

HCI_StatusCodes_t HCI_EXT_setTxPower(uint8_t power);
HCI_StatusCodes_t HCI_EXT_setBdaddr(const uint8_t *bdaddr);
HCI_StatusCodes_t HCI_EXT_setLocalSupportedFeatures(const uint32_t localFeatures);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* HCI_EXT_H */
