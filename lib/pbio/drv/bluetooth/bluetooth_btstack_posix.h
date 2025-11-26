#ifndef PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_POSIX_H
#define PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_POSIX_H

#include <btstack.h>

const btstack_control_t *pbdrv_bluetooth_btstack_posix_control_instance(void);

const hci_transport_t *pbdrv_bluetooth_btstack_posix_transport_instance(void);

const void *pbdrv_bluetooth_btstack_posix_transport_config(void);

#endif
