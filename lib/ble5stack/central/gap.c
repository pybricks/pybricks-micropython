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

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "gap.h"
#include "gap_scanner.h"
#include "gap_initiator.h"

HCI_StatusCodes_t GAP_deviceInit(uint8_t profileRole, uint8_t maxScanResponses,
                                 uint8_t *irk, uint8_t *csrk, uint32_t signCounter)
{
    uint8_t pData[38];

    pData[0] = profileRole;
    pData[1] = maxScanResponses;
    if (irk) {
        memcpy(&pData[2], irk, 16);
    }
    else {
        memset(&pData[2], 0, 16);
    }
    if (csrk) {
        memcpy(&pData[18], csrk, 16);
    }
    else {
        memset(&pData[18], 0, 16);
    }
    pData[34] = signCounter & 0xFF;
    pData[35] = (signCounter >> 8) & 0xFF;
    pData[36] = (signCounter >> 16) & 0xFF;
    pData[37] = (signCounter >> 24) & 0xFF;

    return HCI_sendHCICommand(GAP_DEVICE_INIT, pData, 38);
}

HCI_StatusCodes_t GAP_makeDiscoverable(Gap_eventType_t eventType,
                                       Gap_initiatorAddrType_t initiatorAddrType,
                                       uint8_t *initiatorAddr,
                                       Gap_channelMap_t channelMap,
                                       Gap_filterPolicy_t filterPolicy)
{
    uint8_t pData[10];

    pData[0] = eventType;
    pData[1] = initiatorAddrType;
    if (initiatorAddr) {
        memcpy(&pData[2], initiatorAddr, 6);
    }
    else {
        memset(&pData[2], 0, 6);
    }
    pData[8] = channelMap;
    pData[9] = filterPolicy;

    return HCI_sendHCICommand(GAP_MAKE_DISCOVERABLE, pData, 10);
}

HCI_StatusCodes_t GAP_endDiscoverable(void)
{
    return HCI_sendHCICommand(GAP_END_DISCOVERABLE, NULL, 0);
}

HCI_StatusCodes_t GAP_updateAdvertistigData(Gap_adType_t adType, uint8_t dataLen,
                                            uint8_t *advertData)
{
    uint8_t pData[33]; // dataLen must be 1 to 31

    pData[0] = adType;
    pData[1] = dataLen;
    memcpy(&pData[2], advertData, dataLen);

    return HCI_sendHCICommand(GAP_UPDATE_ADVERTISING_DATA, pData, 2 + dataLen);
}

HCI_StatusCodes_t GapScan_Enable(uint16_t period, uint16_t duration,
                                 uint8_t maxNumReport)
{
    HCI_StatusCodes_t status;
    uint8_t pData[5];

    pData[0] = LO_UINT16(period);
    pData[1] = HI_UINT16(period);

    pData[2] = LO_UINT16(duration);
    pData[3] = HI_UINT16(duration);

    pData[4] = maxNumReport;

    status = HCI_sendHCICommand(GAPSCAN_ENABLE, pData, 5);

    return status;
}

HCI_StatusCodes_t GapInit_connect(GAP_Peer_Addr_Types_t peerAddrType,
                         uint8_t* pPeerAddress, uint8_t phys, uint16_t timeout)
{
    HCI_StatusCodes_t status;
    uint8_t pData[10];
    uint8_t index;

    pData[0] = peerAddrType;

    for (index = 0; index < DEFAULT_ADDRESS_SIZE; index++)
    {
        pData[1 + index] = *pPeerAddress;
        pPeerAddress++;
    }

    pData[7] = phys;

    pData[8] = LO_UINT16(timeout);
    pData[9] = HI_UINT16(timeout);

    status = HCI_sendHCICommand(GAPINIT_CONNECT, pData, 10);

    return status;
}

HCI_StatusCodes_t GapInit_connectWl(uint8_t phys, uint16_t timeout)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = phys;

    pData[1] = LO_UINT16(timeout);
    pData[2] = HI_UINT16(timeout);

    status = HCI_sendHCICommand(GAPINIT_CONNECTWL, pData, 3);

    return status;
}

