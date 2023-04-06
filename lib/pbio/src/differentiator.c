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
#include <pbio/util.h>

/**
 * Updates the angle buffer and calculates the average speed across buffer.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  angle          New angle sample to add to the buffer.
 * @return                     Average speed across position buffer.
 */
int32_t pbio_differentiator_update_and_get_speed(pbio_differentiator_t *dif, const pbio_angle_t *angle) {

    // Difference between current angle and oldest in buffer.
    uint8_t read_index = (dif->index - PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE + PBIO_ARRAY_SIZE(dif->history)) % PBIO_ARRAY_SIZE(dif->history);
    int32_t delta = pbio_angle_diff_mdeg(angle, &dif->history[read_index]);

    // Override oldest sample with new value.
    dif->history[dif->index] = *angle;
    dif->index = (dif->index + 1) % PBIO_ARRAY_SIZE(dif->history);

    // Return average speed.
    return delta * (1000 / (PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE * PBIO_CONFIG_CONTROL_LOOP_TIME_MS));
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

    // This is where the most recent angle was stored.
    uint8_t newest_index = (dif->index - 1 + PBIO_ARRAY_SIZE(dif->history)) % PBIO_ARRAY_SIZE(dif->history);
    uint8_t compare_index = (newest_index - window_size + PBIO_ARRAY_SIZE(dif->history)) % PBIO_ARRAY_SIZE(dif->history);

    // Speed is determined as position delta across the given window.
    int32_t delta = pbio_angle_diff_mdeg(&dif->history[newest_index], &dif->history[compare_index]);
    *speed = delta * 1000 / (window_size * PBIO_CONFIG_CONTROL_LOOP_TIME_MS);
    return PBIO_SUCCESS;
}

/**
 * Resets the position angle buffer in order to set speed to zero.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  angle          New angle sample to add to the buffer.
 */
void pbio_differentiator_reset(pbio_differentiator_t *dif, const pbio_angle_t *angle) {
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(dif->history); i++) {
        dif->history[i] = *angle;
    }
}
