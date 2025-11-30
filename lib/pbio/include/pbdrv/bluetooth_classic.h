#ifndef PBIO_INCLUDE_PBDRV_BLUETOOTH_CLASSIC_H
#define PBIO_INCLUDE_PBDRV_BLUETOOTH_CLASSIC_H

#include <pbdrvconfig.h>

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC

#include <stdint.h>
#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/os.h>

/**
 * Initializes the Bluetooth driver.
 */
void pbdrv_bluetooth_init(void);

/**
 * Deinitializes the Bluetooth driver.
 */
void pbdrv_bluetooth_deinit(void);

/**
 * Gets the bluetooth hub name.
 */
const char *pbdrv_bluetooth_get_hub_name(void);

#endif  // PBDRV_CONFIG_BLUETOOTH_CLASSIC

#endif
