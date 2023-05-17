// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <assert.h>
#include <stddef.h>

#include "pbdrv/ioport.h"
#include "pbio/error.h"
#include "pbio/port.h"

/**
 * Gets the size of a data type.
 * @param [in]  type        The data type
 * @return                  The size of the type or 0 if the type was not valid
 */
size_t pbio_iodev_size_of(pbio_iodev_data_type_t type) {
    switch (type & PBIO_IODEV_DATA_TYPE_MASK) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            return 1;
        case PBIO_IODEV_DATA_TYPE_INT16:
            return 2;
        case PBIO_IODEV_DATA_TYPE_INT32:
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            return 4;
    }

    assert(0); // bug if reached

    return 0;
}

/**
 * Gets the binary format used by the current mode of an I/O device.
 * @param [in]  iodev       The I/O device
 * @param [in]  mode        The mode
 * @param [out] len         The number of values in the raw data array
 * @param [out] type        The data type of the raw data values
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 */
pbio_error_t pbio_iodev_get_data_format(pbio_iodev_t *iodev, uint8_t mode, uint8_t *len, pbio_iodev_data_type_t *type) {
    if (iodev->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    if (mode >= iodev->info->num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *len = iodev->info->mode_info[mode].num_values;
    *type = iodev->info->mode_info[mode].data_type;

    return PBIO_SUCCESS;
}
