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
#include "hci_tl.h"
#include "hal_defs.h"
#include "gatt.h"

HCI_StatusCodes_t GATT_WriteCharValue(GattWriteCharValue_t *para)
{
    uint8_t dataLength;
    uint8_t *pData;
    HCI_StatusCodes_t status;

    dataLength = sizeof(para->connHandle) + sizeof(para->handle) + para->dataSize;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(para->connHandle);
        pData[1] = HI_UINT16(para->connHandle);

        pData[2] = LO_UINT16(para->handle);
        pData[3] = HI_UINT16(para->handle);

        memcpy(&pData[4], para->value, para->dataSize);
    }

    status = HCI_sendHCICommand(GATT_WRITECHARVALUE, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_ExchangeMTU(uint16_t connHandle, attExchangeMTUReq_t *pReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(pReq->clientRxMTU);
    pData[3] = HI_UINT16(pReq->clientRxMTU);

    status = HCI_sendHCICommand(GATT_EXCHANGEMTU, pData, 4);

    return status;
}

HCI_StatusCodes_t GATT_DiscAllPrimaryServices(uint16_t connHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[2];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    status = HCI_sendHCICommand(GATT_DISCALLPRIMARYSERVICES, pData, 2);

    return status;
}

HCI_StatusCodes_t GATT_DiscPrimaryServiceByUUID( uint16_t connHandle, uint8_t *pUUID, uint8_t len)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        memcpy(&pData[2], pUUID, len);
    }

    status = HCI_sendHCICommand(GATT_DISCPRIMARYSERVICEBYUUID, pData, dataLength);

    free(pData);

    return status;
}


HCI_StatusCodes_t GATT_ReadCharValue(uint16_t connHandle, attReadReq_t *pReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(pReq->handle);
    pData[3] = HI_UINT16(pReq->handle);

    status = HCI_sendHCICommand(GATT_READCHARVALUE, pData, 4);

    return status;
}

HCI_StatusCodes_t GATT_FindIncludedServices(uint16_t connHandle,
                                           uint16_t startHandle,
                                           uint16_t endHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(startHandle);
    pData[3] = HI_UINT16(startHandle);

    pData[4] = LO_UINT16(endHandle);
    pData[5] = HI_UINT16(endHandle);

    status = HCI_sendHCICommand(GATT_FINDINCLUDEDSERVICES, pData, 6);

    return status;
}

HCI_StatusCodes_t GATT_DiscAllChars(uint16_t connHandle, uint16_t startHandle,
                                   uint16_t endHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(startHandle);
    pData[3] = HI_UINT16(startHandle);

    pData[4] = LO_UINT16(endHandle);
    pData[5] = HI_UINT16(endHandle);

    status = HCI_sendHCICommand(GATT_DISCALLCHARS, pData, 6);

    return status;
}

