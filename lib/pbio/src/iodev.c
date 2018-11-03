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
 * @param [out] data        Pointer that will point to the raw data
 * @param [out] len         The length of *data*
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached
 */
pbio_error_t pbio_iodev_get_raw_values(pbio_port_t port, uint8_t **data, uint8_t *len) {
pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *data = iodev->bin_data;
    *len = pbio_iodev_size_of(iodev->info->mode_info[iodev->mode].data_type) *
           iodev->info->mode_info[iodev->mode].num_values;

    return PBIO_SUCCESS;
}

/**
 * Sets the mode of an I/O device.
 * @param [in]  port        The port the device is associated with
 * @param [in]  mode        The new mode
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_PORT if the port is not valid
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid
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

    return iodev->set_mode(iodev, mode);
}
