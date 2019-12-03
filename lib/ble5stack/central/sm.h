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
 *  @file  sm.h
 *  @brief    This file contains the interface to the SM.
 */

#ifndef SM_H
#define SM_H

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------
 * INCLUDES
 */
#include "bcomdef.h"

/*-------------------------------------------------------------------
 * MACROS
 */

/*-------------------------------------------------------------------
 * CONSTANTS
 */
/** @addtogroup GAPBondMgr_Constants
 *  @{
 */

/**
 * @defgroup SM_IO_CAP_DEFINES SM I/O Capabilities
 * @{
 */
#define DISPLAY_ONLY              0x00  //!< Display Only Device
#define DISPLAY_YES_NO            0x01  //!< Display and Yes and No Capable
#define KEYBOARD_ONLY             0x02  //!< Keyboard Only
#define NO_INPUT_NO_OUTPUT        0x03  //!< No Display or Input Device
#define KEYBOARD_DISPLAY          0x04  //!< Both Keyboard and Display Capable
/** @} End SM_IO_CAP_DEFINES */

#define SM_AUTH_MITM_MASK(a)    (((a) & 0x04) >> 2)   //!< MITM Mask

/**
 * @defgroup SM_PASSKEY_TYPE_DEFINES SM Passkey Types (Bit Masks)
 * @{
 */
#define SM_PASSKEY_TYPE_INPUT   0x01    //!< Input the passkey
#define SM_PASSKEY_TYPE_DISPLAY 0x02    //!< Display the passkey
/** @} End SM_PASSKEY_TYPE_DEFINES */

/**
 * @defgroup SM_BONDING_FLAGS_DEFINES SM AuthReq Bonding Flags
 * Bonding flags 0x02 and 0x03 are reserved.
 * @{
 */
#define SM_AUTH_REQ_NO_BONDING    0x00  //!< No bonding
#define SM_AUTH_REQ_BONDING       0x01  //!< Bonding
/** @} End SM_BONDING_FLAGS_DEFINES */

#define PASSKEY_LEN     6   //!< Passkey Character Length (ASCII Characters)

#define SM_AUTH_STATE_AUTHENTICATED       0x04  //!< Authenticate requested
#define SM_AUTH_STATE_BONDING             0x01  //!< Bonding requested
#define SM_AUTH_STATE_SECURECONNECTION    0x08  //!< Secure Connection requested

#define SM_ECC_KEY_LEN 32  //!< ECC Key length in bytes

/* SM private/public key regeneration policy */
#define SM_ECC_KEYS_NOT_AVAILABLE     0xFF  //!< Initial state of recycled keys before they exist.
#define SM_ECC_KEYS_REGNENERATE_NEVER 0xFF  //!< Never regenerate the keys.
#define SM_ECC_KEYS_REGENERATE_ALWAYS 0x00  //!< Always regenerate the keys.

/**
 * @defgroup SM_MESSAGE_EVENT_OPCODES SM Message opcocdes
 * @{
 */
#define SM_ECC_KEYS_EVENT 0x00 //!< ECC Keys
#define SM_DH_KEY_EVENT   0x01 //!< Diffie-Hellman key
/** @} End SM_MESSAGE_EVENT_OPCODES */

/** @} */ // end of GAPBondMgr_Constants

/*-------------------------------------------------------------------
 * General TYPEDEFS
 */

/**
 * SM_NEW_RAND_KEY_EVENT message format.  This message is sent to the
 * requesting task.
 */
typedef struct
{
  uint8_t newKey[KEYLEN];       //!< New key value - if status is SUCCESS
} smNewRandKeyEvent_t;

/**
 * header type for ECC and ECDH commands
 */
typedef struct
{
  uint8_t opcode;             //!< op code
} smEventHdr_t;

/**
 * SM_ECC_KEYS_EVENT message format for ECC keys.  This message is sent to
 * the request task.
 */
typedef struct
{
  uint8_t opcode;                     //!< SM_ECC_KEYS_EVENT
  uint8_t privateKey[SM_ECC_KEY_LEN]; //!< ECC private key.
  uint8_t publicKeyX[SM_ECC_KEY_LEN]; //!< ECC public key X-coordinate.
  uint8_t publicKeyY[SM_ECC_KEY_LEN]; //!< ECC public key Y-coordinate.
} smEccKeysEvt_t;

/**
 * SM_DH_KEY_EVENT message format for ECDH keys.
 * This message is sent to the request task.
 */
typedef struct
{
  uint8_t opcode;                //!< SM_DH_KEY_EVENT
  uint8_t dhKey[SM_ECC_KEY_LEN]; //!< ECC Diffie-Hellman key
} smDhKeyEvt_t;

