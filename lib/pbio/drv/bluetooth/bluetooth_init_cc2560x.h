#ifndef PBIO_DRV_BLUETOOTH_BLUETOOTH_INIT_CC2560X_H_
#define PBIO_DRV_BLUETOOTH_BLUETOOTH_INIT_CC2560X_H_

#include <stdint.h>
#include <pbio/error.h>
#include <stdint.h>

typedef struct {
    const uint8_t *script;
    uint32_t script_size;
} pbdrv_bluetooth_init_script_t;

// Returns the init script for the given LMP subversion. If no matching script
// is found, returns PBIO_ERROR_NOT_SUPPORTED.
//
// This function is intended for the EV3, where some EV3s have a later revision
// of the bluetooth module.
pbio_error_t pbdrv_bluetooth_get_init_script(uint16_t lmp_subversion, pbdrv_bluetooth_init_script_t *result);

#endif // PBIO_DRV_BLUETOOTH_BLUETOOTH_INIT_CC2560X_H_
