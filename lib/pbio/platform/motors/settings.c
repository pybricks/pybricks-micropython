// SPDX-License-Identifier: MIT
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/control.h>
#include <pbio/observer.h>
#include <pbio/iodev.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#if PBIO_CONFIG_CONTROL_MINIMAL
#define SCALE(value, scalar) ((int32_t)((value) * (scalar)))
#else
#define SCALE(value, scalar) ((value) * (scalar))
#endif

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT

static const pbio_observer_model_t model_ev3_m = {
    .phi_01 = SCALE(0.0f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.02222706190764267f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000204397590361446f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00262048192771084f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.009158620689655174f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = 0,
};

static const pbio_control_settings_t settings_ev3_m = {
    .max_rate = 2000,
    .abs_acceleration = 8000,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 3000,
    .pid_ki = 150,
    .pid_kd = 30,
    .integral_rate = 10,
    .use_estimated_rate = false,
};

static const pbio_observer_model_t model_ev3_l = {
    .phi_01 = SCALE(0.0f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(0.0f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.049886243386243395f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000433486238532110f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00412844036697248f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.00823809523809524f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = 0,
};

static const pbio_control_settings_t settings_ev3_l = {
    .max_rate = 1600,
    .abs_acceleration = 3200,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 15000,
    .pid_ki = 600,
    .pid_kd = 250,
    .integral_rate = 10,
    .use_estimated_rate = false,
};

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT

#if PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST

static const pbio_observer_model_t model_technic_s_angular = {
    .phi_01 = SCALE(0.00468582248829651f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.877017999120650f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(4.49747292641861f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(1760.49589416085f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.010402524000000002f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000255865711052433f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00671533699371639f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.0005f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(100, 500, 500),
};

static const pbio_control_settings_t settings_technic_s_angular = {
    .max_rate = 620,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 5000,
    .pid_ki = 1200,
    .pid_kd = 800,
    .integral_rate = 25,
    .use_estimated_rate = true,
};

static const pbio_observer_model_t model_technic_m_angular = {
    .phi_01 = SCALE(0.00465623814072756f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.865721137934373f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(2.36522158991640f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(923.893256509643f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.02258435646747968f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000223154509118161f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00643543836108826f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.01218641268292683f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(100, 800, 2000),
};

static const pbio_control_settings_t settings_technic_m_angular = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 10000,
    .pid_ki = 2000,
    .pid_kd = 1200,
    .integral_rate = 25,
    .use_estimated_rate = true,
};

static const pbio_observer_model_t model_technic_l_angular = {
    .phi_01 = SCALE(0.00476139134919619f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.906099484723787f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(0.684051954862549f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(269.197410994572f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.052359221241860474f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000337807915176920f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00666198910636721f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.011619602790697674f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(1000, 2000, 4000),
};

static const pbio_control_settings_t settings_technic_l_angular = {
    .max_rate = 1000,
    .abs_acceleration = 1500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 25000,
    .pid_ki = 6000,
    .pid_kd = 4500,
    .integral_rate = 5,
    .use_estimated_rate = true,
};

static const pbio_observer_model_t model_interactive = {
    .phi_01 = SCALE(0.00476271917080314f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.906613349592095f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(2.93111612537300f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(1153.59979915647f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.01500933205496964f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000275066965901687f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00539346991964092f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.005613422818791947f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(2000, 2000, 2000),
};

static const pbio_control_settings_t settings_interactive = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 5,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 10000,
    .pid_ki = 1000,
    .pid_kd = 1000,
    .integral_rate = 3,
    .use_estimated_rate = true,
};

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

static const pbio_observer_model_t model_movehub = {
    .phi_01 = SCALE(0.00482560542071840f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.931062779704023f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(2.20557850267909f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(871.853080213830f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.02120903269295585f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000260968229954614f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00372811757078020f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.012417391304347828f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(2000, 2000, 2000),
};

static const pbio_control_settings_t settings_movehub = {
    .max_rate = 1500,
    .abs_acceleration = 5000,
    .rate_tolerance = 50,
    .count_tolerance = 6,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 15000,
    .pid_ki = 1500,
    .pid_kd = 500,
    .integral_rate = 5,
    .use_estimated_rate = true,
};

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

static const pbio_observer_model_t model_technic_l = {
    .phi_01 = SCALE(0.00480673379919289f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.923702638108050f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(1.53998720733930f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(607.954007356971f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.029291367521367528f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000269922879177378f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00428449014567267f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.013215000000000001f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(1000, 2000, 2000),
};

static const pbio_control_settings_t settings_technic_l = {
    .max_rate = 1000,
    .abs_acceleration = 1500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 4000,
    .pid_ki = 600,
    .pid_kd = 1000,
    .integral_rate = 5,
    .use_estimated_rate = true,
};

static const pbio_observer_model_t model_technic_xl = {
    .phi_01 = SCALE(0.00481529951016882f, PBIO_OBSERVER_SCALE_HIGH),
    .phi_11 = SCALE(0.927040916512593f, PBIO_OBSERVER_SCALE_LOW),
    .gam_0 = SCALE(1.66041757033247f, PBIO_OBSERVER_SCALE_LOW),
    .gam_1 = SCALE(655.886425902678f, PBIO_OBSERVER_SCALE_LOW),
    .k_0 = SCALE(0.025904742547425474f, PBIO_OBSERVER_SCALE_HIGH),
    .k_1 = SCALE(0.000283410138248848f, PBIO_OBSERVER_SCALE_HIGH),
    .k_2 = SCALE(0.00429409300377042f, PBIO_OBSERVER_SCALE_HIGH),
    .f_low = SCALE(0.002f, PBIO_OBSERVER_SCALE_TRQ),
    .obs_gains = PBIO_OBSERVER_GAINS(500, 1500, 2000),
};

static const pbio_control_settings_t settings_technic_xl = {
    .max_rate = 1000,
    .abs_acceleration = 2500,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 4000,
    .pid_ki = 600,
    .pid_kd = 2000,
    .integral_rate = 5,
    .use_estimated_rate = true,
};

#endif // PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST

pbio_error_t pbio_servo_load_settings(pbio_control_settings_t *settings, const pbio_observer_model_t **model, pbio_iodev_type_id_t id) {
    switch (id) {
        case PBIO_IODEV_TYPE_ID_NONE:
            return PBIO_ERROR_NOT_SUPPORTED;
        #if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *model = &model_ev3_m;
            *settings = settings_ev3_m;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
            *model = &model_ev3_l;
            *settings = settings_ev3_l;
            break;
        #endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO || PBDRV_CONFIG_COUNTER_NXT
        #if PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST
        case PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR:
            *model = &model_interactive;
            *settings = settings_interactive;
            break;
        #if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *model = &model_movehub;
            *settings = settings_movehub;
            break;
        #endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_MOTOR:
            *model = &model_technic_l;
            *settings = settings_technic_l;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_XL_MOTOR:
            *model = &model_technic_xl;
            *settings = settings_technic_xl;
            break;
        case PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR:
            *model = &model_technic_s_angular;
            *settings = settings_technic_s_angular;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR:
            *model = &model_technic_l_angular;
            *settings = settings_technic_l_angular;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR:
            *model = &model_technic_m_angular;
            *settings = settings_technic_m_angular;
            break;
        #endif // PBDRV_CONFIG_IOPORT_LPF2 || PBDRV_CONFIG_COUNTER_TEST
        default:
            return PBIO_ERROR_NOT_SUPPORTED;
    }
    // Given the configured settings, now calculate dependent settings.

    // Initialize maximum torque as the stall torque for maximum voltage.
    settings->max_torque = pbio_observer_voltage_to_torque(*model, pbio_dcmotor_get_max_voltage(id));

    return PBIO_SUCCESS;
}

int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id) {
    if (id == PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR) {
        return 6000;
    }
    return 9000;
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0
