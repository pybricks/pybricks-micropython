// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#ifndef _PBIO_PORT_I2C_KNOWN_H_
#define _PBIO_PORT_I2C_KNOWN_H_

#include <stdint.h>

#include <lego/device.h>

#include <pbdrv/i2c.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/port.h>

#include <pbsys/telemetry.h>

typedef struct _pbio_port_i2c_known_dev_t pbio_port_i2c_known_dev_t;

#if PBIO_CONFIG_PORT_I2C_KNOWN

pbio_port_i2c_known_dev_t *pbio_port_i2c_known_init_instance(uint8_t index);

void pbio_port_i2c_known_reset(pbio_port_i2c_known_dev_t *dev);

pbio_error_t pbio_port_i2c_known_scan_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer);

pbio_error_t pbio_port_i2c_known_poll_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer);

bool pbio_port_i2c_known_is_found(pbio_port_i2c_known_dev_t *dev);

pbio_error_t pbio_port_i2c_known_assert_unknown(pbio_port_i2c_known_dev_t *dev);

pbio_error_t pbio_port_i2c_known_get_data(pbio_port_i2c_known_dev_t *dev, lego_device_type_id_t type_id, const uint8_t **data);

pbio_port_power_requirements_t pbio_port_i2c_known_get_power_requirements(pbio_port_i2c_known_dev_t *dev);

pbsys_telemetry_error_t pbio_port_i2c_known_get_telemetry(pbio_port_i2c_known_dev_t *dev, pbsys_telemetry_packet_t *tel, uint32_t *size);

#else // PBIO_CONFIG_PORT_I2C_KNOWN

static inline pbio_port_i2c_known_dev_t *pbio_port_i2c_known_init_instance(uint8_t index) {
    return NULL;
}

static inline void pbio_port_i2c_known_reset(pbio_port_i2c_known_dev_t *dev) {
}

static inline pbio_error_t pbio_port_i2c_known_scan_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_i2c_known_poll_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbio_port_i2c_known_is_found(pbio_port_i2c_known_dev_t *dev) {
    return false;
}

static inline pbio_error_t pbio_port_i2c_known_assert_unknown(pbio_port_i2c_known_dev_t *dev) {
    return PBIO_SUCCESS;
}

static inline pbio_error_t pbio_port_i2c_known_get_data(pbio_port_i2c_known_dev_t *dev, lego_device_type_id_t type_id, const uint8_t **data) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_port_power_requirements_t pbio_port_i2c_known_get_power_requirements(pbio_port_i2c_known_dev_t *dev) {
    return PBIO_PORT_POWER_REQUIREMENTS_NONE;
}

static inline pbsys_telemetry_error_t pbio_port_i2c_known_get_telemetry(pbio_port_i2c_known_dev_t *dev, pbsys_telemetry_packet_t *tel, uint32_t *size) {
    return PBSYS_TELEMETRY_ERROR_NO_REPORT;
}

#endif // PBIO_CONFIG_PORT_I2C_KNOWN

#endif // _PBIO_PORT_I2C_KNOWN_H_
