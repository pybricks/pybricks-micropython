// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Device connection manager for NXT sensor ports.

#include <pbio/config.h>

#include <pbio/port_dcm.h>

#if PBIO_CONFIG_PORT_DCM_NXT

struct _pbio_port_dcm_t {
    uint8_t unused;
};

static pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {
    // Never completes, so the port process does not spin.
    return PBIO_ERROR_AGAIN;
}

pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    if (index >= PBIO_CONFIG_PORT_DCM_NUM_DEV) {
        return NULL;
    }
    return &dcm_state[index];
}

pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *expected_type_id) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, bool active) {
    return 0;
}

bool pbio_port_dcm_get_pressed(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {
    return false;
}

pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode) {
    // Callers raise on error, but the compiler can't tell they never read these.
    if (reflected) {
        *reflected = 0;
    }
    if (ambient) {
        *ambient = 0;
    }
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, pbsys_telemetry_packet_t *tel, uint32_t *size) {
    return PBSYS_TELEMETRY_ERROR_NO_REPORT;
}

#endif // PBIO_CONFIG_PORT_DCM_NXT
