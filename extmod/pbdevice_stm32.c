// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "pbdevice.h"

struct _pbdevice_t {
    pbio_iodev_t *iodev;
};

pbio_error_t pbdevice_get_device(pbdevice_t **pbdev, pbio_iodev_type_id_t valid_id, pbio_port_t port) {
    pbio_iodev_t *iodev;
    pbio_error_t err;
    
    // Get the iodevice
    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *pbdev = (pbdevice_t *) iodev;
 
    // Verify ID
    pbio_iodev_type_id_t actual_id;
    err = pbdevice_get_type_id(*pbdev, &actual_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    if (actual_id != valid_id) {
        return PBIO_ERROR_NO_DEV;
    }

    return PBIO_SUCCESS;
}


pbio_error_t pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, void *values) {
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbdevice_get_type_id(pbdevice_t *pbdev, pbio_iodev_type_id_t *id) {
    if (!pbdev->iodev->info) {
        return PBIO_ERROR_NO_DEV;
    }
    *id = pbdev->iodev->info->type_id;
    return PBIO_SUCCESS;
}
