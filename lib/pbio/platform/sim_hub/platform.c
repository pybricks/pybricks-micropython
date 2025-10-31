// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <stdio.h>

#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"

#include "pbio_os_config.h"

#include <pbio/port_interface.h>
#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = NULL,
    .pin = 0,
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .counter_driver_index = 0,
        .external_port_index = 0,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .counter_driver_index = 1,
        .external_port_index = 1,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .external_port_index = 2,
        .counter_driver_index = 2,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .external_port_index = 3,
        .counter_driver_index = 3,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .motor_driver_index = 4,
        .external_port_index = 4,
        .counter_driver_index = 4,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .motor_driver_index = 5,
        .external_port_index = 5,
        .counter_driver_index = 5,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
};

#define INFINITY (1e100)

const pbdrv_motor_driver_virtual_simulation_platform_data_t
    pbdrv_motor_driver_virtual_simulation_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 123456,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -142000,
        .endstop_angle_positive = 142000,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .type_id = LEGO_DEVICE_TYPE_ID_NONE,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_S_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 45000,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
};

extern uint8_t pbsys_hmi_native_program_buf[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
extern uint32_t pbsys_hmi_native_program_size;

int main(int argc, char **argv) {

    if (argc > 1) {

        // Pybricksdev helper script, pipes multi-mpy to us.
        char command[512];
        snprintf(command, sizeof(command), "python ./bricks/simhub/make_mpy.py %s", argv[1]);
        FILE *pipe = popen(command, "r");
        if (!pipe) {
            printf("Failed to compile program with Pybricksdev\n");
            return 0;
        }

        // Read the multi-mpy file from pipe.
        pbsys_hmi_native_program_size = fread(pbsys_hmi_native_program_buf, 1, sizeof(pbsys_hmi_native_program_buf), pipe);
        pclose(pipe);

        if (pbsys_hmi_native_program_size == 0) {
            printf("Error reading from pipe\n");
            return 0;
        }
    }

    extern void _main(void);
    _main();
}
