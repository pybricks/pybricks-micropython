
/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "sys/process.h"

#if PBDRV_CONFIG_BLUETOOTH

/**
 * Get one character from the nRF UART service Rx buffer.
 * @param [out] c       The character read
 * @return              ::PBIO_SUCCESS if a character was available, or
 *                      ::PBIO_ERROR_AGAIN if no character was available to be
 *                      read at this time.
 */
pbio_error_t pbdrv_bluetooth_get_char(uint8_t *c);

/** @cond INTERNAL */

PROCESS_NAME(pbdrv_bluetooth_hci_process);
PROCESS_NAME(pbdrv_bluetooth_spi_process);

/** @endcond */

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @}*/
