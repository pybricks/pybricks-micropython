// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC_VIRTUAL

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rproc_virtual.h"

#include <pbio/os.h>
#include <pbio/util.h>

#define SEND_PORT (5002)
#define RECV_PORT (5003)

// Sockets used to exchange data with Python animation.
static int send_socket = -1;
static int recv_socket = -1;
static struct sockaddr_in send_addr;
static struct sockaddr_in recv_addr;

void pbdrv_rproc_virtual_socket_send(const uint8_t *data, uint32_t size) {
    if (send_socket < 0) {
        return;
    }
    ssize_t sent = sendto(send_socket, data, size, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
    if (sent < 0) {
        printf("send() failed\n");
        close(send_socket);
        send_socket = -1;
    }
}

static uint32_t button_state;

uint32_t pdrv_rproc_virtual_get_button_state(void) {
    return button_state;
}

static pbio_error_t pdrv_rproc_virtual_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        PBIO_OS_AWAIT_MS(state, &timer, 2);

        uint8_t buf[256];
        ssize_t len;
        while ((len = recvfrom(recv_socket, buf, sizeof(buf), 0, NULL, NULL)) > 0) {
            // REVISIT: Generalize telemetry receipt protocol. Assume button state.
            if (len == 4) {
                button_state = pbio_get_uint32_le(buf);
            }
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbdrv_rproc_init(void) {
    if (!getenv("PBIO_TEST_CONNECT_SOCKET")) {
        return;
    }

    // Setup sending socket.
    send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_socket < 0) {
        printf("socket() failed\n");
        return;
    }
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(SEND_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &send_addr.sin_addr) <= 0) {
        printf("inet_pton() failed\n");
        close(send_socket);
        send_socket = -1;
        return;
    }

    // Setup receiving socket.
    recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (recv_socket < 0) {
        printf("recv socket() failed\n");
        return;
    }
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_addr.sin_port = htons(RECV_PORT);
    if (bind(recv_socket, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        printf("bind() failed\n");
        close(recv_socket);
        recv_socket = -1;
        return;
    }
    // Don't block recvfrom.
    int flags = fcntl(recv_socket, F_GETFL, 0);
    fcntl(recv_socket, F_SETFL, flags | O_NONBLOCK);

    // Starts receiving.
    static pbio_os_process_t process;
    pbio_os_process_start(&process, pdrv_rproc_virtual_process_thread, NULL);
}

bool pbdrv_rproc_is_ready(void) {
    return true;
}

#endif // PBDRV_CONFIG_RPROC_VIRTUAL
