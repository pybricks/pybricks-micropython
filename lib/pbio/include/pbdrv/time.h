
#ifndef _PBDRV_TIME_H_
#define _PBDRV_TIME_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * \addtogroup TimeDriver Time I/O driver
 * @{
 */

/**
 * Gets the time with units of microseconds. The time value has no specific starting point and should only be used to 
 * measure durations of processess less than 2^32 microseconds, or about an hour.
 * @param [in]  time    Pointer to time variable to be set
 * @return              ::PBIO_SUCCESS if the call was successful
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_get_time_usec(uint32_t *time);

/** @}*/

#endif // _PBDRV_TIME_H_
