// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

// Support for NXT analog sensors, shared by the EV3 and NXT device connection
// managers.

#ifndef _PBIO_SRC_PORT_DCM_NXT_SENSORS_H_
#define _PBIO_SRC_PORT_DCM_NXT_SENSORS_H_

#include <stdbool.h>
#include <stdint.h>

#include <lego/device.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/os.h>

#include <pbdrv/ioport.h>

/**
 * Calibration data sent by the NXT Color Sensor.
 */
typedef struct __attribute__((__packed__)) {
    uint32_t calibration[3][4];
    uint16_t threshold_high;
    uint16_t threshold_low;
    uint16_t crc;
} pbio_port_dcm_nxt_color_sensor_data_t;

/**
 * Cached analog values when a particular light color is active.
 */
typedef struct {
    /** Red analog value. */
    uint32_t r;
    /** Green analog value. */
    uint32_t g;
    /** Blue analog value. */
    uint32_t b;
    /** Ambient analog value. */
    uint32_t a;
    /** Time of most recent full update, or 0 if there is no sample yet. */
    uint32_t last_sample_time;
} pbio_port_dcm_analog_rgba_t;

/**
 * State of an NXT Light Sensor or NXT Color Sensor.
 */
typedef struct {
    /** Protothread state of the routine that is currently running. */
    pbio_os_state_t child;
    /** Byte counter while receiving color sensor calibration data. */
    uint32_t count;
    /** Bit counter while sending or receiving a color sensor byte. */
    uint8_t bit;
    /** Color sensor calibration data. */
    pbio_port_dcm_nxt_color_sensor_data_t data;
    /** Most recent raw samples. */
    pbio_port_dcm_analog_rgba_t rgba;
} pbio_port_dcm_nxt_optical_t;

/**
 * Debounced state of an analog touch sensor.
 */
typedef struct {
    /** Last reported pressed state. */
    bool pressed;
    /** Whether readings are past the release threshold while still pressed. */
    bool releasing;
    /** Time (ms) at which releasing started. */
    uint32_t release_start;
} pbio_port_dcm_touch_t;

uint32_t pbio_port_dcm_adc_to_mv(uint32_t adc_10bit);

uint32_t pbio_port_dcm_get_mv(const pbdrv_ioport_pins_t *pins, uint8_t pin);

pbio_error_t pbio_port_dcm_nxt_color_init(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer);

pbio_error_t pbio_port_dcm_nxt_color_sample(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer);

pbio_error_t pbio_port_dcm_nxt_light_sample(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer);

pbio_error_t pbio_port_dcm_nxt_get_color(pbio_port_dcm_nxt_optical_t *optical, lego_device_type_id_t type_id, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode);

pbio_error_t pbio_port_dcm_nxt_get_light_intensity(pbio_port_dcm_nxt_optical_t *optical, lego_device_type_id_t type_id, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode);

bool pbio_port_dcm_update_touch(pbio_port_dcm_touch_t *touch, bool past_press, bool past_release);

bool pbio_port_dcm_nxt_touch_get_pressed(pbio_port_dcm_touch_t *touch, const pbdrv_ioport_pins_t *pins);

#endif // _PBIO_SRC_PORT_DCM_NXT_SENSORS_H_
