#ifndef __NXOS_BT_H__
#define __NXOS_BT_H__

#include "base/mytypes.h"

#include "base/drivers/uart.h"

/*
 * It will only initialize the communication with the bluetooth
 * coprocessor.
 */
void bt_init();

/*
 * Max 16 car. !
 */
void bt_set_friendly_name(char *name);
void bt_set_discoverable(bool d);

/* to remove */
void bt_debug();

#endif
