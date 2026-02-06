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

// Socket used to send data to Python animation.
static int data_socket = -1;
static struct sockaddr_in serv_addr;

void pbdrv_rproc_virtual_socket_send(const uint8_t *data, uint32_t size) {
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

void pbdrv_rproc_init(void) {
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

bool pbdrv_rproc_is_ready(void) {
    return true;
}

#endif // PBDRV_CONFIG_RPROC_VIRTUAL
