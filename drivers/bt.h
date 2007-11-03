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


typedef void (*bt_inquiry_callback_t)(bt_device_t *remote);

/* timeout unit: 1.28s ; min : 0x01 (1.28s) ; max: 0x30 (61.44s) */
void bt_inquiry(bt_inquiry_callback_t callback,
                U8 max_devices,
                U8 timeout,
                U8 bt_remote_class[4]);

int bt_checksum_errors();

/* to remove */
void bt_debug();


#endif
