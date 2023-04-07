// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2023 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/config.h>
#include <pbio/control_settings.h>
#include <pbio/differentiator.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

/**
 * Internal function to get the speed with a variable window size. Window
 * size must be validated externally for this function to be used safely.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  window_size    Window size in number of samples (Must be > 0 and <= buffer size!).
 * @param [out] speed          Average speed across given time window in mdeg/s.
 */
static int32_t pbio_differentiator_calc_speed(pbio_differentiator_t *dif, uint8_t window_size) {

    // Sum differences including start and endpoint.
    uint8_t start_index = (dif->index - (window_size - 1) + PBIO_ARRAY_SIZE(dif->history)) % PBIO_ARRAY_SIZE(dif->history);
    int32_t total = dif->history[dif->index];
    for (uint8_t i = start_index; i != dif->index; i = (i + 1) % PBIO_ARRAY_SIZE(dif->history)) {
        total += dif->history[i];
    }

    // Each sample has units of mdeg, so take average and convert to mdeg/s.
    return total * (1000 / PBIO_CONFIG_CONTROL_LOOP_TIME_MS) / window_size;
}

/**
 * Updates the angle buffer and calculates the average speed across buffer.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  angle          New angle sample to add to the buffer.
 * @return                     Average speed across position buffer.
 */
int32_t pbio_differentiator_update_and_get_speed(pbio_differentiator_t *dif, const pbio_angle_t *angle) {

    // Increment index where latest difference will be stored.
    dif->index = (dif->index + 1) % PBIO_ARRAY_SIZE(dif->history);

    // The difference is stored in millidegrees. Even at 6000 deg/s (well
    // above the physical limits of the motors we use), this at most
    // 6000 * 1000 * 0.005 = 30000, which fits in a 16-bit signed integer.
    dif->history[dif->index] = pbio_int_math_clamp(pbio_angle_diff_mdeg(angle, &dif->prev_angle), INT16_MAX);
    dif->prev_angle = *angle;

    // Calculate the speed.
    return pbio_differentiator_calc_speed(dif, PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE);
}

/**
 * Gets the speed with a variable window size. This can be called by the user
 * to get a smoothed speed value.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  window         Window size in milliseconds.
 * @param [out] speed          Average speed across given time window.
 * @return                     ::PBIO_SUCCESS if successful, ::PBIO_ERROR_INVALID_ARG if window is 0 or bigger than the buffer size.
 */
pbio_error_t pbio_differentiator_get_speed(pbio_differentiator_t *dif, uint32_t window, int32_t *speed) {

    // Round window to nearest sample size.
    uint8_t window_size = (window + PBIO_CONFIG_CONTROL_LOOP_TIME_MS / 2) / PBIO_CONFIG_CONTROL_LOOP_TIME_MS;
    if (window_size == 0 || window_size > PBIO_ARRAY_SIZE(dif->history) - 1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Speed is determined as position delta across the given window.
    *speed = pbio_differentiator_calc_speed(dif, window_size);
    return PBIO_SUCCESS;
}

/**
 * Resets the position angle buffer in order to set speed to zero.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  angle          New angle sample to add to the buffer.
 */
void pbio_differentiator_reset(pbio_differentiator_t *dif, const pbio_angle_t *angle) {
    dif->prev_angle = *angle;
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(dif->history); i++) {
        dif->history[i] = 0;
    }
}
