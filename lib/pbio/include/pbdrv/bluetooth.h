
/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include <stdint.h>

#include <pbdrv/config.h>


#if PBDRV_CONFIG_BLUETOOTH

/** @cond INTERNAL */

/**
 * Initializes the low level bluetooth driver. This should be called only
 * once and must be called before using any other bluetooth functions.
 */
void _pbdrv_bluetooth_init(void);

void _pbdrv_bluetooth_poll(uint32_t now);

/**
 * Releases the low level bluetooth driver. No bluetooth functions can be called after
 * calling this function.
 */
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_bluetooth_deinit(void);
#else
static inline void _pbdrv_bluetooth_deinit(void) { }
#endif

/** @endcond */

#else // PBDRV_CONFIG_BLUETOOTH

static inline void _pbdrv_bluetooth_init(void) { }
static inline void _pbdrv_bluetooth_poll(uint32_t now) { }
static inline void _pbdrv_bluetooth_deinit(void) { }

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @}*/
