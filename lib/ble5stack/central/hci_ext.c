#include <stdint.h>
#include <string.h>

#include "hci_ext.h"
#include "hci_tl.h"

/**
 * This command is used to set the RF transmitter output power.
 * @power: one of HCI_EXT_CC254X_TX_POWER_* or HCI_EXT_CC26XX_TX_POWER_*
 */
HCI_StatusCodes_t HCI_EXT_setTxPower(uint8_t power)
{
    uint8_t buf[1];

    buf[0] = power;

    return HCI_sendHCICommand(HCI_EXT_SET_TX_POWER, buf, 1);
}

/**
 * This command is used to set this devices BLE address (BDADDR).
 * @bdaddr: The 6-byte address in little-endian order.
 */
HCI_StatusCodes_t HCI_EXT_setBdaddr(const uint8_t *bdaddr)
{
    uint8_t buf[6];

    memcpy(buf, bdaddr, 6);

    return HCI_sendHCICommand(HCI_EXT_SET_BDADDR, buf, 6);
}

/**
 * This command is used to set the Controller’s Local Supported Features. For
 * a complete list of supported LE features, please see [1], Part B, Section
 * 4.6.
 *
 * Note: This command can be issued either before or after one or more
 * connections are formed. However, the local features set in this manner are
 * only effective if performed beforea Feature Exchange Procedure has been
 * initiated by the Master. Once this control procedure has been completed for
 * a particular connection, only the exchanged feature set for that connection
 * will be used.Since the Link Layer may initiate the feature exchange procedure
 * autonomously, it is best to use this command before the connection is formed.
 */
HCI_StatusCodes_t HCI_EXT_setLocalSupportedFeatures(const uint32_t localFeatures)
{
    uint8_t buf[8];

    // techically, localFeatures should be uint64_t, but only the lowest bits
    // are currently used, so we use uint32_t to save on code size.
    buf[0] = localFeatures & 0xFF;
    buf[1] = (localFeatures >> 8) & 0xFF;
    buf[2] = (localFeatures >> 16) & 0xFF;
    buf[3] = (localFeatures >> 24) & 0xFF;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    return HCI_sendHCICommand(HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES, buf, 8);
}
