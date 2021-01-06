// SPDX-License-Identifier: MIT
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/control.h>
#include <pbio/observer.h>
#include <pbio/iodev.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT

static const pbio_observer_settings_t settings_observer_ev3_m = {
    .phi_01 = 0.0f,
    .phi_11 = 0.0f,
    .gam_0 = 0.0f,
    .gam_1 = 0.0f,
    .k_0 = 0.02222706190764267f,
    .k_1 = 0.000204397590361446f,
    .k_2 = 0.00262048192771084f,
    .f_low = 0.009158620689655174f,
    .obs_gain = 0.0f,
};

static const pbio_control_settings_t settings_servo_ev3_m = {
    .max_rate = 2000,
    .abs_acceleration = 8000,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 3000,
    .pid_ki = 150,
    .pid_kd = 30,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 150000,
    .use_estimated_rate = false,
    .use_estimated_count = false,
};

static const pbio_observer_settings_t settings_observer_ev3_l = {
    .phi_01 = 0.0f,
    .phi_11 = 0.0f,
    .gam_0 = 0.0f,
    .gam_1 = 0.0f,
    .k_0 = 0.049886243386243395f,
    .k_1 = 0.000433486238532110f,
    .k_2 = 0.00412844036697248f,
    .f_low = 0.00823809523809524f,
    .obs_gain = 0.0f,
};

static const pbio_control_settings_t settings_servo_ev3_l = {
    .max_rate = 1600,
    .abs_acceleration = 3200,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 15000,
    .pid_ki = 600,
    .pid_kd = 250,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 430000,
    .use_estimated_rate = false,
    .use_estimated_count = false,
};

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT

#if PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST

static const pbio_observer_settings_t settings_observer_technic_m_angular = {
    .phi_01 = 0.00471127825986593f,
    .phi_11 = 0.886778220155195f,
    .gam_0 = 1.98652315497896f,
    .gam_1 = 779.011955265974f,
    .k_0 = 0.02258435646747968f,
    .k_1 = 0.000267785410941794f,
    .k_2 = 0.00643543836108826f,
    .f_low = 0.01218641268292683f,
    .obs_gain = 0.002f,
};

static const pbio_control_settings_t settings_servo_technic_m_angular = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 20000,
    .pid_ki = 2000,
    .pid_kd = 1200,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 160000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

static const pbio_observer_settings_t settings_observer_technic_l_angular = {
    .phi_01 = 0.00476139134919619f,
    .phi_11 = 0.906099484723787f,
    .gam_0 = 0.684051954862549f,
    .gam_1 = 269.197410994572f,
    .k_0 = 0.052359221241860474f,
    .k_1 = 0.000337807915176920f,
    .k_2 = 0.00666198910636721f,
    .f_low = 0.011619602790697674f,
    .obs_gain = 0.004f,
};

static const pbio_control_settings_t settings_servo_technic_l_angular = {
    .max_rate = 1000,
    .abs_acceleration = 1500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 25000,
    .pid_ki = 6000,
    .pid_kd = 4500,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 330000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

static const pbio_observer_settings_t settings_observer_interactive = {
    .phi_01 = 0.00476271917080314f,
    .phi_11 = 0.906613349592095f,
    .gam_0 = 2.93111612537300f,
    .gam_1 = 1153.59979915647f,
    .k_0 = 0.01500933205496964f,
    .k_1 = 0.000275066965901687f,
    .k_2 = 0.00539346991964092f,
    .f_low = 0.005613422818791947f,
    .obs_gain = 0.002f,
};

static const pbio_control_settings_t settings_servo_interactive = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 5,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 10000,
    .pid_ki = 1000,
    .pid_kd = 1000,
    .integral_range = 45,
    .integral_rate = 3,
    .max_control = 100000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

static const pbio_observer_settings_t settings_observer_movehub = {
    .phi_01 = 0.00482560542071840f,
    .phi_11 = 0.931062779704023f,
    .gam_0 = 2.20557850267909f,
    .gam_1 = 871.853080213830f,
    .k_0 = 0.02120903269295585f,
    .k_1 = 0.000260968229954614f,
    .k_2 = 0.00372811757078020f,
    .f_low = 0.012417391304347828f,
    .obs_gain = 0.002f,
};

static const pbio_control_settings_t settings_servo_movehub = {
    .max_rate = 1500,
    .abs_acceleration = 5000,
    .rate_tolerance = 50,
    .count_tolerance = 6,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 15000,
    .pid_ki = 1500,
    .pid_kd = 500,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 150000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

static const pbio_observer_settings_t settings_observer_technic_l = {
    .phi_01 = 0.00480673379919289f,
    .phi_11 = 0.923702638108050f,
    .gam_0 = 1.53998720733930f,
    .gam_1 = 607.954007356971f,
    .k_0 = 0.029291367521367528f,
    .k_1 = 0.000269922879177378f,
    .k_2 = 0.00428449014567267f,
    .f_low = 0.013215000000000001f,
    .obs_gain = 0.002f,
};

static const pbio_control_settings_t settings_servo_technic_l = {
    .max_rate = 1000,
    .abs_acceleration = 1500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 4000,
    .pid_ki = 600,
    .pid_kd = 1000,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 260000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

static const pbio_observer_settings_t settings_observer_technic_xl = {
    .phi_01 = 0.00481529951016882f,
    .phi_11 = 0.927040916512593f,
    .gam_0 = 1.66041757033247f,
    .gam_1 = 655.886425902678f,
    .k_0 = 0.025904742547425474f,
    .k_1 = 0.000283410138248848f,
    .k_2 = 0.00429409300377042f,
    .f_low = 0.006446341463414635f,
    .obs_gain = 0.002f,
};

static const pbio_control_settings_t settings_servo_technic_xl = {
    .max_rate = 1000,
    .abs_acceleration = 1500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 2500,
    .pid_ki = 400,
    .pid_kd = 2000,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 260000,
    .use_estimated_rate = true,
    .use_estimated_count = false,
};

#endif // PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST

void pbio_servo_load_settings(pbio_control_settings_t *control_settings, const pbio_observer_settings_t **observer_settings, pbio_iodev_type_id_t id) {
    switch (id) {
        #if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *observer_settings = &settings_observer_ev3_m;
            *control_settings = settings_servo_ev3_m;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
        default:
            *observer_settings = &settings_observer_ev3_l;
            *control_settings = settings_servo_ev3_l;
            break;
        #endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT
        #if PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST
        case PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR:
            *observer_settings = &settings_observer_interactive;
            *control_settings = settings_servo_interactive;
            break;
        #if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *observer_settings = &settings_observer_movehub;
            *control_settings = settings_servo_movehub;
            break;
        #endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_MOTOR:
            *observer_settings = &settings_observer_technic_l;
            *control_settings = settings_servo_technic_l;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_XL_MOTOR:
            *observer_settings = &settings_observer_technic_xl;
            *control_settings = settings_servo_technic_xl;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR:
            *observer_settings = &settings_observer_technic_l_angular;
            *control_settings = settings_servo_technic_l_angular;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR:
        default:
            *observer_settings = &settings_observer_technic_m_angular;
            *control_settings = settings_servo_technic_m_angular;
            break;
        #endif // PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0
