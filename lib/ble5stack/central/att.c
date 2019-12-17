
#include <stdint.h>
#include <string.h>

#include "att.h"
#include "hci_tl.h"

bStatus_t ATT_ErrorRsp(uint16_t connHandle, attErrorRsp_t *pRsp) {
    uint8_t buf[6];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    buf[2] = pRsp->reqOpcode;
    buf[3] = pRsp->handle & 0xFF;
    buf[4] = (pRsp->handle >> 8) & 0xFF;
    buf[5] = pRsp->errCode;

    return HCI_sendHCICommand(ATT_CMD_ERROR_RSP, buf, 6);
}

bStatus_t ATT_ExchangeMTURsp(uint16_t connHandle, attExchangeMTURsp_t *pRsp) {
    uint8_t buf[4];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    buf[2] = pRsp->serverRxMTU & 0xFF;
    buf[3] = (pRsp->serverRxMTU >> 8) & 0xFF;

    return HCI_sendHCICommand(ATT_CMD_EXCHANGE_MTU_RSP, buf, 4);
}

bStatus_t ATT_ReadByTypeRsp(uint16_t connHandle, attReadByTypeRsp_t *pRsp) {
    uint8_t buf[32];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    buf[2] = pRsp->dataLen;
    memcpy(&buf[3], pRsp->pDataList, pRsp->dataLen);

    return HCI_sendHCICommand(ATT_CMD_READ_BY_TYPE_VALUE_RSP, buf, 3 + pRsp->dataLen);
}

bStatus_t ATT_ReadRsp(uint16_t connHandle, attReadRsp_t *pRsp) {
    uint8_t buf[32];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    memcpy(&buf[2], pRsp->pValue, pRsp->len);

    return HCI_sendHCICommand(ATT_CMD_READ_RSP, buf, 2 + pRsp->len);
}


bStatus_t ATT_ReadByGrpTypeRsp(uint16_t connHandle, attReadByGrpTypeRsp_t *pRsp) {
    uint8_t buf[32];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    buf[2] = pRsp->dataLen;
    memcpy(&buf[3], pRsp->pDataList, pRsp->dataLen);

    return HCI_sendHCICommand(ATT_CMD_READ_BY_GRP_TYPE_RSP, buf, 3 + pRsp->dataLen);
}

bStatus_t ATT_WriteRsp(uint16_t connHandle) {
    uint8_t buf[2];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;

    return HCI_sendHCICommand(ATT_CMD_WRITE_RSP, buf, 2);
}

bStatus_t ATT_HandleValueNoti(uint16_t connHandle, attHandleValueNoti_t *pNoti) {
    uint8_t buf[32];

    buf[0] = connHandle & 0xFF;
    buf[1] = (connHandle >> 8) & 0xFF;
    buf[2] = 0; // authenticated link not required
    buf[3] = pNoti->handle & 0xFF;
    buf[4] = (pNoti->handle >> 8) & 0xFF;
    memcpy(&buf[5], pNoti->pValue, pNoti->len);

    return HCI_sendHCICommand(ATT_CMD_HANDLE_VALUE_NOTI, buf, 5 + pNoti->len);
}
