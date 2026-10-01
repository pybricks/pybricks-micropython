// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// White balancing of the EV3 color sensor.
//
// The sensor applies its factory calibration only to the modes that report a
// precomputed color index, not to the raw RGB mode that we use. Those gains
// are not readable, so equivalent ones are measured on the hub instead.

#include <pbio/config.h>

#if PBIO_CONFIG_PORT_LUMP && PBIO_CONFIG_PORT_LUMP_EV3

#include <string.h>

#include <pbio/color.h>
#include <pbio/int_math.h>
#include <pbio/os.h>
#include <pbio/port_lump.h>

#include <pbsys/storage.h>

#include <lego/device.h>

#include "lump_dev.h"

// COL-REFLECT reports (uint8_t)(|red off - red on| * redFactor / 4.09), where
// redFactor is the factory white balance constant that the sensor keeps to
// itself. Comparing that against the same measurement in REF-RAW recovers the
// constant, which puts the raw RGB channels on an absolute scale.
#define EV3_COLOR_CAL_REFLECT_DIV       (4.09f)

// The factory stores redFactor = 409 / raw red of its reference white, so 409
// is the raw red level that LEGO's white produced at their chosen distance.
// It is the full scale of the calibrated channels.
#define EV3_COLOR_CAL_WHITE_LEVEL       (409.0f)

#define EV3_COLOR_CAL_SAMPLES           (40)
#define EV3_COLOR_CAL_WHITE_SAMPLES     (10)
#define EV3_COLOR_CAL_USER_TIMEOUT      (30000)
#define EV3_COLOR_CAL_MEASURE_TIMEOUT   (10000)
#define EV3_COLOR_CAL_RESULT_TIME       (3000)

// Value of COL-COLOR when the sensor sees white.
#define EV3_COLOR_CAL_WHITE_INDEX       (6)

// Calibration for the sensor on each port, owned by persistent storage, or
// NULL if the settings have not been loaded (yet).
static pbio_port_lump_ev3_color_calibration_t *ev3_color_calibration;

/**
 * Sets the calibration of all ports to neutral.
 *
 * @param [out] calibration  Array of ::PBIO_CONFIG_PORT_LUMP_NUM_DEV entries.
 */
void pbio_port_lump_ev3_color_set_default_calibration(pbio_port_lump_ev3_color_calibration_t *calibration) {
    for (uint8_t i = 0; i < PBIO_CONFIG_PORT_LUMP_NUM_DEV; i++) {
        for (uint8_t channel = 0; channel < 3; channel++) {
            calibration[i].gain[channel] = 1.0f;
        }
    }
}

/**
 * Takes ownership of the stored calibration, so it can be read when a sensor
 * syncs up and updated when the user calibrates one.
 *
 * @param [in]  calibration  Array of ::PBIO_CONFIG_PORT_LUMP_NUM_DEV entries.
 */
void pbio_port_lump_ev3_color_apply_loaded_calibration(pbio_port_lump_ev3_color_calibration_t *calibration) {
    ev3_color_calibration = calibration;
}

/** Copies the stored calibration of this port into the device that synced up. */
void pbio_port_lump_ev3_color_load_calibration(pbio_port_lump_dev_t *lump_dev) {
    for (uint8_t channel = 0; channel < 3; channel++) {
        lump_dev->device.ev3_color.gain[channel] = ev3_color_calibration ?
            ev3_color_calibration[lump_dev->index].gain[channel] : 1.0f;
    }
}

/** Copies a newly measured calibration back into storage for this port. */
static void pbio_port_lump_ev3_color_save_calibration(pbio_port_lump_dev_t *lump_dev) {
    if (!ev3_color_calibration) {
        return;
    }
    memcpy(ev3_color_calibration[lump_dev->index].gain, lump_dev->device.ev3_color.gain,
        sizeof(lump_dev->device.ev3_color.gain));
    pbsys_storage_request_write();
}

/**
 * Checks whether a calibration is currently running on this device.
 *
 * While it is, the sensor changes modes on its own, so its measurements should
 * not be used.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  True if calibrating, false otherwise.
 */
bool pbio_port_lump_ev3_color_calibration_busy(pbio_port_lump_dev_t *lump_dev) {
    return lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR && lump_dev->device.ev3_color.thread != 0;
}

// Requests a mode and waits until fresh data for it comes in. Only for use
// inside the calibration protothread.
#define EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, desired)                                   \
    do {                                                                                     \
        PBIO_OS_AWAIT_UNTIL(state, pbio_port_lump_set_mode(lump_dev, desired) == PBIO_SUCCESS); \
        PBIO_OS_AWAIT_UNTIL(state, pbio_port_lump_is_ready(lump_dev) == PBIO_SUCCESS &&      \
    lump_dev->mode == (desired));                                                    \
    } while (0)

