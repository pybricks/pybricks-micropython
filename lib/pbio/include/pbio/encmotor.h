#ifndef _PBIO_ENCMOTOR_H_
#define _PBIO_ENCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/dcmotor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

#define PID_PRESCALE (1000)
#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (200)
#define MS_PER_SECOND (1000)

/**
 * Settings for an encoded motor
 */
typedef struct _pbio_encmotor_settings_t {
    float_t counts_per_unit;        /**< Encoder counts per output unit. Counts per degree for rotational motors, counts per cm for a linear motor. */
    float_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train. Equals counts_per_unit*gear_ratio. */
    int16_t max_speed;              /**< Soft limit on the reference speed in all run commands */
    int16_t tolerance;              /**< Allowed deviation (deg) from target before motion is considered complete */
    int16_t acceleration_start;     /**< Acceleration when beginning to move. Positive value in degrees per second per second */
    int16_t acceleration_end;       /**< Deceleration when stopping. Positive value in degrees per second per second */
    int16_t tight_loop_time_ms;     /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    int32_t offset;                 /**< Virtual zero point of the encoder */
    int16_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    int16_t pid_ki;                 /**< Integral position control constant */
    int16_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
} pbio_encmotor_settings_t;

pbio_encmotor_settings_t encmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction, float_t gear_ratio);

pbio_error_t pbio_encmotor_set_settings(pbio_port_t port, float_t stall_torque_limit, float_t max_speed, float_t tolerance, float_t acceleration_start, float_t acceleration_end, float_t tight_loop_time, float_t pid_kp, float_t pid_ki, float_t pid_kd);

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count);

pbio_error_t pbio_motor_reset_encoder_count(pbio_port_t port, int32_t reset_count);

pbio_error_t pbio_motor_get_angle(pbio_port_t port, float_t *angle);

pbio_error_t pbio_motor_reset_angle(pbio_port_t port, float_t reset_angle);

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *encoder_rate);

pbio_error_t pbio_motor_get_angular_rate(pbio_port_t port, float_t *angular_rate);

/** @}*/

#endif // _PBIO_ENCMOTOR_H_
