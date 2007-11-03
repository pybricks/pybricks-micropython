#ifndef __NXOS_BT_H__
#define __NXOS_BT_H__

#include "base/types.h"

#include "base/drivers/uart.h"

/*
 * It will only initialize the communication with the bluetooth
 * coprocessor.
 */
void nx_bt_init();

#endif
