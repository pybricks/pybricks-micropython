#ifndef PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_CLASSIC_H
#define PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_CLASSIC_H
#include <pbdrvconfig.h>

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC

#include <stdint.h>

void pbdrv_bluetooth_classic_init();

#else

static inline void pbdrv_bluetooth_classic_init() {
}

#endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC

#endif  // PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_CLASSIC_H
