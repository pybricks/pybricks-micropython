#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbdrv/motor.h>

typedef float float_t;

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (200)

// This should go elsewhere in the lib. The items below should get the actual IDs
/**
 * Device ID
 */
typedef enum {
    PBIO_ID_UNKNOWN,
    PBIO_ID_UNKNOWN_DCMOTOR,
    PBIO_ID_UNKNOWN_ENCMOTOR,
    PBIO_ID_PUP_MOVEHUB_MOTOR = 39,   // 0x27,
    PBIO_ID_EV3_MEDIUM_MOTOR,
    PBIO_ID_EV3_LARGE_MOTOR,
} pbio_id_t;

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

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction);

pbio_error_t pbio_dcmotor_set_settings(pbio_port_t port, float_t stall_torque_limit);

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_dcmotor_coast(pbio_port_t port);

pbio_error_t pbio_dcmotor_brake(pbio_port_t port);

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int16_t duty_cycle_int);

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle);

#endif // _PBIO_DCMOTOR_H_
