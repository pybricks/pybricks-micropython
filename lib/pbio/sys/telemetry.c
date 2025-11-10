// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_TELEMETRY

#include <stdio.h>

#include <pbio/os.h>
#include <pbio/port_interface.h>
#include <pbio/util.h>

#include <pbsys/host.h>

typedef struct {
    lego_device_type_id_t type_id;
    int32_t value;
} pbsys_telemetry_port_data_t;

static pbsys_telemetry_port_data_t last_data[PBIO_CONFIG_PORT_NUM_DEV];

// Revisit: Come up with a data encoding protocol. Right now it just sends
// six motor positions to drive the existing motor animation.
static uint8_t update_port_data(uint8_t index, uint8_t *buf) {

    // Get type and angle.
    int32_t degrees = 0;
    pbio_angle_t angle;
    lego_device_type_id_t type_id = LEGO_DEVICE_TYPE_ID_NONE;
    pbio_port_t *port = pbio_port_by_index(index);
    pbio_error_t err = pbio_port_get_angle(port, &angle);
    if (err == PBIO_SUCCESS) {
        type_id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
        degrees = pbio_angle_to_low_res(&angle, 1000);
    }

    pbsys_telemetry_port_data_t *data = &last_data[index];

    if (data->type_id == type_id && data->value == degrees) {
        return 0;
    }

    data->type_id = type_id;
    data->value = degrees;

    buf[0] = type_id;
    buf[1] = index;
    pbio_set_uint32_le(&buf[2], degrees);
    return 6;
}

/**
 * Hub, motor, and sensor telemetry to host.
 */
static pbio_error_t pbsys_telemetry_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;
    static pbio_os_state_t sub;
    static uint8_t buf[20];
    static uint8_t size;
    static uint32_t i = 0;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        PBIO_OS_AWAIT_MS(state, &timer, 40);

        // Revisit, could concatenate packages for more efficient BLE sending.
        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {
            size = update_port_data(i, buf);
            if (size) {
                PBIO_OS_AWAIT(state, &sub, pbsys_host_send_event(&sub, PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY, buf, size));
            }
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 *
 * Starts telemetry process.
 */
void pbsys_telemetry_init(void) {
    static pbio_os_process_t pbsys_telemetry_process;
    pbio_os_process_start(&pbsys_telemetry_process, pbsys_telemetry_process_thread, NULL);
}

#endif // PBSYS_CONFIG_TELEMETRY
