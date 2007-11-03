#ifndef __NXOS_BT_H__
#define __NXOS_BT_H__

#include "base/mytypes.h"

#include "base/drivers/uart.h"

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


/*
 * It will only initialize the communication with the bluetooth
 * coprocessor.
 */
void bt_init();

bt_state_t bt_get_state();


/*
 * Max 16 car. !
 */
void bt_set_friendly_name(char *name);
void bt_set_discoverable(bool d);


/* timeout unit: 1.28s ; min : 0x01 (1.28s) ; max: 0x30 (61.44s) */
void bt_begin_inquiry(U8 max_devices,
                      U8 timeout,
                      U8 bt_remote_class[4]);
bool bt_has_found_device();
bt_device_t *bt_get_discovered_device();
void bt_cancel_inquiry();


void bt_begin_known_devices_dumping();
bool bt_has_known_device();
bt_device_t *bt_get_known_device();

void bt_add_known_device(bt_device_t *dev);
void bt_remove_device(bt_device_t *dev);



/**
 * return the number of messages from the BC4
 * with a wring checksum. Should be 0.
 */
int bt_checksum_errors();

/* to remove */
void bt_debug();


#endif
