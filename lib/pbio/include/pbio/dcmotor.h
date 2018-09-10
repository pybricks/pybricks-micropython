#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbdrv/rawmotor.h>

typedef float float_t;

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (200)

/**
 * Motor direction convention.
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

/**
 * Settings for a Motor
 */
typedef struct _pbio_dcmotor_settings_t {
    pbio_motor_dir_t direction; /**< Whether or not polarity of duty cycle and encoder counter is inverted */
    int16_t max_stall_duty;     /**< Upper limit on duty cycle, which corresponds to a maximum torque while stalled. */
} pbio_dcmotor_settings_t;

pbio_dcmotor_settings_t dcmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

/**
 * Configure motor settings that should not be changed during runtime
 * @param [in]  port      ::The motor port
 * @param [in]  direction ::Whether or not polarity of duty cycle and encoder counter is inverted
 * @return                ::PBIO_SUCCESS if the call was successful,
 *                        ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                        ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                        ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_dcmotor_set_constant_settings(pbio_port_t port, pbio_motor_dir_t direction);

/**
 * Configure motor settings that should not be changed during runtime
 * @param [in]  port      ::The motor port
 * @param [in]  max_stall_duty  ::Soft limit on duty cycle
 * @return                ::PBIO_SUCCESS if the call was successful,
 *                        ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                        ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                        ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_dcmotor_set_variable_settings(pbio_port_t port, float_t stall_torque_limit);

pbio_error_t pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_dcmotor_coast(pbio_port_t port);

pbio_error_t pbio_dcmotor_brake(pbio_port_t port);

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int16_t duty_cycle_int);

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle);

#endif // _PBIO_DCMOTOR_H_