HCI_StatusCodes_t GapInit_cancelConnect(void)
{
    HCI_StatusCodes_t status;

    status = HCI_sendHCICommand(GAPINIT_CANCELCONNECT, NULL, 0);

    return status;
}

HCI_StatusCodes_t GAP_ConfigDeviceAddr(Gap_initiatorAddrType_t addrType, const uint8_t *Addr)
{
    uint8_t pData[7];

    pData[0] = addrType;

    if (Addr) {
        memcpy(&pData[1], Addr, 6);
    } else {
        memset(&pData[1], 0, 6);
    }

    return HCI_sendHCICommand(GAP_CONFIG_DEVICE_ADDR, pData, 7);
}

HCI_StatusCodes_t GAP_DeviceDiscoveryRequest(Gap_deviceDiscoveryMode_t mode, uint8_t activeScan, Gap_filterPolicy_t filterPolicy)
{
    uint8_t pData[3];

    pData[0] = mode;
    pData[1] = activeScan;
    pData[2] = filterPolicy;

    return HCI_sendHCICommand(GAP_DEVICE_DISCOVERY_REQUEST, pData, 3);
}

HCI_StatusCodes_t GAP_DeviceDiscoveryCancel(void)
{
    return HCI_sendHCICommand(GAP_DEVICE_DISCOVERY_CANCEL, NULL, 0);
}

HCI_StatusCodes_t GAP_EstablishLinkReq(uint8_t highDutyCycle, uint8_t whiteList, GAP_Addr_Types_t addrTypePeer, uint8_t *peerAddr)
{
    uint8_t pData[9];

    pData[0] = highDutyCycle;
    pData[1] = whiteList;
    pData[2] = addrTypePeer;
    memcpy(&pData[3], peerAddr, 6);

    return HCI_sendHCICommand(GAP_ESTABLISH_LINK_REQUEST, pData, 9);
}

HCI_StatusCodes_t GAP_TerminateLinkReq(uint16_t connectionHandle, uint8_t reason)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    pData[2] = reason;

    status = HCI_sendHCICommand(GAP_TERMINATE_LINK_REQUEST, pData, 3);

    return status;
}

HCI_StatusCodes_t GapScan_Disable(void)
{
    HCI_StatusCodes_t status;

    status = HCI_sendHCICommand(GAPSCAN_DISABLE, NULL, 0);

    return status;
}

HCI_StatusCodes_t GapConfig_SetParameter(Gap_configParamIds_t param,
                                         void *pValue)
{
    HCI_StatusCodes_t status;
    uint8_t i;
    uint8_t pData[17];

    pData[0] = param;

    for(i = 0; i < KEYLEN; i++)
    {
        pData[1 + i] = *((uint8_t*)pValue);
        pValue = (uint8_t*)pValue + sizeof(uint8_t);
    }

    status = HCI_sendHCICommand(GAPCONFIG_SETPARAMETER, pData, 17);

    return status;
}

HCI_StatusCodes_t GapScan_setEventMask(GapScan_EventMask_t eventMask)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = BREAK_UINT32(eventMask, 0);
    pData[1] = BREAK_UINT32(eventMask, 1);
    pData[2] = BREAK_UINT32(eventMask, 2);
    pData[3] = BREAK_UINT32(eventMask, 3);

    status = HCI_sendHCICommand(GAPSCAN_SETEVENTMASK, pData, 4);

    return status;
}

HCI_StatusCodes_t GapScan_getAdvReport(uint8_t rptIdx)
{
    HCI_StatusCodes_t status;
    uint8_t pData[1];

    pData[0] = rptIdx;

    status = HCI_sendHCICommand(GAPSCAN_GETADVREPORT, pData, 1);

    return status;
}

HCI_StatusCodes_t GAP_SetParamValue(uint8_t paramID, uint16_t paramValue)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = paramID;

    pData[1] = LO_UINT16(paramValue);
    pData[2] = HI_UINT16(paramValue);

    status = HCI_sendHCICommand(GAP_SETPARAMVALUE, pData, 3);

    return status;
}

HCI_StatusCodes_t GAP_GetParamValue(uint8_t paramID)
{
    HCI_StatusCodes_t status;
    uint8_t pData[1];

    pData[0] = paramID;

    status = HCI_sendHCICommand(GAP_GETPARAMVALUE, pData, 1);

    return status;
}