// Waits for the next sample in the given mode, giving up on the whole
// calibration if it does not arrive before the timer expires. Only for use
// inside the calibration protothread.
#define EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, desired, fail_label)              \
    do {                                                                                     \
        do {                                                                                 \
            PBIO_OS_AWAIT_ONCE(state);                                                       \
            if (pbio_os_timer_is_expired(timer)) {                                           \
                debug_pr("EV3 color calibration: timed out.\n");                             \
                goto fail_label;                                                             \
            }                                                                                \
        } while (lump_dev->mode != (desired));                                               \
    } while (0)

/**
 * Measures the white balance gains of the EV3 color sensor.
 *
 * The gains are reconstructed by measuring a white surface that the user holds
 * in front of the sensor. The result is only valid for the raw RGB mode; the
 * sensor itself is left untouched.
 *
 * This is iterated once for every sample received from the sensor.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  ::PBIO_ERROR_AGAIN while calibrating, else
 *                          ::PBIO_SUCCESS when done, successfully or not.
 */
static pbio_error_t pbio_port_lump_ev3_color_calibrate(pbio_port_lump_dev_t *lump_dev) {

    pbio_port_lump_ev3_color_state_t *color = &lump_dev->device.ev3_color;
    pbio_os_state_t *state = &color->thread;
    pbio_os_timer_t *timer = &color->timer;
    const int16_t *data16 = (const int16_t *)lump_dev->bin_data;

    PBIO_OS_ASYNC_BEGIN(state);

    color->calibration_state = EV3_COLOR_CALIBRATION_STATE_WAIT_FOR_USER;

    // The precomputed color index does respect the factory calibration, so it
    // is a trustworthy way to tell that the sensor now sees white at a
    // workable distance.
    EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__COLOR);
    pbio_os_timer_set(timer, EV3_COLOR_CAL_USER_TIMEOUT);
    color->count = 0;
    while (color->count < EV3_COLOR_CAL_WHITE_SAMPLES) {
        EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__COLOR, fail);
        // Must be white several samples in a row, not just on average.
        color->count = lump_dev->bin_data[0] == EV3_COLOR_CAL_WHITE_INDEX ? color->count + 1 : 0;
    }

    color->calibration_state = EV3_COLOR_CALIBRATION_STATE_RGB_BALANCING;
    pbio_os_timer_set(timer, EV3_COLOR_CAL_MEASURE_TIMEOUT);

    EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REFLECT);
    color->sum[0] = 0;
    color->count = 0;
    color->reflect_min = INT16_MAX;
    color->reflect_max = INT16_MIN;
    while (color->count < EV3_COLOR_CAL_SAMPLES) {
        EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REFLECT, fail);
        int16_t reflect = lump_dev->bin_data[0];
        color->sum[0] += reflect;
        color->reflect_min = pbio_int_math_min(color->reflect_min, reflect);
        color->reflect_max = pbio_int_math_max(color->reflect_max, reflect);
        color->count++;
    }
    color->reflect = color->sum[0] / EV3_COLOR_CAL_SAMPLES;

    // Same acquisition as COL-REFLECT but without the factory scaling applied.
    EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REF_RAW);
    color->sum[0] = 0;
    color->count = 0;
    while (color->count < EV3_COLOR_CAL_SAMPLES) {
        EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REF_RAW, fail);
        color->sum[0] += pbio_int_math_abs(data16[1] - data16[0]);
        color->count++;
    }
    color->delta = color->sum[0] / EV3_COLOR_CAL_SAMPLES;

    EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW);
    color->sum[0] = 0;
    color->sum[1] = 0;
    color->sum[2] = 0;
    color->count = 0;
    while (color->count < EV3_COLOR_CAL_SAMPLES) {
        EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW, fail);
        for (uint8_t i = 0; i < 3; i++) {
            color->sum[i] += data16[i];
        }
        color->count++;
    }

    // Confirm that the sensor did not move away from the white surface while
    // all of the above was measured.
    EV3_COLOR_CAL_AWAIT_MODE(state, lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__COLOR);
    color->count = 0;
    while (color->count < EV3_COLOR_CAL_WHITE_SAMPLES) {
        EV3_COLOR_CAL_AWAIT_SAMPLE(state, lump_dev, timer, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__COLOR, fail);
        if (lump_dev->bin_data[0] != EV3_COLOR_CAL_WHITE_INDEX) {
            debug_pr("EV3 color calibration: no longer looking at white.\n");
            goto fail;
        }
        color->count++;
    }

    {
        float red = color->sum[0] / EV3_COLOR_CAL_SAMPLES;
        float green = color->sum[1] / EV3_COLOR_CAL_SAMPLES;
        float blue = color->sum[2] / EV3_COLOR_CAL_SAMPLES;
        float weakest = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
        float strongest = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);

        if (color->reflect_max >= 100) {
            debug_pr("EV3 color calibration: COL-REFLECT clamped, too close.\n");
            goto fail;
        }
        if (color->reflect_min < 20 || weakest < 60) {
            debug_pr("EV3 color calibration: too little light, too far away.\n");
            goto fail;
        }
        if (color->reflect_max - color->reflect_min > 1) {
            debug_pr("EV3 color calibration: sensor was not held still.\n");
            goto fail;
        }
        if (strongest > 550 || color->delta < 40) {
            debug_pr("EV3 color calibration: raw levels out of usable range.\n");
            goto fail;
        }

        // COL-REFLECT is truncated to a whole number, so the true value lies
        // halfway the measured mean and the next step up.
        float red_factor = EV3_COLOR_CAL_REFLECT_DIV * (color->reflect + 0.5f) / color->delta;
        color->gain[0] = red_factor;
        color->gain[1] = red_factor * red / green;
        color->gain[2] = red_factor * red / blue;
    }

    debug_pr("EV3 color gains (x1000): R=%d G=%d B=%d\n",
        (int)(color->gain[0] * 1000),
        (int)(color->gain[1] * 1000),
        (int)(color->gain[2] * 1000));

    pbio_port_lump_ev3_color_save_calibration(lump_dev);

    color->calibration_state = EV3_COLOR_CALIBRATION_STATE_DONE_SUCCESS;
    goto done;

