
#ifndef _PBDRV_IOPORT_H_
#define _PBDRV_IOPORT_H_

/**
 * \addtogroup IOPortDriver I/O Port I/O driver
 * @{
 */

/**
 * Initializes the low level I/O port driver. This should be called only
 * once and must be called before using any other I/O port functions.
 */
void pbdrv_ioport_init(void);

/**
 * Do periodic background tasks. This should be called very 1ms.
 */
void pbdrv_ioport_poll(void);

/** @}*/

#endif // _PBDRV_IOPORT_H_
