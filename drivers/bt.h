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

/**
 * Size of a bluetooth address (in bytes)
 */
#define BT_ADDR_SIZE  7

/**
 * Size of a bluetooth class (in bytes)
 */
#define BT_CLASS_SIZE 4

/**
 * Maximum length of bluetooth friendly name (in bytes)
 */
#define BT_NAME_MAX_LNG 16

/**
 * Maximum length of bluetooth pin code (in bytes)
 */
#define BT_PIN_MAX_LNG  16



typedef struct bt_device {
  U8 addr[BT_ADDR_SIZE];
  char name[BT_NAME_MAX_LNG+1];
  U8 class[BT_CLASS_SIZE];
} bt_device_t;


typedef enum {
  BT_STATE_WAITING = 0x0,
  BT_STATE_INQUIRING,
  BT_STATE_KNOWN_DEVICES_DUMPING,
  BT_STATE_STREAMING
} bt_state_t;

typedef enum {
  BT_NOTHING = 0x0, /* can mean no answer from the bluecore */
  BT_LR_SUCCESS = 0x50,
  BT_LR_COULD_NOT_SAVE,
  BT_LR_STORE_IS_FULL,
  BT_LR_ENTRY_REMOVED,
  BT_LR_UNKNOWN_ADDR
} bt_return_value_t;

typedef enum {
  BT_DISCONNECTION_SUCCESS,
  BT_DISCONNECTION_LINK_LOSS,
  BT_DISCONNECTION_NO_SLC, /* Service Level Connection */
  BT_DISCONNECTION_TIMEOUT,
  BT_DISCONNECTION_ERROR
} bt_disconnection_status_t;

typedef struct bt_version {
  U8 major;
  U8 minor;
} bt_version_t;



/**
 * It will only initialize the communication with the bluecore
 */
void nx_bt_init(void);

bt_state_t nx_bt_get_state(void);


/**
 * @param[in] name Max 16 car. !
 */
void nx_bt_set_friendly_name(char *name);
void nx_bt_set_discoverable(bool d);


/**
 * @param timeout unit: 1.28s ; min : 0x01 (1.28s) ; max: 0x30 (61.44s)
 */
void nx_bt_begin_inquiry(U8 max_devices,
			 U8 timeout,
			 U8 bt_remote_class[BT_CLASS_SIZE]);
bool nx_bt_has_found_device(void);

/**
 * @param[out] dev will fill in the structure
 */
bool nx_bt_get_discovered_device(bt_device_t *dev);
void nx_bt_cancel_inquiry(void);


void nx_bt_begin_known_devices_dumping(void);
bool nx_bt_has_known_device(void);
bool nx_bt_get_known_device(bt_device_t *dev);

/**
 * @param[in] dev need to be fully filled in
 */
bt_return_value_t nx_bt_add_known_device(bt_device_t *dev);
bt_return_value_t nx_bt_remove_known_device(U8 dev_addr[BT_ADDR_SIZE]);


/**
 * @return the firmware version
 */
bt_version_t nx_bt_get_version(void);

/**
 * @param[out] name will be filled in with the friendly name. An '\0' will be appended,
 *             so this area must have at least a size of (BT_NAME_MAX_LNG+1).
 * @return name length, 0 if failure
 */
int nx_bt_get_friendly_name(char *name);

/**
 * @param[out] addr will be filled in with the local address name. An '\0' will
 *             be appended, so this area must have a size of 7.
 * @return address length, 0 if failure
 */
int nx_bt_get_local_addr(U8 *addr);

/**
 * return the number of messages from the BC4
 * with a wring checksum. Should be 0.
 */
int nx_bt_checksum_errors(void);


/**
 * Indicates if a device is waiting for a pin code
 */
bool nx_bt_has_dev_waiting_for_pin(void);

/**
 * will only send the pin code if nx_has_dev_waiting_for_pin() returning true
 * @param code must finished with a '\0' && max 16 chars ('\0' excluded)
 */
void nx_bt_send_pin(char *code);


/**
 * @return port handle or -1 if failure
 */
int nx_bt_open_port(void);
bool nx_bt_close_port(int handle);

/**
 * @return true if an host want to connect to us
 */
bool nx_bt_connection_pending(void);

/**
 * Only valid if nx_connection_pending() return true.
 * If you accept the connection, a pin code may be required.
 * @param accept specify if we accept the connection of not
 */
void nx_bt_accept_connection(bool accept);


/**
 * @return -1 if no new connexion has been established, else it
 * returns the corresponding handle (only returned once !)
 */
int nx_bt_connection_established(void);

U8 nx_bt_get_link_quality(int handle);

/**
 * Close the specified connexion
 */
bt_disconnection_status_t nx_bt_close_connection(int handle);

/**
 * Only one data stream can be opened at the same time.
 * @note As long as the stream is opened, don't use any other function
 * than nx_bt_stream_write() or nx_bt_stream_has_data() or nx_bt_stream_get_buffer()
 */
void nx_bt_stream_open(int handle);

/**
 * Only valid if a stream has been opened
 * @note : Don't free/erase the data pointed by data until nx_bt_stream_writing_finished() return TRUE
 */
void nx_bt_stream_write(U8 *data, U32 length);

/**
 * Indicates when the data have been transmitted to
 * the BlueCore and can be freed/erased from the memory.
 */
bool nx_bt_stream_data_written(void);

/**
 * Only valid if a stream is (or was) opened.
 * @return FALSE if the remote device or the bluecore
 *         has shuted down the stream (you don't need to
 *         close the stream in this case)
 */
bool nx_bt_stream_opened(void);

/**
 * Specify a memory area where to put the data
 * read from the stream
 * @note reading is only possible if in stream mode
 */
void nx_bt_stream_read(U8 *data, U32 length);

/**
 * Indicates when some data have been read.
 * @note initial value = 0 ; reset to 0 after each call to nx_bt_stream_read()
 * @return number of bytes read
 */
U32 nx_bt_stream_data_read(void);

/**
 * Close a currently opened stream.
 */
void nx_bt_stream_close(void);


/* to remove */
void nx_bt_debug(void);


#endif
