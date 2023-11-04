
#include <string.h>

#include "hci_tl.h"

HCI_StatusCodes_t HCI_readBdaddr(void)
{
    return HCI_sendHCICommand(HCI_READ_BDADDR, NULL, 0);
}

HCI_StatusCodes_t HCI_readLocalVersionInfo(void)
{
    return HCI_sendHCICommand(HCI_READ_LOCAL_VERSION_INFO, NULL, 0);
}

HCI_StatusCodes_t HCI_LE_readAdvertisingChannelTxPower(void)
{
    return HCI_sendHCICommand(HCI_LE_READ_ADVERTISING_CHANNEL_TX_POWER, NULL, 0);
}

HCI_StatusCodes_t HCI_LE_setAdvertisingData(uint8_t len, uint8_t *data)
{
    uint8_t pData[32];

    pData[0] = len;
    memcpy(&pData[1], data, len);

    return HCI_sendHCICommand(HCI_LE_SET_ADVERTISING_DATA, pData, len + 1);
}