HCI_StatusCodes_t GATT_DiscCharsByUUID(uint16_t connHandle,
                                      attReadByTypeReq_t *pReq)
{
    uint8_t dataLength, index;
    uint8_t *pData;
    HCI_StatusCodes_t status;

    dataLength = sizeof(connHandle) + sizeof(pReq->startHandle)
            + sizeof(pReq->endHandle) + pReq->type.len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->startHandle);
        pData[3] = HI_UINT16(pReq->startHandle);

        pData[4] = LO_UINT16(pReq->endHandle);
        pData[5] = HI_UINT16(pReq->endHandle);

        for(index = 0; index < pReq->type.len; index++)
        {
            pData[6 + index] = pReq->type.uuid[(pReq->type.len - 1 - index)];
        }
    }

    status = HCI_sendHCICommand(GATT_DISCCHARSBYUUID, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_DiscAllCharDescs(uint16_t connHandle,
                                       uint16_t startHandle, uint16_t endHandle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(startHandle);
    pData[3] = HI_UINT16(startHandle);

    pData[4] = LO_UINT16(endHandle);
    pData[5] = HI_UINT16(endHandle);

    status = HCI_sendHCICommand(GATT_DISCALLCHARDESCS, pData, 6);

    return status;
}

HCI_StatusCodes_t GATT_ReadUsingCharUUID( uint16_t connHandle, attReadByTypeReq_t *pReq)
{
    uint8_t dataLength, index;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->startHandle) + sizeof(pReq->endHandle) + pReq->type.len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->startHandle);
        pData[3] = HI_UINT16(pReq->startHandle);

        pData[4] = LO_UINT16(pReq->endHandle);
        pData[5] = HI_UINT16(pReq->endHandle);

        for(index = 0; index < pReq->type.len; index++)
        {
            pData[6 + index] = pReq->type.uuid[(pReq->type.len - 1 - index)];
        }
    }

    status = HCI_sendHCICommand(GATT_READUSINGCHARUUID, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_ReadLongCharValue(uint16_t connHandle,
                                        attReadBlobReq_t *pReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(pReq->handle);
    pData[3] = HI_UINT16(pReq->handle);

    pData[4] = LO_UINT16(pReq->offset);
    pData[5] = HI_UINT16(pReq->offset);

    status = HCI_sendHCICommand(GATT_READLONGCHARVALUE, pData, 6);

    return status;
}

HCI_StatusCodes_t GATT_ReadMultiCharValues( uint16_t connHandle, attReadMultiReq_t *pReq)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + (pReq->numHandles * 2); // number of handles * two-byte handle sizes
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        memcpy(&pData[2], pReq->pHandles, (pReq->numHandles * 2));
    }

    status = HCI_sendHCICommand(GATT_READMULTILCHARVALUES, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_WriteNoRsp( uint16_t connHandle, attWriteReq_t *pReq )
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->handle) + pReq->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->handle);
        pData[3] = HI_UINT16(pReq->handle);

        memcpy(&pData[4], pReq->pValue, pReq->len);
    }

    status = HCI_sendHCICommand(GATT_WRITENORSP, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_SignedWriteNoRsp( uint16_t connHandle, attWriteReq_t *pReq )
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->handle) + pReq->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->handle);
        pData[3] = HI_UINT16(pReq->handle);

        memcpy(&pData[4], pReq->pValue, pReq->len);
    }

    status = HCI_sendHCICommand(GATT_SIGNEDWRITENORSP, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_WriteLongCharValue( uint16_t connHandle, attPrepareWriteReq_t *pReq)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->handle) + sizeof(pReq->offset) + pReq->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->handle);
        pData[3] = HI_UINT16(pReq->handle);

        pData[4] = LO_UINT16(pReq->offset);
        pData[5] = HI_UINT16(pReq->offset);

        memcpy(&pData[6], pReq->pValue, pReq->len);
    }

    status = HCI_sendHCICommand(GATT_WRITELONGCHARVALUE, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_ReadCharDesc(uint16_t connHandle, attReadReq_t *pReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(pReq->handle);
    pData[3] = HI_UINT16(pReq->handle);

    status = HCI_sendHCICommand(GATT_READCHARDESC, pData, 4);

    return status;
}

HCI_StatusCodes_t GATT_ReadLongCharDesc(uint16_t connHandle,
                                       attReadBlobReq_t *pReq)
{
    HCI_StatusCodes_t status;
    uint8_t pData[6];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(pReq->handle);
    pData[3] = HI_UINT16(pReq->handle);

    pData[4] = LO_UINT16(pReq->offset);
    pData[5] = HI_UINT16(pReq->offset);

    status = HCI_sendHCICommand(GATT_READLONGCHARDESC, pData, 6);

    return status;
}

HCI_StatusCodes_t GATT_WriteCharDesc( uint16_t connHandle, attWriteReq_t *pReq)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->handle) + pReq->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->handle);
        pData[3] = HI_UINT16(pReq->handle);

        memcpy(&pData[4], pReq->pValue, pReq->len);
    }

    status = HCI_sendHCICommand(GATT_WRITECHARDESC, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_WriteLongCharDesc( uint16_t connHandle, attPrepareWriteReq_t *pReq)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pReq->handle) + sizeof(pReq->offset) + pReq->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = LO_UINT16(pReq->handle);
        pData[3] = HI_UINT16(pReq->handle);

        pData[4] = LO_UINT16(pReq->offset);
        pData[5] = HI_UINT16(pReq->offset);

        memcpy(&pData[6], pReq->pValue, pReq->len);
    }

    status = HCI_sendHCICommand(GATT_WRITELONGCHARDESC, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_Indication( uint16_t connHandle, attHandleValueInd_t *pInd,
                                  uint8_t authenticated)
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(authenticated) + sizeof(pInd->handle) + pInd->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = authenticated;

        pData[3] = LO_UINT16(pInd->handle);
        pData[4] = HI_UINT16(pInd->handle);

        memcpy(&pData[5], pInd->pValue, pInd->len);
    }

    status = HCI_sendHCICommand(GATT_INDICATION, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_Notification( uint16_t connHandle, attHandleValueNoti_t *pNoti,
                                    uint8_t authenticated )
{
    uint8_t dataLength;
    HCI_StatusCodes_t status;
    uint8_t *pData;

    dataLength = sizeof(connHandle) + sizeof(pNoti->handle) + pNoti->len;
    pData = (uint8_t *) malloc(dataLength);
    if (pData == NULL)
    {
        free(pData);
        return bleMemAllocError;
    }
    else
    {
        pData[0] = LO_UINT16(connHandle);
        pData[1] = HI_UINT16(connHandle);

        pData[2] = authenticated;

        pData[3] = LO_UINT16(pNoti->handle);
        pData[4] = HI_UINT16(pNoti->handle);

        memcpy(&pData[5], pNoti->pValue, pNoti->len);
    }

    status = HCI_sendHCICommand(GATT_NOTIFICATION, pData, dataLength);

    free(pData);

    return status;
}

HCI_StatusCodes_t GATT_AddService( uint16_t uuid, uint16_t numAttrs,
                                    uint8_t encKeySize )
{
    HCI_StatusCodes_t status;
    uint8_t pData[5];

    pData[0] = LO_UINT16(uuid);
    pData[1] = HI_UINT16(uuid);

    pData[2] = LO_UINT16(numAttrs);
    pData[3] = HI_UINT16(numAttrs);

    pData[4] = encKeySize;

    status = HCI_sendHCICommand(GATT_ADDSERVICE, pData, 5);

    return status;
}

HCI_StatusCodes_t GATT_DelService(uint16_t handle)
{
    HCI_StatusCodes_t status;
    uint8_t pData[2];

    pData[0] = LO_UINT16(handle);
    pData[1] = HI_UINT16(handle);

    status = HCI_sendHCICommand(GATT_DELSERVICE, pData, 2);

    return status;
}

HCI_StatusCodes_t GATT_AddAttribute( uint16_t uuid, uint8_t permissions )
{
    HCI_StatusCodes_t status;
    uint8_t pData[3];

    pData[0] = LO_UINT16(uuid);
    pData[1] = HI_UINT16(uuid);

    pData[2] = permissions;

    status = HCI_sendHCICommand(GATT_ADDATTRIBUTE, pData, 3);

    return status;
}

HCI_StatusCodes_t GATT_AddAttribute2(const uint8_t *uuid128, uint8_t permissions)
{
    HCI_StatusCodes_t status;
    uint8_t pData[17];

    memcpy(pData, uuid128, 16);
    pData[16] = permissions;

    status = HCI_sendHCICommand(GATT_ADDATTRIBUTE, pData, 17);

    return status;
}

HCI_StatusCodes_t GATT_UpdateMTU(uint16_t connHandle, uint16_t mtuSize)
{
    HCI_StatusCodes_t status;
    uint8_t pData[4];

    pData[0] = LO_UINT16(connHandle);
    pData[1] = HI_UINT16(connHandle);

    pData[2] = LO_UINT16(mtuSize);
    pData[3] = HI_UINT16(mtuSize);

    status = HCI_sendHCICommand(GATT_UPDATEMTU, pData, 4);

    return status;
}
