// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"

#include "pbio_os_config.h"

#include <pbio/port_interface.h>
#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

#include <umm_malloc.h>

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
        .motor_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
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

// Socket used to send data to Python animation.
static int data_socket = -1;
static struct sockaddr_in serv_addr;

void virtual_hub_socket_send(const uint8_t *data, uint32_t size) {
    if (data_socket < 0) {
        return;
    }
    ssize_t sent = sendto(data_socket, data, size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (sent < 0) {
        printf("send() failed");
        close(data_socket);
        data_socket = -1;
    }
}

static void virtual_hub_socket_init(void) {
    if (!getenv("PBIO_TEST_CONNECT_SOCKET")) {
        return;
    }
    data_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (data_socket < 0) {
        printf("socket() failed");
        return;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5002);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("inet_pton() failed");
        close(data_socket);
        data_socket = -1;
        return;
    }
}

// The 'embedded' main.
extern void _main(void);

int main(int argc, char **argv) {

    // Optional output via animated hub.
    virtual_hub_socket_init();

    // Separate heap for large allocations - defined in linker script.
    static uint8_t umm_heap[1024 * 1024 * 2];
    umm_init_heap(umm_heap, sizeof(umm_heap));

    // Parse given program, else otherwise default to REPL.
    if (argc > 1) {

        // Pybricksdev helper script, pipes multi-mpy to us.
        char command[512];
        snprintf(command, sizeof(command), "pybricksdev compile --bin %s", argv[argc - 1]);
        FILE *pipe = popen(command, "r");
        if (!pipe) {
            printf("Failed to compile program with Pybricksdev\n");
            return 0;
        }

        // Read the multi-mpy file from pipe.
        extern uint8_t pbsys_hmi_native_program_buf[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
        extern uint32_t pbsys_hmi_native_program_size;
        pbsys_hmi_native_program_size = fread(pbsys_hmi_native_program_buf, 1, sizeof(pbsys_hmi_native_program_buf), pipe);
        pclose(pipe);

        if (pbsys_hmi_native_program_size == 0) {
            printf("Error reading from pipe\n");
            return 0;
        }
    }

    #ifdef PBDRV_CONFIG_RUN_ON_CI
    // On the CI modifying settings for stdin causes problems. The REPL isn't
    // used on CI anyway.
    _main();
    return 0;
    #endif

    // Save the original terminal settings
    struct termios term_old, term_new;
    if (tcgetattr(STDIN_FILENO, &term_old) != 0) {
        printf("DEBUG: Failed to get terminal attributes\n");
        return 0;
    }
    term_new = term_old;

    // Get one char at a time instead of newline and disable CTRL+C for exit.
    term_new.c_lflag &= ~(ICANON | ECHO | ISIG);

    // MicroPython REPL expects \r for newline.
    term_new.c_iflag |= INLCR;
    term_new.c_iflag &= ~ICRNL;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term_new) != 0) {
        printf("Failed to set terminal attributes\n");
        return 0;
    }

    // Set stdin non-blocking so we can service it in the runloop like on
    // embedded hubs.
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (stdin_flags == -1) {
        printf("Failed to get fcntl flags\n");
        return 0;
    }
    if (fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK) == -1) {
        printf("Failed to set non-blocking\n");
        return 0;
    }

    // Simulate running embedded main.
    _main();

    // Restore stdin flags.
    if (fcntl(STDIN_FILENO, F_SETFL, stdin_flags) == -1) {
        printf("Failed to restore stdin flags\n");
    }

    // Restore terminal settings.
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term_old) != 0) {
        printf("Failed to restore terminal attributes\n");
        return 0;
    }

    return 0;
}
