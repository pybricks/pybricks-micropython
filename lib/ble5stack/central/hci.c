
#include "hci_tl.h"

HCI_StatusCodes_t HCI_readBdaddr()
{
    return HCI_sendHCICommand(HCI_READ_BDADDR, NULL, 0);
}

HCI_StatusCodes_t HCI_readLocalVersionInfo()
{
    return HCI_sendHCICommand(HCI_READ_LOCAL_VERSION_INFO, NULL, 0);
}
