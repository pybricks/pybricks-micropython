
#ifndef _PBDRV_IOPORT_H_
#define _PBDRV_IOPORT_H_

/**
 * \addtogroup IOPortDriver I/O Port I/O driver
 * @{
 */

/** @cond INTERNAL */

/**
 * Initializes the low level I/O port driver. This should be called only
 * once and must be called before using any other I/O port functions.
 */
void _pbdrv_ioport_init(void);

/**
 * Do periodic background tasks. This should be called very 1ms.
 * @param [in] now  the current time in millisecond ticks
 */
void _pbdrv_ioport_poll(uint32_t now);

/** @endcond */

/** @}*/

#endif // _PBDRV_IOPORT_H_