extern HCI_StatusCodes_t GAP_UpdateLinkParamReq(gapUpdateLinkParamReq_t *pParams)
{
    HCI_StatusCodes_t status;
    uint8_t pData[10];

    pData[0] = LO_UINT16(pParams->connectionHandle);
    pData[1] = HI_UINT16(pParams->connectionHandle);

    pData[2] = LO_UINT16(pParams->intervalMin);
    pData[3] = HI_UINT16(pParams->intervalMin);

    pData[4] = LO_UINT16(pParams->intervalMax);
    pData[5] = HI_UINT16(pParams->intervalMax);

    pData[6] = LO_UINT16(pParams->connLatency);
    pData[7] = HI_UINT16(pParams->connLatency);

    pData[8] = LO_UINT16(pParams->connTimeout);
    pData[9] = HI_UINT16(pParams->connTimeout);

    status = HCI_sendHCICommand(GAP_UPDATELINKPARAMREQ, pData, 10);

    return status;
}

HCI_StatusCodes_t GAP_UpdateLinkParamReqReply(gapUpdateLinkParamReqReply_t *pParams)
{
    HCI_StatusCodes_t status;
    uint8_t pData[12];

    pData[0] = LO_UINT16(pParams->connectionHandle);
    pData[1] = HI_UINT16(pParams->connectionHandle);

    pData[2] = LO_UINT16(pParams->intervalMin);
    pData[3] = HI_UINT16(pParams->intervalMin);

    pData[4] = LO_UINT16(pParams->intervalMax);
    pData[5] = HI_UINT16(pParams->intervalMax);

    pData[6] = LO_UINT16(pParams->connLatency);
    pData[7] = HI_UINT16(pParams->connLatency);

    pData[8] = LO_UINT16(pParams->connTimeout);
    pData[9] = HI_UINT16(pParams->connTimeout);

    pData[10] = pParams->signalIdentifier;

    pData[11] = pParams->accepted;

    status = HCI_sendHCICommand(GAP_UPDATELINKPARAMREQREPLY, pData, 12);

    return status;
}


HCI_StatusCodes_t GapInit_getPhyParam(uint8_t phy, GapInit_PhyParamId_t paramId)
{
    HCI_StatusCodes_t status;
    uint8_t pData[2];

    pData[0] = phy;

    pData[1] = paramId;

    status = HCI_sendHCICommand(GAPINIT_GETPHYPARAM, pData, 2);

    return status;
}

HCI_StatusCodes_t GapScan_disable(void)
{
    HCI_StatusCodes_t status;

    status = HCI_sendHCICommand(GAPSCAN_DISABLE, NULL, 0);

    return status;
}

HCI_StatusCodes_t GapScan_setPhyParams(uint8_t primPhys,
                                          GapScan_ScanType_t type,
                                          uint16_t interval,
                                          uint16_t window)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = primPhys;

    pData[1] = type;

    pData[2] = LO_UINT16(interval);
    pData[3] = HI_UINT16(interval);

    pData[4] = LO_UINT16(window);
    pData[5] = HI_UINT16(window);

    status = HCI_sendHCICommand(GAPSCAN_SETPHYPARAMS, pData, 6);

    return status;
}

HCI_StatusCodes_t GapScan_getPhyParams(uint8_t primPhy)
{
    HCI_StatusCodes_t status;
    uint8_t pData[1];

    pData[0] = primPhy;

    status = HCI_sendHCICommand(GAPSCAN_GETPHYPARAMS, pData, 1);

    return status;
}

HCI_StatusCodes_t GapScan_setParam(GapScan_ParamId_t paramId, void* pValue)
{
    HCI_StatusCodes_t status;

    if (paramId == SCAN_PARAM_FLT_PDU_TYPE || paramId == SCAN_PARAM_RPT_FIELDS)
    {
        uint8_t pData[3];

        pData[0] = paramId;

        pData[1] = LO_UINT16(*(uint16_t*)pValue);
        pData[2] = HI_UINT16(*(uint16_t*)pValue);

        status = HCI_sendHCICommand(GAPSCAN_SETPARAM, pData, 3);
    }
    else
    {
        uint8_t pData[2];

        pData[0] = paramId;

        pData[1] = *(uint8_t*)pValue;

        status = HCI_sendHCICommand(GAPSCAN_SETPARAM, pData, 2);
    }

    return status;
}

