/** @file bt.h
 *  @brief Manage the communication with the bluecore.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */


#ifndef __NXOS_BASE_DRIVERS_BT_H__
#define __NXOS_BASE_DRIVERS_BT_H__

#include "base/types.h"

#include "base/drivers/_uart.h"

#define BT_ADDR_SIZE  7
#define BT_CLASS_SIZE 4
#define BT_NAME_MAX_LNG 16

typedef struct bt_device {
  U8 addr[BT_ADDR_SIZE];
  char name[17];
  U8 class[BT_CLASS_SIZE];
} bt_device_t;


typedef enum {
  BT_STATE_WAITING = 0x0,
  BT_STATE_INQUIRING,
  BT_STATE_KNOWN_DEVICES_DUMPING,
} bt_state_t;


/**
 * It will only initialize the communication with the bluetooth
 * coprocessor.
 */
void nx_bt_init();

bt_state_t nx_bt_get_state();


/**
 * @param name Max 16 car. !
 */
void nx_bt_set_friendly_name(char *name);
void nx_bt_set_discoverable(bool d);


/**
 * @param timeout unit: 1.28s ; min : 0x01 (1.28s) ; max: 0x30 (61.44s)
 */
void nx_bt_begin_inquiry(U8 max_devices,
			 U8 timeout,
			 U8 bt_remote_class[4]);
bool nx_bt_has_found_device();
bt_device_t *nx_bt_get_discovered_device();
void nx_bt_cancel_inquiry();


void nx_bt_begin_known_devices_dumping();
bool nx_bt_has_known_device();
bt_device_t *nx_bt_get_known_device();

void nx_bt_add_known_device(bt_device_t *dev);
void nx_bt_remove_device(bt_device_t *dev);



/**
 * return the number of messages from the BC4
 * with a wring checksum. Should be 0.
 */
int nx_bt_checksum_errors();

/* to remove */
void nx_bt_debug();


#endif
