
#ifndef _PBDRV_MOTOR_H_
#define _PBDRV_MOTOR_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

/**
 * \addtogroup MotorDriver Motor I/O driver
 * @{
 */

/**
 * Motor constants across devices
 */
#define PBIO_MAX_DUTY (10000)
#define PBIO_MAX_DUTY_PCT (100.0)
#define PBIO_DUTY_PCT_TO_ABS (PBIO_MAX_DUTY/PBIO_MAX_DUTY_PCT)

/** @cond INTERNAL */

/**
 * Initializes the low level motor driver. This should be called only once and
 * must be called before using any other motor functions.
 */
void _pbdrv_motor_init(void);

/**
 * Releases the low level motor driver. No motor functions can be called after
 * calling this function.
 */
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void);
#else
static inline void _pbdrv_motor_deinit(void) { }
#endif

/** @endcond */

/**
 * Gets the tachometer encoder count.
 * @param [in]  port    The motor port
 * @param [out] count   The count
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_get_encoder_count(pbio_port_t port, int32_t *count);

/**
 * Gets the tachometer encoder rate in counts per second.
 * @param [in]  port    The motor port
 * @param [out] rate    The rate
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_get_encoder_rate(pbio_port_t port, int32_t *rate);

/**
 * Instructs the motor to coast freely.
 * @param [in]  port    The motor port
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_coast(pbio_port_t port);

/**
 * Sets the PWM duty cycle for the motor. Setting a duty cycle of 0 will "brake" the motor.
 * @param [in]  port        The motor port
 * @param [in]  duty_cycle  The duty cycle -10000 to 10000
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if duty_cycle is out of range
 *                          ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle);

/** @}*/

#endif // _PBDRV_MOTOR_H_
