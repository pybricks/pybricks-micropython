
/**
 * \addtogroup IOPortDriver I/O Port I/O driver
 * @{
 */

#ifndef _PBDRV_IOPORT_H_
#define _PBDRV_IOPORT_H_

#include "pbdrv/config.h"
#include "sys/process.h"

#if PBDRV_CONFIG_IOPORT

/** @cond INTERNAL */

PROCESS_NAME(pbdrv_ioport_process);

/** @endcond */

#endif // PBDRV_CONFIG_IOPORT

#endif // _PBDRV_IOPORT_H_

/** @}*/