/**
 * Key Distribution field  - True or False fields
 */
typedef struct keyDist_t
{
  unsigned int sEncKey:1;    //!< Set to distribute slave encryption key
  unsigned int sIdKey:1;     //!< Set to distribute slave identity key
  unsigned int sSign:1;      //!< Set to distribute slave signing key
  unsigned int sLinkKey:1;   //!< Set to derive slave link key from slave LTK
  unsigned int sReserved:4;  //!< Reserved for slave - don't use
  unsigned int mEncKey:1;    //!< Set to distribute master encryption key
  unsigned int mIdKey:1;     //!< Set to distribute master identity key
  unsigned int mSign:1;      //!< Set to distribute master signing key
  unsigned int mLinkKey:1;   //!< Set to derive master link key from master LTK
  unsigned int mReserved:4;  //!< Reserved for master - don't use
} keyDist_t;

/**
 * ECC keys for pairing.
 */
typedef struct
{
  uint8_t isUsed;               //!< FALSE if not used.  USE_PKEYS if public keys only.  USE_ALL_KEYS if public and private keys.
  uint8_t sK[SM_ECC_KEY_LEN];   //!< private key (only used if supplied by Bond Manager)
  uint8_t pK_x[SM_ECC_KEY_LEN]; //!< public key X-coordinate
  uint8_t pK_y[SM_ECC_KEY_LEN]; //!< public key Y-coordinate
} smEccKeys_t;

/**
 * Link Security Requirements
 */
typedef struct
{
  uint8_t ioCaps;                 //!< I/O Capabilities (ie.
  uint8_t oobAvailable;           //!< True if remote Out-of-band key available
  uint8_t oob[KEYLEN];            //!< Out-Of-Bounds key from remote device
  uint8_t oobConfirm[KEYLEN];     //!< Out-Of-Bounds confirm from remote device. Secure Connections only.
  uint8_t localOobAvailable;      //!< True if local Out-of-band key available. Secure Connections only.
  uint8_t localOob[KEYLEN];       //!< Out-Of-Bounds local data. Secure Connections only.
  uint8_t isSCOnlyMode;           //!< TRUE if Secure Connections Only Mode. Secure Connections only.
  smEccKeys_t eccKeys;          //!< Optionally specified ECC keys for pairing. Secure Connections only.
  uint8_t authReq;                //!< Authentication Requirements
  keyDist_t keyDist;            //!< Key Distribution mask
  uint8_t maxEncKeySize;          //!< Maximum Encryption Key size (7-16 bytes)
} smLinkSecurityReq_t;

/**
 * Link Security Information
 */
typedef struct
{
  uint8_t ltk[KEYLEN];              //!< Long Term Key (LTK)
  uint16_t div;                     //!< LTK Diversifier
  uint8_t rand[B_RANDOM_NUM_SIZE];  //!< LTK random number
  uint8_t keySize;                  //!< LTK Key Size (7-16 bytes)
} smSecurityInfo_t;

/**
 * Link Identity Information
 */
typedef struct
{
  uint8_t irk[KEYLEN];          //!< Identity Resolving Key (IRK)
  uint8_t addrType;             //!< Address type for BD_ADDR
  uint8_t bd_addr[B_ADDR_LEN];  //!< The advertiser may set this to zeroes to not disclose its BD_ADDR (public address).
} smIdentityInfo_t;

/**
 * Signing Information
 */
typedef struct
{
  uint8_t  srk[KEYLEN]; //!< Signature Resolving Key (CSRK)
  uint32_t signCounter; //!< Sign Counter
} smSigningInfo_t;

/**
 * Pairing Request & Response - authReq field
 */
typedef struct
{
  unsigned int bonding:2;    //!< Bonding flags
  unsigned int mitm:1;       //!< Man-In-The-Middle (MITM)
  unsigned int sc:1;         //!< LE Secure Connections (SC)
  unsigned int kp:1;         //!< LE SC Keypress Notifications
  unsigned int ct2:1;        //!< h7 Link Key conversion support (not used in LE)
  unsigned int reserved:2;   //!< Reserved - don't use
} authReq_t;

/**
 * Application callback to the SM for ECC keys and Diffie-Hellman Shared Secret (ECDH).
 */
typedef void (*smCB_t)
(
uint8_t *secretKey,  //!< Private key when ECC keys were requested.  ECDH shared secret when DHKey is requested.
uint8_t *publicKeyX, //!< Local Public Key X-Coordinate.  Not used when DHKey is returned.
uint8_t *publicKeyY  //!< Local Public Key Y-Coordinate when ECC keys were requested.  Not used when DHKey is returned.
);

/// @endcond //NODOC

/*-------------------------------------------------------------------
-------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SM_H */
