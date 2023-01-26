// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "../../drv/ioport/ioport_test.h"

const pbdrv_ioport_test_platform_data_t
    pbdrv_ioport_test_platform_data[PBDRV_CONFIG_IOPORT_TEST_NUM_PORTS] = {
    // Port A
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    // Port B
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    // Port C
    {
        .type_id = PBIO_IODEV_TYPE_ID_NONE,
    },
    // Port D
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR,
    },
    // Port E
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
    // Port F
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
};
