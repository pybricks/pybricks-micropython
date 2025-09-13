// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2010-2013 The LEGO Group
// Copyright (c) 2017-2018,2021 David Lechner <david@lechnology.com>
// Copyright (C) 2025 The Pybricks Authors

#include <pbsys/config.h>

// LEGO MINDSTORMS EV3 battery temperature estimation from lms2012
#if PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION

/**
 * Function for estimating new battery temperature based on measurements
 * of battery voltage and battery current.
 * @param [in] V_bat Battery voltage (volts)
 * @param [in] I_bat Battery current (amps)
 * @returns Estimated battery temperature (degrees Celsius)
 */
float pbsys_battery_temp_update(float V_bat, float I_bat) {
    static struct {
        /** Keeps track of sample index since power-on. */
        uint32_t index;
        /** Running mean current. */
        float I_bat_mean;
        /** Battery temperature. */
        float T_bat;
        /** EV3 electronics temperature. */
        float T_elec;
        /** Old internal resistance of the battery model. */
        float R_bat_model_old;
        /** Internal resistance of the batteries. */
        float R_bat;
        // Flag that prevents initialization of R_bat when the battery is charging
        bool has_passed_7v5_flag;
    } bat_temp;

    /*************************** Model parameters *******************************/
    // Approx. initial internal resistance of 6 Energizer industrial batteries:
    const float R_bat_init = 0.63468f;
    // Batteries' heat capacity:
    const float heat_cap_bat = 136.6598f;
    // Newtonian cooling constant for electronics:
    const float K_bat_loss_to_elec = -0.0003f; // -0.000789767;
    // Newtonian heating constant for electronics:
    const float K_bat_gain_from_elec = 0.001242896f; // 0.001035746;
    // Newtonian cooling constant for environment:
    const float K_bat_to_room = -0.00012f;
    // Battery power Boost
    const float battery_power_boost = 1.7f;
    // Battery R_bat negative gain
    const float R_bat_neg_gain = 1.00f;

    // Slope of electronics lossless heating curve (linear!!!) [Deg.C / s]:
    const float K_elec_heat_slope = 0.0123175f;
    // Newtonian cooling constant for battery packs:
    const float K_elec_loss_to_bat = -0.004137487f;
    // Newtonian heating constant for battery packs:
    const float K_elec_gain_from_bat = 0.002027574f; // 0.00152068;
    // Newtonian cooling constant for environment:
    const float K_elec_to_room = -0.001931431f; // -0.001843639;

    // NB: This time must match PBSYS_BATTERY_TEMP_TIMER_PERIOD_MS
    const float sample_period = 0.4f;   // Algorithm update period in seconds

    float R_bat_model;          // Internal resistance of the battery model
    float slope_A;              // Slope obtained by linear interpolation
    float intercept_b;          // Offset obtained by linear interpolation
    const float I_1A = 0.05f;   // Current carrying capacity at bottom of the curve
    const float I_2A = 2.0f;    // Current carrying capacity at the top of the curve

    float R_1A; // Internal resistance of the batteries at 1A and V_bat
    float R_2A; // Internal resistance of the batteries at 2A and V_bat

    float dT_bat_own;               // Batteries' own heat
    float dT_bat_loss_to_elec;      // Batteries' heat loss to electronics
    float dT_bat_gain_from_elec;    // Batteries' heat gain from electronics
    float dT_bat_loss_to_room;      // Batteries' cooling from environment

    float dT_elec_own;              // Electronics' own heat
    float dT_elec_loss_to_bat;      // Electronics' heat loss to the battery pack
    float dT_elec_gain_from_bat;    // Electronics' heat gain from battery packs
    float dT_elec_loss_to_room;     // Electronics' heat loss to the environment

    /***************************************************************************/

    // Update the average current: I_bat_mean
    if (bat_temp.index > 0) {
        bat_temp.I_bat_mean = (bat_temp.index * bat_temp.I_bat_mean + I_bat) / (bat_temp.index + 1);
    } else {
        bat_temp.I_bat_mean = I_bat;
    }

    bat_temp.index++;

    // Calculate R_1A as a function of V_bat (internal resistance at 1A continuous)
    R_1A = 0.014071f * (V_bat * V_bat * V_bat * V_bat)
        - 0.335324f * (V_bat * V_bat * V_bat)
        + 2.933404f * (V_bat * V_bat)
        - 11.243047f * V_bat
        + 16.897461f;

    // Calculate R_2A as a function of V_bat (internal resistance at 2A continuous)
    R_2A = 0.014420f * (V_bat * V_bat * V_bat * V_bat)
        - 0.316728f * (V_bat * V_bat * V_bat)
        + 2.559347f * (V_bat * V_bat)
        - 9.084076f * V_bat
        + 12.794176f;

    // Calculate the slope by linear interpolation between R_1A and R_2A
    slope_A = (R_1A - R_2A) / (I_1A - I_2A);

    // Calculate intercept by linear interpolation between R1_A and R2_A
    intercept_b = R_1A - slope_A * R_1A;

    // Reload R_bat_model:
    R_bat_model = slope_A * bat_temp.I_bat_mean + intercept_b;

    // Calculate batteries' internal resistance: R_bat
    if (V_bat > 7.5 && !bat_temp.has_passed_7v5_flag) {
        bat_temp.R_bat = R_bat_init; // 7.5 V not passed a first time
    } else {
        // Only update R_bat with positive outcomes: R_bat_model - R_bat_model_old
        // R_bat updated with the change in model R_bat is not equal value in the model!
        if ((R_bat_model - bat_temp.R_bat_model_old) > 0) {
            bat_temp.R_bat += R_bat_model - bat_temp.R_bat_model_old;
        } else { // The negative outcome of R_bat_model added to only part of R_bat
            bat_temp.R_bat += R_bat_neg_gain * (R_bat_model - bat_temp.R_bat_model_old);
        }
        // Make sure we initialize R_bat later
        bat_temp.has_passed_7v5_flag = true;
    }

    // Save R_bat_model for use in the next function call
    bat_temp.R_bat_model_old = R_bat_model;

    //  pbdrv_uart_debug_printf("%c %f %f %f %f %f %f\r\n", bat_temp.has_passed_7v5_flag ? 'Y' : 'N',
    //      (double)R_1A, (double)R_2A, (double)slope_A, (double)intercept_b,
    //      (double)(R_bat_model - bat_temp.R_bat_model_old), (double)bat_temp.R_bat);

    /**** Calculate the 4 types of temperature change for the batteries ****/

    // Calculate the batteries' own temperature change
    dT_bat_own = bat_temp.R_bat * I_bat * I_bat * sample_period * battery_power_boost / heat_cap_bat;

    // Calculate the batteries' heat loss to the electronics
    if (bat_temp.T_bat > bat_temp.T_elec) {
        dT_bat_loss_to_elec = K_bat_loss_to_elec * (bat_temp.T_bat - bat_temp.T_elec) * sample_period;
    } else {
        dT_bat_loss_to_elec = 0.0f;
    }

    // Calculate the batteries' heat gain from the electronics
    if (bat_temp.T_bat < bat_temp.T_elec) {
        dT_bat_gain_from_elec = K_bat_gain_from_elec * (bat_temp.T_elec - bat_temp.T_bat) * sample_period;
    } else {
        dT_bat_gain_from_elec = 0.0f;
    }

    // Calculate the batteries' heat loss to environment
    dT_bat_loss_to_room = K_bat_to_room * bat_temp.T_bat * sample_period;

    /**** Calculate the 4 types of temperature change for the electronics ****/

    // Calculate the electronics' own temperature change
    dT_elec_own = K_elec_heat_slope * sample_period;

    // Calculate the electronics' heat loss to the batteries
    if (bat_temp.T_elec > bat_temp.T_bat) {
        dT_elec_loss_to_bat = K_elec_loss_to_bat * (bat_temp.T_elec - bat_temp.T_bat) * sample_period;
    } else {
        dT_elec_loss_to_bat = 0.0f;
    }

    // Calculate the electronics' heat gain from the batteries
    if (bat_temp.T_elec < bat_temp.T_bat) {
        dT_elec_gain_from_bat = K_elec_gain_from_bat * (bat_temp.T_bat - bat_temp.T_elec) * sample_period;
    } else {
        dT_elec_gain_from_bat = 0.0f;
    }

    // Calculate the electronics' heat loss to the environment
    dT_elec_loss_to_room = K_elec_to_room * bat_temp.T_elec * sample_period;

    /*****************************************************************************/

    //  pbdrv_uart_debug_printf("%f %f %f %f %f <> %f %f %f %f %f\r\n",
    //      (double)dT_bat_own, (double)dT_bat_loss_to_elec,
    //      (double)dT_bat_gain_from_elec, (double)dT_bat_loss_to_room, (double)bat_temp.T_bat,
    //      (double)dT_elec_own, (double)dT_elec_loss_to_bat, (double)dT_elec_gain_from_bat,
    //      (double)dT_elec_loss_to_room, (double)bat_temp.T_elec);

    // Refresh battery temperature
    bat_temp.T_bat += dT_bat_own + dT_bat_loss_to_elec + dT_bat_gain_from_elec + dT_bat_loss_to_room;

    // Refresh electronics temperature
    bat_temp.T_elec += dT_elec_own + dT_elec_loss_to_bat + dT_elec_gain_from_bat + dT_elec_loss_to_room;

    return bat_temp.T_bat;
}

#endif // PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION
