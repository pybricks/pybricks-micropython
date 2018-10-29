
/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include "pbdrv/config.h"
#include "sys/process.h"

#if PBDRV_CONFIG_BLUETOOTH

/** @cond INTERNAL */

PROCESS_NAME(pbdrv_bluetooth_hci_process);
PROCESS_NAME(pbdrv_bluetooth_spi_process);

/** @endcond */

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @}*/
