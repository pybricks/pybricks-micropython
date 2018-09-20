
#ifndef _PBDRV_TIME_H_
#define _PBDRV_TIME_H_

#include <stdint.h>

/**
 * \addtogroup TimeDriver Time I/O driver
 * @{
 */

/**
 * Gets the time with units of milliseconds. The time value has no specific
 * starting point and should only be used to measure durations of less than
 * 2^32 milliseconds, or about 7 weeks.
 * @return  Current time with units of milliseconds
 */
uint32_t pbdrv_time_get_msec();

/**
 * Gets the time with units of microseconds. The time value has no specific
 * starting point and should only be used to measure durations of less than
 * 2^32 microseconds, or about an hour.
 * @return  Current time with units of microseconds
 */
uint32_t pbdrv_time_get_usec();

/** @}*/

#endif // _PBDRV_TIME_H_
