// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef _PBIO_PORT_DCM_H_
#define _PBIO_PORT_DCM_H_

#include <lego/device.h>

#include <pbdrv/i2c.h>

#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>

#include <pbsys/telemetry.h>

typedef struct _pbio_port_dcm_t pbio_port_dcm_t;

#if PBIO_CONFIG_PORT_DCM

pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins);

/**
 * Waits for an I2C device to be disconnected, as detected by the pins. Used
 * for I2C devices that are not polled by the port.
 *
 * Disconnection detection does not need to work for every type of I2C device.
 * Devices that are polled are detected by no longer responding.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  timer       The timer to use for timing.
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 * @param [in]  i2c_dev     The I2C driver of this port, or NULL.
 * @return                  ::PBIO_SUCCESS when the device is disconnected.
 */
pbio_error_t pbio_port_dcm_i2c_exited(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbdrv_i2c_dev_t *i2c_dev);

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
 * Gets the analog value of the device connected to the port.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 * @param [in]  active      Whether to get activate active mode.
 */
uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, bool active);

/**
 * Gets the pressed state of a touch sensor connected to the port.
 *
 * Uses hysteresis around the threshold, so the state is updated each time
 * this is called.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 * @return                  True if pressed, false if released.
 */
bool pbio_port_dcm_get_pressed(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins);

/**
 * Gets the color measured by an analog color sensor on the port, in a device
 * independent HSV format that is comparable across sensors.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [out] color_hsv   The measured color.
 * @param [out] id          The type of the sensor that took the measurement,
 *                          or NULL to skip.
 * @param [out] mode        The measurement mode of that sensor, or NULL to
 *                          skip. See ::pbio_port_lump_get_color.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if no color sensor is attached
 *                          or it does not measure color in the way that is
 *                          currently active.
 *                          ::PBIO_ERROR_AGAIN if no sample is available yet.
 */
pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode);

/**
 * Gets the light intensity measured by an analog light sensor on the port.
 *
 * @param [in]  dcm         The device connection manager.
 * @param [out] reflected   The measured reflection, 0--1000, or
 *                          ::PBIO_LIGHT_INTENSITY_NOT_AVAILABLE if the sensor
 *                          does not measure it. May be NULL to skip.
 * @param [out] ambient     The measured ambient light, same scale and sentinel
 *                          as @p reflected. May be NULL to skip.
 * @param [out] id          The type of the sensor that took the measurement,
 *                          or NULL to skip.
 * @param [out] mode        The measurement mode of that sensor, or NULL to
 *                          skip. See ::pbio_port_lump_get_color.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if no light sensor is attached
 *                          or it measures neither value in the way that is
 *                          currently active.
 *                          ::PBIO_ERROR_AGAIN if no sample is available yet.
 */
pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode);

pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbsys_telemetry_packet_t *tel, uint32_t *size);

#else // PBIO_CONFIG_PORT_DCM

static inline pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    return NULL;
}

static inline pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id) {
    // Fixme: need a DCM implementation for CI tests. For now just pass LUMP.
    if (*type_id == LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, bool active) {
    return 0;
}

static inline bool pbio_port_dcm_get_pressed(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins) {
    return false;
}

static inline pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_dcm_i2c_exited(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbdrv_i2c_dev_t *i2c_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbsys_telemetry_packet_t *tel, uint32_t *size) {
    return PBSYS_TELEMETRY_ERROR_NO_REPORT;
}

#endif // PBIO_CONFIG_PORT_DCM

#endif // _PBIO_PORT_DCM_H_
