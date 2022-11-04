// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/config.h>
#include <pbio/control_settings.h>
#include <pbio/differentiator.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

/**
 * Updates the angle buffer and calculates the average speed across buffer.
 *
 * @param [in]  dif            The differentiator instance.
 * @param [in]  angle          New angle sample to add to the buffer.
 * @return                     Average speed across position buffer.
 */
int32_t pbio_differentiator_get_speed(pbio_differentiator_t *dif, const pbio_angle_t *angle) {

    // Difference between current angle and oldest in buffer.
    int32_t delta = pbio_angle_diff_mdeg(angle, &dif->history[dif->index]);

    // Override oldest sample with new value.
    dif->history[dif->index] = *angle;
    dif->index = (dif->index + 1) % PBIO_ARRAY_SIZE(dif->history);

    // Return average speed.
    return delta * (1000 / PBIO_CONFIG_DIFFERENTIATOR_WINDOW_MS);
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
