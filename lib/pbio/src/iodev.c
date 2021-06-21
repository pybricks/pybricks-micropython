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

/**
 * Gets the raw data from an I/O device.
 * @param [in]  iodev       The I/O device
 * @param [out] data        Pointer to hold array of data values
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *
 * The binary format and size of *data* is determined by ::pbio_iodev_get_data_format().
 */
pbio_error_t pbio_iodev_get_data(pbio_iodev_t *iodev, uint8_t **data) {
    if (iodev->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    *data = iodev->bin_data;

    return PBIO_SUCCESS;
}

/**
 * Sets the mode of an I/O device.
 * @param [in]  iodev       The I/O device
 * @param [in]  mode        The new mode
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support setting the mode
 */
pbio_error_t pbio_iodev_set_mode_begin(pbio_iodev_t *iodev, uint8_t mode) {
    if (!iodev->ops->set_mode_begin) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (mode >= iodev->info->num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return iodev->ops->set_mode_begin(iodev, mode);
}

pbio_error_t pbio_iodev_set_mode_end(pbio_iodev_t *iodev) {
    if (!iodev->ops->set_mode_end) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return iodev->ops->set_mode_end(iodev);
}

void pbio_iodev_set_mode_cancel(pbio_iodev_t *iodev) {
    if (!iodev->ops->set_mode_cancel) {
        return;
    }

    iodev->ops->set_mode_cancel(iodev);
}

/**
 * Sets the raw data of an I/O device.
 * @param [in]  iodev       The I/O device
 * @param [in]  mode        The mode
 * @param [in]  data        Array of data values
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *                          ::PBIO_ERROR_AGAIN if the device is busy with something else
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support setting values
 *                          ::PBIO_ERROR_INVALID_OP if the current mode does not match the requested mode
 *
 * The binary format and size of *data* is determined by ::pbio_iodev_get_data_format().
 */
pbio_error_t pbio_iodev_set_data_begin(pbio_iodev_t *iodev, uint8_t mode, const uint8_t *data) {
    if (!iodev->ops->set_data_begin) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (iodev->mode != mode) {
        return PBIO_ERROR_INVALID_OP;
    }

    return iodev->ops->set_data_begin(iodev, data);
}

pbio_error_t pbio_iodev_set_data_end(pbio_iodev_t *iodev) {
    if (!iodev->ops->set_data_end) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return iodev->ops->set_data_end(iodev);
}

void pbio_iodev_set_data_cancel(pbio_iodev_t *iodev) {
    if (!iodev->ops->set_data_cancel) {
        return;
    }

    iodev->ops->set_data_cancel(iodev);
}

/**
 * Writes arbitrary data to an I/O device.
 * @param [in]  iodev       The I/O device
 * @param [in]  data        Pointer to raw data to write
 * @param [in]  size        Size of the *data* in bytes
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if *size* is too large
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *                          ::PBIO_ERROR_AGAIN if the device is busy with something else
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support writing
 */
pbio_error_t pbio_iodev_write_begin(pbio_iodev_t *iodev, const uint8_t *data, uint8_t size) {
    if (!iodev->ops->write_begin) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return iodev->ops->write_begin(iodev, data, size);
}

pbio_error_t pbio_iodev_write_end(pbio_iodev_t *iodev) {
    if (!iodev->ops->write_end) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return iodev->ops->write_end(iodev);
}

void pbio_iodev_write_cancel(pbio_iodev_t *iodev) {
    if (!iodev->ops->write_cancel) {
        return;
    }

    iodev->ops->write_cancel(iodev);
}