HCI_StatusCodes_t GapScan_getParam(GapScan_ParamId_t paramId)
{
    HCI_StatusCodes_t status;
    uint8_t pData[1];

    pData[0] = paramId;

    status = HCI_sendHCICommand(GAPSCAN_GETPARAM, pData, 1);

    return status;
}


HCI_StatusCodes_t GapInit_setPhyParam(uint8_t phys,
                                      GapInit_PhyParamId_t paramId,
                                      uint16_t value)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = phys;

    pData[1] = paramId;

    pData[2] = LO_UINT16(value);
    pData[3] = HI_UINT16(value);

    status = HCI_sendHCICommand(GAPINIT_SETPHYPARAM, pData, 4);

    return status;
}

HCI_StatusCodes_t GAP_TerminateAuth(uint16_t connectionHandle, uint8_t reason)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    pData[2] = reason;

    status = HCI_sendHCICommand(GAP_TERMINATEAUTH, pData, 3);

    return status;
}

HCI_StatusCodes_t GAP_PasskeyUpdate(uint8_t *pPasskey, uint16_t connectionHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[8];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    memcpy(&pData[2], pPasskey, DEFAULT_PASSKEY_SIZE);

    status = HCI_sendHCICommand(GAP_PASSKEYUPDATE, pData, 8);

    return status;
}

HCI_StatusCodes_t GAP_Signable(uint16_t connectionHandle, uint8_t authenticated,
                              smSigningInfo_t *pParams)
{
    HCI_StatusCodes_t status;
    uint8_t pData[23];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    pData[2] = authenticated;

    memcpy(&pData[3], &pParams->srk, KEYLEN);

    memcpy(&pData[19], &pParams->signCounter, sizeof(pParams->signCounter));

    status = HCI_sendHCICommand(GAP_SIGNABLE, pData, 23);

    return status;
}

HCI_StatusCodes_t GAP_Bond(uint16_t connectionHandle, uint8_t authenticated,
                          uint8_t secureConnections, smSecurityInfo_t *pParams,
                          uint8_t startEncryption)
{
    HCI_StatusCodes_t status;
    uint8_t pData[31];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    pData[2] = authenticated;

    pData[3] = secureConnections;

    memcpy(&pData[4], &pParams->ltk, KEYLEN);

    pData[20] = LO_UINT16(pParams->div);
    pData[21] = HI_UINT16(pParams->div);

    memcpy(&pData[22], &pParams->rand, B_RANDOM_NUM_SIZE);

    pData[30] = pParams->keySize;

    status = HCI_sendHCICommand(GAP_BOND, pData, 31);

    return status;
}

HCI_StatusCodes_t Gap_RegisterConnEvent(GAP_CB_Action_t action,
                                       uint16_t connHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = action;

    pData[1] = LO_UINT16(connHandle);
    pData[2] = HI_UINT16(connHandle);

    status = HCI_sendHCICommand(GAP_REGISTERCONNEVENT, pData, 3);

    return status;
}

HCI_StatusCodes_t GAP_SendSlaveSecurityRequest(uint16_t connectionHandle,
                                              uint8_t authReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = LO_UINT16(connectionHandle);
    pData[1] = HI_UINT16(connectionHandle);

    pData[2] = authReq;

    status = HCI_sendHCICommand(GAP_SENDSLAVESECURITYREQUEST, pData, 3);

    return status;
}

HCI_StatusCodes_t GAP_BondMgrSetParameter(uint16_t paramID, uint8_t paramDataLen, uint8_t *paramData)
{
    uint8_t pData[32];

    pData[0] = LO_UINT16(paramID);
    pData[1] = HI_UINT16(paramID);
    pData[2] = paramDataLen;
    memcpy(&pData[3], paramData, paramDataLen);

    return HCI_sendHCICommand(GAP_BOND_MGR_SET_PARAMETER, pData, paramDataLen + 3);
}
