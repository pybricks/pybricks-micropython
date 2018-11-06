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
    switch (type) {
    case PBIO_IODEV_DATA_TYPE_INT8:
        return 1;
    case PBIO_IODEV_DATA_TYPE_INT16:
        return 2;
    case PBIO_IODEV_DATA_TYPE_INT32:
    case PBIO_IODEV_DATA_TYPE_FLOAT:
        return 4;
    }

    return 0;
}

/**
 * Gets the raw data from an I/O device.
 * @param [in]  port        The port the device is associated with
 * @param [out] data        Pointer to hold array of data values
 * @param [out] len         The length of the *data* array
 * @param [out] type        The data type of the *data* values
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *
 * The length of *data* in bytes will be *len* times the size of *type*.
 */
pbio_error_t pbio_iodev_get_raw_values(pbio_port_t port, uint8_t **data, uint8_t *len, pbio_iodev_data_type_t *type) {
pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *data = iodev->bin_data;
    *len = iodev->info->mode_info[iodev->mode].num_values;
    *type = iodev->info->mode_info[iodev->mode].data_type;

    return PBIO_SUCCESS;
}

/**
 * Sets the raw data of an I/O device.
 * @param [in]  port        The port the device is associated with
 * @param [in]  data        Array of data values
 * @param [in]  len         The length of the *data* array
 * @param [in]  type        The data type of the *data* values
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_INVALID_ARG if *len* or *type* does not match the current mode
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *                          ::PBIO_ERROR_AGAIN if the device is busy with something else
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support setting values
 *
 * The length of *data* in bytes must be *len* times the size of *type*.
 */
pbio_error_t pbio_iodev_set_raw_values(pbio_port_t port, uint8_t *data, uint8_t len, pbio_iodev_data_type_t type) {
pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!iodev->set_data) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (len != iodev->info->mode_info[iodev->mode].num_values) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (type != iodev->info->mode_info[iodev->mode].data_type) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return iodev->set_data(iodev, data, len, type);
}

/**
 * Writes arbitrary data to an I/O device.
 * @param [in]  port        The port the device is associated with
 * @param [in]  data        Pointer to raw data to write
 * @param [in]  len         Length of the *data* in bytes
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_INVALID_ARG if *len* is too large
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 *                          ::PBIO_ERROR_AGAIN if the device is busy with something else
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support writing
 */
pbio_error_t pbio_iodev_write(pbio_port_t port, uint8_t *data, uint8_t len) {
pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!iodev->write) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return iodev->write(iodev, data, len);
}

/**
 * Sets the mode of an I/O device.
 * @param [in]  port        The port the device is associated with
 * @param [in]  mode        The new mode
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the device does not support setting the mode
 */
pbio_error_t pbio_iodev_set_mode(pbio_port_t port, uint8_t mode) {
    pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!iodev->set_mode) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (mode >= iodev->info->num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return iodev->set_mode(iodev, mode);
}
