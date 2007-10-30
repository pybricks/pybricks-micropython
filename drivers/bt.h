#ifndef __NXOS_BT_H__
#define __NXOS_BT_H__

#include "base/mytypes.h"

#include "base/drivers/uart.h"

/*
 * It will only initialize the communication with the bluetooth
 * coprocessor.
 */
void bt_init();


void bt_debug();

#endif
