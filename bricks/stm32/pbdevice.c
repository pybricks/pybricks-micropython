// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 David Lechner

#include <string.h>

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "pberror.h"
#include "pbdevice.h"

struct _pbdevice_t {
    pbio_iodev_t iodev;
};

static void wait(pbio_error_t (*end)(pbio_iodev_t *), void (*cancel)(pbio_iodev_t *), pbio_iodev_t* iodev) {
    nlr_buf_t nlr;
    pbio_error_t err;

    if (nlr_push(&nlr) == 0) {
        while ((err = end(iodev)) == PBIO_ERROR_AGAIN) {
            MICROPY_EVENT_POLL_HOOK
        }
        nlr_pop();
        pb_assert(err);
    } else {
        cancel(iodev);
        while (end(iodev) == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_raise(nlr.ret_val);
    }
}

static void set_mode(pbio_iodev_t *iodev, uint8_t new_mode) {
    pbio_error_t err;

    if (iodev->mode == new_mode){
        return;
    }

    while ((err = pbio_iodev_set_mode_begin(iodev, new_mode)) == PBIO_ERROR_AGAIN);
    pb_assert(err);
    wait(pbio_iodev_set_mode_end, pbio_iodev_set_mode_cancel, iodev);
}

pbdevice_t *pbdevice_get_device(pbio_port_t port, pbio_iodev_type_id_t valid_id) {

    // Get the iodevice
    pbio_iodev_t *iodev;
    pb_assert(pbdrv_ioport_get_iodev(port, &iodev));

    // Verify the ID
    if (!iodev->info || iodev->info->type_id != valid_id) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Return pointer to device
    return (pbdevice_t *) iodev;
}

void pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, void *values) {

    pbio_iodev_t *iodev = &pbdev->iodev;

    uint8_t *data;
    uint8_t len;
    pbio_iodev_data_type_t type;

    set_mode(iodev, mode);

    pb_assert(pbio_iodev_get_data(iodev, &data));
    pb_assert(pbio_iodev_get_data_format(iodev, iodev->mode, &len, &type));

    if (len == 0) {
        pb_assert(PBIO_ERROR_IO);
    }

    for (uint8_t i = 0; i < len; i++) {
        switch (type) {
            case PBIO_IODEV_DATA_TYPE_UINT8:
            memcpy((int8_t  *) values + i * 1, (uint8_t *)(data + i * 1), 1);
            break;
        case PBIO_IODEV_DATA_TYPE_INT8:
            memcpy((int8_t  *) values + i * 1, (int8_t  *)(data + i * 1), 1);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            memcpy((int8_t  *) values + i * 2, (int16_t *)(data + i * 2), 2);
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            memcpy((int8_t  *) values + i * 4, (int32_t *)(data + i * 4), 4);
            break;
        #if MICROPY_PY_BUILTINS_FLOAT
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            memcpy((int8_t  *) values + i * 4, (float   *)(data + i * 4), 4);
            break;
        #endif
        default:
            pb_assert(PBIO_ERROR_IO);
        }
    }
}

pbio_iodev_type_id_t pbdevice_get_type_id(pbdevice_t *pbdev) {

    if (!pbdev->iodev.info) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
    return pbdev->iodev.info->type_id;
}
