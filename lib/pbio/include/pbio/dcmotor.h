#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/iodev.h>
#include <pbdrv/motor.h>

typedef float float_t;

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (200)

/**
 * Motor direction convention
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

/**
 * Motor control active state
 */
typedef enum {
    PBIO_MOTOR_CONTROL_PASSIVE, /**< Motor is coasting, braking, or set to a duty value by the user. */
    PBIO_MOTOR_CONTROL_RUNNING, /**< Motor busy executing command: Firmware repeatedly sets duty cycle to control motor speed and position for a desired trajectory*/
    PBIO_MOTOR_CONTROL_DONE,    /**< Motor is holding position or speed after completing command: Firmware repeatedly sets duty cycle to keep constant position OR speed */
} pbio_motor_control_active_t;

pbio_motor_control_active_t motor_control_active[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

/**
 * Settings for a DC Motor
 */
typedef struct _pbio_dcmotor_settings_t {
    pbio_motor_dir_t direction; /**< Whether or not polarity of duty cycle and encoder counter is inverted */
    int32_t max_stall_duty;     /**< Upper limit on duty cycle, which corresponds to a maximum torque while stalled. */
} pbio_dcmotor_settings_t;

pbio_dcmotor_settings_t dcmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_iodev_type_id_t expected_id, pbio_motor_dir_t direction);

pbio_error_t pbio_dcmotor_set_settings(pbio_port_t port, int16_t stall_torque_limit_pct);

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_dcmotor_coast(pbio_port_t port);

pbio_error_t pbio_dcmotor_brake(pbio_port_t port);

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int32_t duty_cycle_int);

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle);

#endif // _PBIO_DCMOTOR_H_