fail:
    color->calibration_state = EV3_COLOR_CALIBRATION_STATE_DONE_FAILURE;

done:
    pbio_port_lump_request_mode(lump_dev, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW);

    // Keep reporting the result for a while so the host can show it.
    PBIO_OS_AWAIT_MS(state, timer, EV3_COLOR_CAL_RESULT_TIME);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Starts a new calibration, if one is not already running.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 */
void pbio_port_lump_ev3_color_calibration_start(pbio_port_lump_dev_t *lump_dev) {
    if (pbio_port_lump_ev3_color_calibration_busy(lump_dev)) {
        return;
    }
    PBIO_OS_ASYNC_RESET(&lump_dev->device.ev3_color.thread);
    pbio_port_lump_ev3_color_calibrate(lump_dev);
}

/**
 * Advances a calibration in progress, if any, on receiving new data.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 */
void pbio_port_lump_ev3_color_handle_data(pbio_port_lump_dev_t *lump_dev) {
    if (!pbio_port_lump_ev3_color_calibration_busy(lump_dev)) {
        return;
    }
    if (pbio_port_lump_ev3_color_calibrate(lump_dev) != PBIO_ERROR_AGAIN) {
        // Calibration completed, go back to reporting data.
        PBIO_OS_ASYNC_RESET(&lump_dev->device.ev3_color.thread);
    }
}

/**
 * Scales one white balanced EV3 color sensor channel to 0--255.
 *
 * @param [in]  raw         The raw channel value.
 * @param [in]  gain        The white balance gain for this channel.
 * @return                  The scaled channel value.
 */
static uint8_t ev3_color_sensor_scale(int16_t raw, float gain) {
    float value = raw * gain * 255 / EV3_COLOR_CAL_WHITE_LEVEL;
    if (value <= 0) {
        return 0;
    }
    return value >= 255 ? 255 : (uint8_t)value;
}

/**
 * Gets the white balanced RGB values of the EV3 color sensor, scaled to 0--255.
 *
 * Until the user calibrates, the gains are 1 and only the overall scale is
 * right, which makes the colors usable but not accurate.
 *
 * @param [in]  lump_dev    The LEGO UART device instance, in RGB_RAW mode.
 * @param [out] rgb         The scaled color channels.
 */
void pbio_port_lump_ev3_color_get_rgb(pbio_port_lump_dev_t *lump_dev, pbio_color_rgb_t *rgb) {
    const int16_t *data = (const int16_t *)lump_dev->bin_data;
    const float *gain = lump_dev->device.ev3_color.gain;
    rgb->r = ev3_color_sensor_scale(data[0], gain[0]);
    rgb->g = ev3_color_sensor_scale(data[1], gain[1]);
    rgb->b = ev3_color_sensor_scale(data[2], gain[2]);
}

#endif // PBIO_CONFIG_PORT_LUMP && PBIO_CONFIG_PORT_LUMP_EV3
