#ifndef UTIL_H
#define UTIL_H

#include "hci_tl.h"

#define HCI_UTIL_UNKNOWN1               0xFE87
#define HCI_UTIL_GET_LEGO_FW_VERSION    0xFE88

HCI_StatusCodes_t Util_readLegoHwVersion(void);
HCI_StatusCodes_t Util_readLegoFwVersion(void);

#endif // UTIL_H
