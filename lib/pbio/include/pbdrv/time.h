
#ifndef _PBDRV_TIME_H_
#define _PBDRV_TIME_H_

#include <stdint.h>

#include <pbdrv/config.h>

/**
 * \addtogroup TimeDriver Time I/O driver
 * @{
 */

#if PBDRV_CONFIG_TIME

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

/**
 * Sleep for the given amount of milliseconds.
 */
void pbdrv_time_sleep_msec(uint32_t duration);

/**
 * Delay for the given amount of microseconds. On most platforms, this will
 * busy-wait and block anything else from running, so it should only be used
 * for very short durations. On Linux it will actually sleep (allow other
 * processes/threads to run) and cannot be depended upon for accurate timing.
 * @param [in] duration     number of microseconds to wait before returning
 */
void pbdrv_time_delay_usec(uint32_t duration);

#else

static inline uint32_t pbdrv_time_get_msec() { return 0; }
static inline uint32_t pbdrv_time_get_usec() { return 0; }
static inline pbdrv_time_sleep_msec(uint32_t duration) { }
static inline pbdrv_time_delay_usec(uint32_t duration) { }

#endif

/** @}*/

#endif // _PBDRV_TIME_H_
