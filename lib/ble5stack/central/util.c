
#include "hci_tl.h"
#include "util.h"


HCI_StatusCodes_t Util_readLegoHwVersion(void)
{
    return HCI_sendHCICommand(HCI_UTIL_UNKNOWN1, NULL, 0);
}

HCI_StatusCodes_t Util_readLegoFwVersion(void)
{
    return HCI_sendHCICommand(HCI_UTIL_GET_LEGO_FW_VERSION, NULL, 0);
}

