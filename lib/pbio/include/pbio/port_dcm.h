// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef _PBIO_PORT_DCM_H_
#define _PBIO_PORT_DCM_H_

#include <lego/device.h>

#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbdrv/ioport.h>

#include <pbsys/telemetry.h>

typedef struct _pbio_port_dcm_t pbio_port_dcm_t;

#if PBIO_CONFIG_PORT_DCM

pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins);

/**
 * Gets device connection manager state.
 *
 * @param [in]  index       The index of the DC motor.
 * @return                  The dcmotor instance.
 */
pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index);

/**
 * Asserts or gets the device id of a device connected to the port.
 *
 * @param [in]  lump_dev    The connection manager instance.
 * @param [out] type_id     The device id.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached or is not of the expected type range.
 */
pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id);

/**
 * Sets the device id of a device connected to the port on platforms without
 * device detection.
 *
 * @param [in]  lump_dev    The connection manager instance.
 * @param [out] type_id     The device id to set.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NOT_SUPPORTED if this type does not support setting.
 */
pbio_error_t pbio_port_dcm_set_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t type_id);

/**
 * Gets the analog value of the device connected to the port.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 * @param [in]  active      Whether to get activate active mode.
 */
uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, bool active);

/**
 * Gets the color measured by an analog color sensor on the port, in a device
 * independent HSV format that is comparable across sensors.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [out] color_hsv   The measured color.
 * @param [in]  reflected   Whether to measure the surface lit by the sensor
 *                          light (true) or the ambient light (false).
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if no color sensor is attached.
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the sensor cannot
 *                          measure color in this way.
 *                          ::PBIO_ERROR_AGAIN if no sample is available yet.
 */
pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, bool reflected);

/**
 * Gets the light intensity measured by an analog light sensor on the port.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [out] intensity   The measured intensity, 0--1000.
 * @param [in]  reflected   Whether to measure the surface lit by the sensor
 *                          light (true) or the ambient light (false).
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if no light sensor is attached.
 *                          ::PBIO_ERROR_AGAIN if no sample is available yet.
 */
pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, int32_t *intensity, bool reflected);

pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, pbsys_telemetry_packet_t *tel, uint32_t *size);

#else // PBIO_CONFIG_PORT_DCM

static inline pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    return NULL;
}

static inline pbio_error_t pbio_port_dcm_set_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t type_id) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id) {
    // Fixme: need a DCM implementation for CI tests. For now just pass LUMP.
    if (*type_id == LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, bool active) {
    return 0;
}

static inline pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, bool reflected) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, int32_t *intensity, bool reflected) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, pbsys_telemetry_packet_t *tel, uint32_t *size) {
    return PBSYS_TELEMETRY_ERROR_NO_REPORT;
}

#endif // PBIO_CONFIG_PORT_DCM

#endif // _PBIO_PORT_DCM_H_
