// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Encoding of LEGO UART device data for the telemetry protocol.
//
// Telemetry modes are abstract capabilities rather than LEGO modes, since not
// all LEGO modes are functional or meaningfully different.

#include <pbio/config.h>

#if PBIO_CONFIG_PORT_LUMP

#include <string.h>

#include <pbio/int_math.h>
#include <pbio/port_lump.h>
#include <pbio/util.h>

#include <pbsys/telemetry.h>

#include <lego/device.h>

#include "lump_dev.h"

/**
 * Gets the telemetry report for the attached device, if any.
 *
 * Color and light sensors are reported by the port in a device independent
 * way, so only their other measurements are handled here.
 *
 * @param [in]     lump_dev The LEGO UART device instance.
 * @param [out]    tel      The packet to populate.
 * @param [in,out] size     Room available for the payload, set to the size of
 *                          the payload on success.
 * @return                  ::PBSYS_TELEMETRY_SUCCESS on success.
 */
pbsys_telemetry_error_t pbio_port_lump_get_telemetry(pbio_port_lump_dev_t *lump_dev, pbsys_telemetry_packet_t *tel, uint32_t *size) {

    if (pbio_port_lump_is_ready(lump_dev) != PBIO_SUCCESS) {
        // Leaves size untouched: it is the room still available to the next
        // generator in this round, not an output until we report success.
        return PBSYS_TELEMETRY_ERROR_NO_REPORT;
    }

    tel->id = lump_dev->type_id;

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->mode = 2;
        tel->payload[0] = lump_dev->bin_data[0] * 10;
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW) {
        int32_t force;
        int32_t distance;
        if (pbio_port_lump_get_force(lump_dev, &force, &distance) == PBIO_SUCCESS) {
            if (*size < 2 * sizeof(uint16_t)) {
                return PBSYS_TELEMETRY_ERROR_NO_ROOM;
            }
            tel->mode = 0;
            pbio_set_uint16_le(&tel->payload[0], force);
            pbio_set_uint16_le(&tel->payload[2], distance);
            *size = 2 * sizeof(uint16_t);
            return PBSYS_TELEMETRY_SUCCESS;
        }
    }

    if ((lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_ULTRASONIC_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__DISTL) ||
        (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_ULTRASONIC_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM)) {
        if (*size < sizeof(uint16_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        int16_t distance = *(int16_t *)lump_dev->bin_data;
        int16_t limit = lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_ULTRASONIC_SENSOR ? 2000 : 2550;
        pbio_set_uint16_le(tel->payload, distance < 0 || distance >= limit ? limit : distance);
        *size = sizeof(uint16_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_WEDO2_MOTION_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_PUP_WEDO2_MOTION_SENSOR__CAL) {
        if (*size < 2 * sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        uint16_t raw = *(int16_t *)lump_dev->bin_data;
        tel->payload[0] = pbio_int_math_bind(1100 / (10 + raw), 0, 100);
        tel->payload[1] = pbio_int_math_bind(raw / 5, 0, 100);
        *size = 2 * sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_WEDO2_TILT_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE) {
        if (*size < 2 * sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->mode = 0;
        tel->payload[0] = lump_dev->bin_data[0];
        tel->payload[1] = lump_dev->bin_data[1];
        *size = 2 * sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX && lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O) {
        if (*size < 9 * sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->mode = 0;
        // This device reports the rows in the order opposite of setting.
        memcpy(&tel->payload[0], &lump_dev->bin_data[6], 3);
        memcpy(&tel->payload[3], &lump_dev->bin_data[3], 3);
        memcpy(&tel->payload[6], &lump_dev->bin_data[0], 3);
        *size = 9 * sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    #if PBIO_CONFIG_PORT_LUMP_EV3
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_IR_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__PROX) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        // TODO: Other modes.
        tel->mode = 0;
        tel->payload[0] = lump_dev->bin_data[0];
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_GYRO_SENSOR && lump_dev->mode == LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A) {
        if (*size < 2 * sizeof(uint16_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        int16_t angle = *(int16_t *)&lump_dev->bin_data[0];
        int16_t speed = *(int16_t *)&lump_dev->bin_data[2];
        pbio_set_uint16_le(&tel->payload[0], angle);
        pbio_set_uint16_le(&tel->payload[2], speed);
        tel->mode = 0;
        *size = 2 * sizeof(uint16_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    // When calibrating, return only the calibration progress state.
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR && pbio_port_lump_ev3_color_calibration_busy(lump_dev)) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->payload[0] = lump_dev->device.ev3_color.calibration_state;
        tel->mode = 2;
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    #endif // PBIO_CONFIG_PORT_LUMP_EV3

    // No specific encoding, so return device ID without payload for this mode.
    tel->mode = 0xFF;
    *size = 0;
    return PBSYS_TELEMETRY_SUCCESS;
}

/**
 * Sets the mode of the attached device based on a telemetry request.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [in]  tel         The packet with the requested mode.
 * @param [in]  size        The size of the packet payload.
 * @return                  ::PBSYS_TELEMETRY_SUCCESS on success.
 */
pbsys_telemetry_error_t pbio_port_lump_set_telemetry_mode(pbio_port_lump_dev_t *lump_dev, pbsys_telemetry_packet_t *tel, uint32_t size) {

    if (pbio_port_lump_is_ready(lump_dev) != PBIO_SUCCESS) {
        return PBSYS_TELEMETRY_ERROR_NO_REPORT;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR) {
        uint8_t mode;
        switch (tel->mode) {
            case 0:
                mode = LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I;
                break;
            case 1:
                mode = LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI;
                break;
            case 2:
                mode = LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX;
                break;
            default:
                return PBSYS_TELEMETRY_ERROR_NO_REPORT;
        }
        pbio_port_lump_set_mode(lump_dev, mode);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR) {
        uint8_t mode;
        switch (tel->mode) {
            case 0:
                mode = LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I;
                break;
            case 1:
                mode = LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV;
                break;
            default:
                return PBSYS_TELEMETRY_ERROR_NO_REPORT;
        }
        pbio_port_lump_set_mode(lump_dev, mode);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    #if PBIO_CONFIG_PORT_LUMP_EV3
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR) {
        uint8_t mode;
        switch (tel->mode) {
            case 0:
                mode = LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW;
                break;
            case 1:
                mode = LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__AMBIENT;
                break;
            case 2:
                // Calibration mode. If not active, set it in motion.
                pbio_port_lump_ev3_color_calibration_start(lump_dev);
                return PBSYS_TELEMETRY_SUCCESS;
            default:
                return PBSYS_TELEMETRY_ERROR_NO_REPORT;
        }
        pbio_port_lump_set_mode(lump_dev, mode);
        return PBSYS_TELEMETRY_SUCCESS;
    }
    #endif // PBIO_CONFIG_PORT_LUMP_EV3

    return PBSYS_TELEMETRY_SUCCESS;
}

#endif // PBIO_CONFIG_PORT_LUMP
