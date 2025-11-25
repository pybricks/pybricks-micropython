#ifndef PBIO_DRV_BLUETOOTH_BLUETOOTH_BTSTACK_CONTROL_EV3
#define PBIO_DRV_BLUETOOTH_BLUETOOTH_BTSTACK_CONTROL_EV3

#include <pbdrvconfig.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3

#include <btstack.h>

#include <pbio/os.h>

// Returns the control instance for the EV3. This does a little more than
// the typical control instance -- it's responsible for configuring the slow
// clock and configuring certain pins that interfere with bluetooth operation.
const btstack_control_t *pbdrv_bluetooth_control_ev3_instance(void);

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3

#endif // PBIO_DRV_BLUETOOTH_BLUETOOTH_BTSTACK_CONTROL_EV3
