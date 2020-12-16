
#include "hci_tl.h"

HCI_StatusCodes_t HCI_readBdaddr(void)
{
    return HCI_sendHCICommand(HCI_READ_BDADDR, NULL, 0);
}

HCI_StatusCodes_t HCI_readLocalVersionInfo(void)
{
    return HCI_sendHCICommand(HCI_READ_LOCAL_VERSION_INFO, NULL, 0);
}
