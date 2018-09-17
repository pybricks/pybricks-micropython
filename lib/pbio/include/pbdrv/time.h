
#ifndef _PBDRV_TIME_H_
#define _PBDRV_TIME_H_

#include <stdint.h>

/**
 * \addtogroup TimeDriver Time I/O driver
 * @{
 */

/**
 * Gets the time with units of microseconds. The time value has no specific starting point and should only be used to 
 * measure durations of processess less than 2^32 microseconds, or about an hour.
 * @param [in]  time    Pointer to time variable to be set
 * @return              ::Time with units of microseconds
 */
uint32_t pbdrv_get_time_usec();

/** @}*/

#endif // _PBDRV_TIME_H_
