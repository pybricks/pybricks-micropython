// SPDX-License-Identifier: MIT
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/control.h>
#include <pbio/observer.h>
#include <pbio/iodev.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_control_settings_t settings_servo_default = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 5,
    .count_tolerance = 3,
    .stall_rate_limit = 2,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 200,
    .pid_ki = 100,
    .pid_kd = 0,
    .integral_range = 45,
    .integral_rate = 3,
    .max_control = 10000,
    .control_offset = 0,
    .actuation_scale = 100,
    .use_estimated_speed = true,
};

static pbio_observer_settings_t settings_observer_default = {
    .phi_01 = 0.00475829617417038f,
    .phi_11 = 0.904902040212507f,
    .gam_0 = 0.692925315153182f,
    .gam_1 = 272.630288453209f,
    .k_0 = 0.052359221241860474f,
    .k_1 = 0.000333337511635670f,
    .k_2 = 0.00666198910636721f,
    .f_low = 0.011619602790697674f,
    .obs_gain = 0.004f,
};

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

static pbio_control_settings_t settings_servo_ev3_medium = {
    .max_rate = 2000,
    .abs_acceleration = 8000,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 300,
    .pid_ki = 400,
    .pid_kd = 3,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 10000,
    .control_offset = 2000,
    .actuation_scale = 100,
    .use_estimated_speed = false,
};

static pbio_control_settings_t settings_servo_ev3_large = {
    .max_rate = 1600,
    .abs_acceleration = 3200,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 1200,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 10000,
    .control_offset = 0,
    .actuation_scale = 100,
    .use_estimated_speed = false,
};

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

static pbio_control_settings_t settings_servo_move_hub = {
    .max_rate = 1500,
    .abs_acceleration = 5000,
    .rate_tolerance = 50,
    .count_tolerance = 6,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 600,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 10000,
    .control_offset = 2000,
    .actuation_scale = 100,
    .use_estimated_speed = false,
};

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

#if PBDRV_CONFIG_IOPORT_LPF2

static pbio_control_settings_t settings_servo_boost_interactive = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 5,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 600,
    .pid_ki = 600,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 3,
    .max_control = 10000,
    .control_offset = 1000,
    .actuation_scale = 100,
    .use_estimated_speed = true,
};

static pbio_control_settings_t settings_servo_technic_xl = {
    .max_rate = 1000,
    .abs_acceleration = 4000,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 250,
    .pid_ki = 350,
    .pid_kd = 0,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 10000,
    .control_offset = 1500,
    .actuation_scale = 100,
    .use_estimated_speed = true,
};

static pbio_observer_settings_t settings_observer_technic_m_angular = {
    .phi_01 = 0.00479747589526887f,
    .phi_11 = 0.920099171168609f,
    .gam_0 = 1.39344831914273f,
    .gam_1 = 549.750242229303f,
    .k_0 = 0.02258435646747968f,
    .k_1 = 0.000386402254699512f,
    .k_2 = 0.00643543836108826f,
    .f_low = 0.01218641268292683f,
    .obs_gain = 0.002f,
};

static pbio_observer_settings_t settings_observer_technic_l_angular = {
    .phi_01 = 0.00475829617417038f,
    .phi_11 = 0.904902040212507f,
    .gam_0 = 0.692925315153182f,
    .gam_1 = 272.630288453209f,
    .k_0 = 0.052359221241860474f,
    .k_1 = 0.000333337511635670f,
    .k_2 = 0.00666198910636721f,
    .f_low = 0.011619602790697674f,
    .obs_gain = 0.004f,
};

#endif // PBDRV_CONFIG_IOPORT_LPF2

void pbio_servo_load_settings(pbio_control_settings_t *control_settings, pbio_observer_settings_t *observer_settings, pbio_iodev_type_id_t id) {
    switch (id) {
        #if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *control_settings = settings_servo_ev3_medium;
            *observer_settings = settings_observer_default;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
            *control_settings = settings_servo_ev3_large;
            *observer_settings = settings_observer_default;
            break;
        #endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO
        #if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *control_settings = settings_servo_move_hub;
            *observer_settings = settings_observer_default;
            break;
        #endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        #if PBDRV_CONFIG_IOPORT_LPF2
        case PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR:
            *control_settings = settings_servo_boost_interactive;
            *observer_settings = settings_observer_default;
            *observer_settings = settings_observer_default;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_MOTOR:
            *control_settings = settings_servo_technic_xl;
            control_settings->control_offset = 2500;
            *observer_settings = settings_observer_default;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_XL_MOTOR:
            *control_settings = settings_servo_technic_xl;
            *observer_settings = settings_observer_default;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR:
            *control_settings = settings_servo_technic_xl;
            control_settings->control_offset = 2500;
            *observer_settings = settings_observer_technic_m_angular;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR:
            *control_settings = settings_servo_technic_xl;
            *observer_settings = settings_observer_technic_l_angular;
            break;
        #endif // PBDRV_CONFIG_IOPORT_LPF2
        default:
            *control_settings = settings_servo_default;
            *observer_settings = settings_observer_default;
            break;
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0
