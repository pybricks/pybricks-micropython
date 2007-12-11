/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/util.h"
#include "base/display.h"
#include "base/drivers/systick.h"
#include "base/drivers/_uart.h"

#include "base/drivers/bt.h"

#define BT_ACK_TIMEOUT 3000
#define BT_ARGS_BUFSIZE (BT_NAME_MAX_LNG+1)

/* to remove : */
/*#define UART_DEBUG*/
#ifdef UART_DEBUG
#include "base/display.h"
#include "base/drivers/usb.h"
#include "base/util.h"
#define CMDS_BUFSIZE 128
#define USB_SEND(txt) nx_usb_write((U8*)txt, strlen(txt))
#else
#define CMDS_BUFSIZE 0
#define USB_SEND(txt)
#endif


#define BT_RST_PIN AT91C_PIO_PA11
#define BT_ARM_CMD_PIN AT91C_PIO_PA27
/* BT_BC4_CMD is connected to the channel 4 (counting from 0) of the Analog to Digital converter */
#define BT_CS_PIN   AT91C_PIO_PA31


/*** MESSAGES CODES ***/

typedef enum {
  /* AMR7 -> BC4 */
  BT_MSG_BEGIN_INQUIRY = 0x00,
  BT_MSG_CANCEL_INQUIRY = 0x01,
  BT_MSG_CONNECT = 0x02,
  BT_MSG_OPEN_PORT = 0x03,
  BT_MSG_LOOKUP_NAME = 0x04,
  BT_MSG_ADD_DEVICE = 0x05,
  BT_MSG_REMOVE_DEVICE = 0x06,
  BT_MSG_DUMP_LIST = 0x07,
  BT_MSG_CLOSE_CONNECTION = 0x08,
  BT_MSG_ACCEPT_CONNECTION = 0x09,
  BT_MSG_PIN_CODE = 0x0A,
  BT_MSG_OPEN_STREAM = 0x0B,
  BT_MSG_START_HEART = 0x0C,
  BT_MSG_SET_DISCOVERABLE = 0x1C,
  BT_MSG_CLOSE_PORT = 0x1D,
  BT_MSG_SET_FRIENDLY_NAME = 0x21,
  BT_MSG_GET_LINK_QUALITY = 0x23,
  BT_MSG_SET_FACTORY_SETTINGS = 0x25,
  BT_MSG_GET_LOCAL_ADDR = 0x26,
  BT_MSG_GET_FRIENDLY_NAME = 0x29,
  BT_MSG_GET_DISCOVERABLE = 0x2A,
  BT_MSG_GET_PORT_OPEN = 0x2B,
  BT_MSG_GET_VERSION = 0x2F,
  BT_MSG_GET_BRICK_STATUS_BYTE = 0x33,
  BT_MSG_SET_BRICK_STATUS_BYTE = 0x34,
  BT_MSG_GET_OPERATING_MODE = 0x35,
  BT_MSG_SET_OPERATING_MODE = 0x36,
  BT_MSG_GET_CONNECTION_STATUS = 0x38,
  BT_MSG_GOTO_DFU_MODE = 0x3A,

  /* BC4 -> ARM7 */
  BT_MSG_HEARTBEAT = 0x0D,
  BT_MSG_INQUIRY_RUNNING = 0x0E,
  BT_MSG_INQUIRY_RESULT = 0x0F,
  BT_MSG_INQUIRY_STOPPED = 0x10,
  BT_MSG_LOOKUP_NAME_RESULT = 0x11,
  BT_MSG_LOOKUP_NAME_FAILURE = 0x12,
  BT_MSG_CONNECT_RESULT = 0x13,
  BT_MSG_RESET_INDICATION = 0x14,
  BT_MSG_REQUEST_PIN_CODE = 0x15,
  BT_MSG_REQUEST_CONNECTION = 0x16,
  BT_MSG_LIST_RESULT = 0x17,
  BT_MSG_LIST_ITEM = 0x18,
  BT_MSG_LIST_DUMP_STOPPED = 0x19,
  BT_MSG_CLOSE_CONNECTION_RESULT = 0x1A,
  BT_MSG_PORT_OPEN_RESULT = 0x1B,
  BT_MSG_CLOSE_PORT_RESULT = 0x1E,
  BT_MSG_PIN_CODE_ACK = 0x1F,
  BT_MSG_SET_DISCOVERABLE_ACK = 0x20,
  BT_MSG_SET_FRIENDLY_NAME_ACK = 0x22,
  BT_MSG_LINK_QUALITY_RESULT = 0x24,
  BT_MSG_SET_FACTORY_SETTINGS_ACK = 0x26,
  BT_MSG_GET_LOCAL_ADDR_RESULT = 0x28,
  BT_MSG_GET_FRIENDLY_NAME_RESULT = 0x2C,
  BT_MSG_GET_DISCOVERABLE_RESULT = 0x2D,
  BT_MSG_GET_PORT_OPEN_RESULT = 0x2E,
  BT_MSG_GET_VERSION_RESULT = 0X30,
  BT_MSG_GET_BRICK_STATUS_BYTE_RESULT = 0x31,
  BT_MSG_SET_BRICK_STATUS_BYTE_RESULT = 0x32,
  BT_MSG_OPERATING_MODE_RESULT = 0x37,
  BT_MSG_CONNECTION_STATUS_RESULT = 0x39,
} bt_msg_t;



/*** Predefined messages ***/


static const U8 bt_msg_start_heart[] = {
  0x03, /* length */
  BT_MSG_START_HEART,
  0xFF, /* checksum (high) */
  0xF4  /* checksum (low) */
};


static const U8 bt_msg_get_version[] = {
  0x03, /* length */
  BT_MSG_GET_VERSION,
  0xFF, /* checksum (hi) */
  0xD1  /* checksum (lo) */
};


static const U8 bt_msg_set_discoverable_true[] = {
  0x04, /* length */
  BT_MSG_SET_DISCOVERABLE,
  0x01, /* => true */
  0xFF,
  0xE3
};

static const U8 bt_msg_set_discoverable_false[] = {
  0x04, /* length */
  BT_MSG_SET_DISCOVERABLE, /* set discoverable */
  0x00, /* => true */
  0xFF,
  0xE4
};

static const U8 bt_msg_cancel_inquiry[] = {
  0x03, /* length */
  BT_MSG_CANCEL_INQUIRY,
  0xFF,
  0xFF
};


/* dump list of known devices */
static const U8 bt_msg_dump_list[] = {
  0x03,
  BT_MSG_DUMP_LIST,
  0xFF,
  0xF9
};


static const U8 bt_msg_get_friendly_name[] = {
  0x03,
  BT_MSG_GET_FRIENDLY_NAME,
  0xFF,
  0xD7
};


static const U8 bt_msg_open_port[] = {
  0x03,
  BT_MSG_OPEN_PORT,
  0xFF,
  0xFD
};


static const U8 bt_msg_accept_connection[] = {
  0x04,
  BT_MSG_ACCEPT_CONNECTION,
  0x01,
  0xFF,
  0xF6
};

static const U8 bt_msg_refuse_connection[] = {
  0x04,
  BT_MSG_ACCEPT_CONNECTION,
  0x00,
  0xFF,
  0xF7
};



static volatile struct {
  bt_state_t state;

  /* see nx_systick_get_ms() */
  U32 last_heartbeat;

  /* used for inquiring */
  U32 last_checked_id, remote_id; /* used to know when a new device is found */
  bt_device_t remote_device;

  /* all bytes to 0 if no device is waiting a pin code */
  U8 dev_waiting_for_pin[BT_ADDR_SIZE];
  /* all bytes to 0 if no device is waiting to connect */
  U8 dev_waiting_for_connection[BT_ADDR_SIZE];
  /* set when a new connection has been established. */
  int new_handle;

  U8 last_msg;

  /* use to return various return value (version, etc) */
  U8 args[BT_ARGS_BUFSIZE];

  int nmb_checksum_errors;

#ifdef UART_DEBUG
  /* to remove: */
  U8 cmds[CMDS_BUFSIZE];
  U32 cmds_pos;
#endif

} bt_state;




/* len => checksum included
 */
static U32 bt_get_checksum(U8 *msg, U32 len, bool count_len)
{
  U32 i;
  U32 checksum;

  checksum = 0;

  /* from the second byte of the message
   * to the last byte before the checksum */
  for (i = 0 ; i < len-2 ; i++) {
    checksum += msg[i];
  }

  if (count_len)
    checksum += len;

  checksum = -checksum;

  return checksum;
}


/* len => length excepted, but checksum included
 * two last bytes will be set
 */
static void bt_set_checksum(U8 *msg, U32 len) {
  U32 checksum = bt_get_checksum(msg, len, FALSE);

  msg[len-2] = ((checksum >> 8) & 0xFF);
  msg[len-1] = checksum & 0xFF;
}


/* len => length excepted, but checksum included */
static bool bt_check_checksum(U8 *msg, U32 len) {

  /* Strangeness: Must include the packet length in the checksum ?! */
  U32 checksum = bt_get_checksum(msg, len, TRUE);
  U32 hi, lo;

  hi = ((checksum >> 8) & 0xFF);
  lo = checksum & 0xFF;

  return (hi == msg[len-2] && lo == msg[len-1]);
}


static bool bt_wait_msg(U8 msg)
{
  U32 start = nx_systick_get_ms();

  while(bt_state.last_msg != msg && start+BT_ACK_TIMEOUT > nx_systick_get_ms());

  return bt_state.last_msg == msg;
}


static void bt_reseted()
{
  bt_state.state = BT_STATE_WAITING;
  //nx__uart_write(bt_msg_start_heart, sizeof(bt_msg_start_heart));
}




static void bt_uart_command_callback(U8 *msg, U32 len)
{
  U32 i;

  /* if it's a break from the nxt */
  if (msg == NULL || len == 0) {
    bt_state.nmb_checksum_errors++;
    return;
  }

  /* we check first the checksum and ignore the message if the checksum is invalid */
  if (len < 1 || !bt_check_checksum(msg, len)) {
    bt_state.nmb_checksum_errors++;
    return;
  }

#ifdef UART_DEBUG
  if (bt_state.cmds_pos < CMDS_BUFSIZE) {
    bt_state.cmds[bt_state.cmds_pos] = msg[0];
    bt_state.cmds_pos++;
  }
#endif

  bt_state.last_msg = msg[0];

  /* we copy the arguments */
  for (i = 0 ; i < len-3 && i < BT_ARGS_BUFSIZE; i++) {
    bt_state.args[i] = msg[i+1];
  }

  for (; i < BT_ARGS_BUFSIZE ; i++) {
    bt_state.args[i] = 0;
  }


  if (msg[0] == BT_MSG_HEARTBEAT) {
    bt_state.last_heartbeat = nx_systick_get_ms();
    return;
  }


  if (msg[0] == BT_MSG_INQUIRY_RESULT) {
    if (bt_state.state != BT_STATE_INQUIRING)
      return;

    /* if the last device written wasn't fetched in the user
     * space, we don't erase it to avoid corrupted structures */
    if (bt_state.last_checked_id != bt_state.remote_id) {
      return;
    }

    /* we've found a device => we extract the fields */

    for (i = 0 ; i < BT_ADDR_SIZE ; i++)
      bt_state.remote_device.addr[i] = msg[1+i];

    for (i = 0 ; i < BT_NAME_MAX_LNG ; i++)
      bt_state.remote_device.name[i] = msg[1+BT_ADDR_SIZE+i];
    bt_state.remote_device.name[BT_NAME_MAX_LNG+1] = '\0';

    for (i = 0 ; i < BT_CLASS_SIZE ; i++)
      bt_state.remote_device.class[i] = msg[1+BT_ADDR_SIZE+BT_NAME_MAX_LNG+i];

    bt_state.remote_id++;
    return;
  }


  if (msg[0] == BT_MSG_LIST_ITEM) {
    if (bt_state.state != BT_STATE_KNOWN_DEVICES_DUMPING)
      return;

    /* if the last device written wasn't fetched in the user
     * space, we don't erase it to avoid corrupted structures */
    if (bt_state.last_checked_id != bt_state.remote_id) {
      return;
    }

    for (i = 0 ; i < BT_ADDR_SIZE ; i++)
      bt_state.remote_device.addr[i] = msg[1+i];

    for (i = 0 ; i < BT_NAME_MAX_LNG ; i++)
      bt_state.remote_device.name[i] = msg[1+BT_ADDR_SIZE+i];
    bt_state.remote_device.name[BT_NAME_MAX_LNG+1] = '\0';

    for (i = 0 ; i < BT_CLASS_SIZE ; i++)
      bt_state.remote_device.class[i] = msg[1+BT_ADDR_SIZE+BT_NAME_MAX_LNG+i];

    bt_state.remote_id++;

    return;
  }


  if (msg[0] == BT_MSG_INQUIRY_STOPPED) {
    if (bt_state.state == BT_STATE_INQUIRING)
      bt_state.state = BT_STATE_WAITING;
    return;
  }

  if (msg[0] == BT_MSG_LIST_DUMP_STOPPED) {
    if (bt_state.state == BT_STATE_KNOWN_DEVICES_DUMPING)
      bt_state.state = BT_STATE_WAITING;
    return;
  }

  if (msg[0] == BT_MSG_RESET_INDICATION) {
    bt_reseted();
    return;
  }

  if (msg[0] == BT_MSG_REQUEST_PIN_CODE) {
    for (i = 0 ; i < BT_ADDR_SIZE ; i++) {
      bt_state.dev_waiting_for_pin[i] = bt_state.args[i];
    }

    return;
  }

  if (msg[0] == BT_MSG_REQUEST_CONNECTION) {
    for (i = 0 ; i < BT_ADDR_SIZE ; i++) {
      bt_state.dev_waiting_for_connection[i] = bt_state.args[i];
    }

    return;
  }

  if (msg[0] == BT_MSG_CONNECT_RESULT) {
    if (bt_state.args[0] >= 1)
      bt_state.new_handle = bt_state.args[1];
    return;
  }
}


void nx_bt_init()
{
  memset((void*)&bt_state, 0, sizeof(bt_state));
  USB_SEND("nx_bt_init()");

  bt_state.new_handle = -1;

  /* we put the ARM CMD pin to 0 => command mode */
  /* and we put the RST PIN to 1 => Will release the reset on the bluecore */
  *AT91C_PIOA_PER = BT_RST_PIN | BT_ARM_CMD_PIN;
  *AT91C_PIOA_PPUDR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_SODR = BT_RST_PIN;
  *AT91C_PIOA_OER = BT_RST_PIN | BT_ARM_CMD_PIN;

  nx_systick_wait_ms(100);

  nx__uart_init(bt_uart_command_callback);

  bt_wait_msg(BT_MSG_RESET_INDICATION);
  /* the function bt_uart_callback() should start the heart after receiving the reset indication */

  USB_SEND("nx_bt_init() finished");
}



void nx_bt_set_friendly_name(char *name)
{
  int i;
  U8 packet[20] = { 0 };

  USB_SEND("set_friendly_name()");

  packet[0] = 19; /* length */
  packet[1] = BT_MSG_SET_FRIENDLY_NAME;

  for (i = 0 ; i < 16 && name[i] != '\0' ; i++)
    packet[i+2] = name[i];

  for (; i < 16 ; i++)
    packet[i+2] = '\0';

  bt_set_checksum(packet+1, 19);

  do {
    nx__uart_write(packet, 20);
  } while(!bt_wait_msg(BT_MSG_SET_FRIENDLY_NAME_ACK));
}


void nx_bt_set_discoverable(bool d)
{
  USB_SEND("set_discoverable()");

  do {
    if (d)
      nx__uart_write(bt_msg_set_discoverable_true, sizeof(bt_msg_set_discoverable_true));
    else
      nx__uart_write(bt_msg_set_discoverable_false, sizeof(bt_msg_set_discoverable_false));
  } while(!bt_wait_msg(BT_MSG_SET_DISCOVERABLE_ACK));
}


bt_state_t nx_bt_get_state() {
  return bt_state.state;
}



void nx_bt_begin_inquiry(U8 max_devices,
			 U8 timeout,
			 U8 bt_remote_class[4])
{
  int i;
  U8 packet[11];

  packet[0] = 10;      /* length */
  packet[1] = BT_MSG_BEGIN_INQUIRY; /* begin inquiry */
  packet[2] = max_devices;
  packet[3] = 0;       /* timeout (hi) */
  packet[4] = timeout; /* timeout (lo) */

  for (i = 0 ; i < 4 ; i++)
    packet[5+i] = bt_remote_class[i];

  bt_set_checksum(packet+1, 10);

  do {
    nx__uart_write(packet, 11);
  } while(!bt_wait_msg(BT_MSG_INQUIRY_RUNNING));

  bt_state.last_checked_id = 0;
  bt_state.state = BT_STATE_INQUIRING;
}

bool nx_bt_has_found_device()
{
  if (bt_state.state == BT_STATE_INQUIRING)
    return (bt_state.last_checked_id != bt_state.remote_id);
  return FALSE;
}


bool nx_bt_get_discovered_device(bt_device_t *dev)
{
  if (nx_bt_has_found_device()) {

    memcpy(dev, (bt_device_t *)&(bt_state.remote_device), sizeof(bt_device_t));
    bt_state.last_checked_id = bt_state.remote_id;

    return TRUE;
  }

  return FALSE;
}


void nx_bt_cancel_inquiry()
{
  nx__uart_write(bt_msg_cancel_inquiry, sizeof(bt_msg_cancel_inquiry));
  bt_wait_msg(BT_MSG_INQUIRY_STOPPED);
}


void nx_bt_begin_known_devices_dumping()
{
  nx__uart_write(bt_msg_dump_list, sizeof(bt_msg_dump_list));
  bt_state.state = BT_STATE_KNOWN_DEVICES_DUMPING;
}

bool nx_bt_has_known_device()
{
  if (bt_state.state == BT_STATE_KNOWN_DEVICES_DUMPING)
    return (bt_state.last_checked_id != bt_state.remote_id);
  return FALSE;
}

bool nx_bt_get_known_device(bt_device_t *dev)
{
  if (nx_bt_has_known_device()) {

    memcpy(dev, (bt_device_t *)&(bt_state.remote_device), sizeof(bt_device_t));
    bt_state.last_checked_id = bt_state.remote_id;

    return TRUE;
  }

  return FALSE;
}


bt_return_value_t nx_bt_add_known_device(bt_device_t *dev)
{
  int i;
  U8 packet[31];

  packet[0] = 30; /* length */
  packet[1] = BT_MSG_ADD_DEVICE;

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    packet[2+i] = dev->addr[i];

  for (i = 0 ; i < BT_NAME_MAX_LNG && dev->name != '\0' ; i++)
    packet[2+BT_ADDR_SIZE+i] = dev->name[i];

  for ( ; i < BT_NAME_MAX_LNG ; i++)
    packet[2+BT_ADDR_SIZE+i] = '\0';

  for (i = 0 ; i < BT_CLASS_SIZE ; i++)
    packet[2+BT_ADDR_SIZE+BT_NAME_MAX_LNG+i] = dev->class[i];

  bt_set_checksum(packet+1, 30);

  nx__uart_write(packet, 31);

  bt_wait_msg(BT_MSG_LIST_RESULT);

  if (bt_state.last_msg == BT_MSG_LIST_RESULT)
    return bt_state.args[0];

  return 0;
}


bt_return_value_t nx_bt_remove_known_device(U8 dev_addr[BT_ADDR_SIZE])
{
  int i;
  U8 packet[11];

  packet[0] = 10; /* length */
  packet[1] = BT_MSG_REMOVE_DEVICE;

  for (i = 0; i < BT_ADDR_SIZE; i++) {
    packet[2+i] = dev_addr[i];
  }

  bt_set_checksum(packet+1, 10);

  nx__uart_write(packet, 11);

  bt_wait_msg(BT_MSG_LIST_RESULT);

  if (bt_state.last_msg == BT_MSG_LIST_RESULT)
    return bt_state.args[0];

  return 0;
}


bt_version_t nx_bt_get_version()
{
  bt_version_t ver = { 0, 0 };

  nx__uart_write(bt_msg_get_version, sizeof(bt_msg_get_version));

  if (bt_wait_msg(BT_MSG_GET_VERSION_RESULT)) {
    ver.major = bt_state.args[0];
    ver.minor = bt_state.args[1];
  }

  return ver;
}


int nx_bt_get_friendly_name(char *name)
{
  int i;

  nx__uart_write(bt_msg_get_friendly_name, sizeof(bt_msg_get_friendly_name));

  if (bt_wait_msg(BT_MSG_GET_FRIENDLY_NAME_RESULT)) {

    for (i = 0 ;
         i < BT_ARGS_BUFSIZE && i < BT_NAME_MAX_LNG && bt_state.args[i] != '\0' ;
         i++)
      name[i] = bt_state.args[i];

    name[i] = '\0';

    return i;

  } else {

    name[0] = '\0';
    return 0;

  }
}


int nx_bt_checksum_errors()
{
  return bt_state.nmb_checksum_errors;
}


int nx_bt_open_port()
{
  USB_SEND("open_port()");

  nx__uart_write(bt_msg_open_port, sizeof(bt_msg_open_port));

  if (!bt_wait_msg(BT_MSG_PORT_OPEN_RESULT))
    return -1;

  if (bt_state.args[0] == 0) /* status = failed */
    return -1;

  return bt_state.args[1]; /* handle */
}

bool nx_bt_close_port(int handle)
{
  U8 packet[5];

  USB_SEND("close_port()");

  packet[0] = 4; /* length */
  packet[1] = BT_MSG_CLOSE_PORT;
  packet[2] = (U8)handle;

  bt_set_checksum(packet+1, 4);

  nx__uart_write(packet, 5);

  do {
    if (!bt_wait_msg(BT_MSG_CLOSE_PORT_RESULT))
      return FALSE;
  } while (bt_state.args[1] != handle); /* second byte is the handle */

  return (bt_state.args[0] >= 1); /* status byte */
}



bool nx_bt_has_dev_waiting_for_pin()
{
  int i;

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    if (bt_state.dev_waiting_for_pin[i] != 0) {
      USB_SEND("has_dev_waiting_for_pin() => TRUE");
      return TRUE;
    }

  return FALSE;
}


void nx_bt_send_pin(char *code)
{
  int i;
  U8 packet[27];

  if (!nx_bt_has_dev_waiting_for_pin())
    return;

  USB_SEND("send_pin()");

  packet[0] = 26;
  packet[1] = BT_MSG_PIN_CODE;

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    packet[2+i] = bt_state.dev_waiting_for_pin[i];

  for (i = 0 ; i < BT_PIN_MAX_LNG && code[i] != '\0' ; i++)
    packet[2+BT_ADDR_SIZE+i] = code[i];

  for (; i < BT_PIN_MAX_LNG ; i++)
    packet[2+BT_ADDR_SIZE+i] = '\0';

  bt_set_checksum(packet+1, 26);

  nx__uart_write(packet, 27);

  bt_wait_msg(BT_MSG_PIN_CODE_ACK);

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    bt_state.dev_waiting_for_pin[i] = 0;
}


bool nx_bt_connection_pending()
{
  int i;

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    if (bt_state.dev_waiting_for_connection[i] != 0) {
      USB_SEND("connection_pending => TRUE");
      return TRUE;
    }

  return FALSE;
}


void nx_bt_accept_connection(bool accept)
{
  int i;

  if (!nx_bt_connection_pending())
    return;

  USB_SEND("accept_connection()");

  if (accept)
    nx__uart_write(bt_msg_accept_connection, sizeof(bt_msg_accept_connection));
  else
    nx__uart_write(bt_msg_refuse_connection, sizeof(bt_msg_refuse_connection));

  for (i = 0 ; i < BT_ADDR_SIZE ; i++)
    bt_state.dev_waiting_for_connection[i] = 0;

  while(nx__uart_is_writing());
}


int nx_bt_connection_established()
{
  int handle;

  handle = bt_state.new_handle;

  if (handle >= 0) {
    USB_SEND("connection_established() => TRUE");
    bt_state.new_handle = -1;
  }

  return handle;
}


U8 nx_bt_get_link_quality(int handle)
{
  U8 packet[5];

  USB_SEND("get_link_quality()");
  packet[0] = 4;
  packet[1] = BT_MSG_GET_LINK_QUALITY;
  packet[2] = (U8)handle;
  bt_set_checksum(packet+1, 4);

  nx__uart_write(packet+1, 5);

  if (!bt_wait_msg(BT_MSG_LINK_QUALITY_RESULT))
    return FALSE;

  return bt_state.args[0];
}


bt_disconnection_status_t nx_bt_close_connection(int handle)
{
  U8 packet[5];
  int ret_handle = 0;

  USB_SEND("close_connection()");

  packet[0] = 4;
  packet[1] = BT_MSG_CLOSE_CONNECTION;
  packet[2] = (U8)handle;

  bt_set_checksum(packet+1, 4);

  nx__uart_write(packet+1, 5);

  while (ret_handle != handle) {
    if (!bt_wait_msg(BT_MSG_CLOSE_CONNECTION_RESULT))
      return FALSE;
    ret_handle = bt_state.args[1];
  }

  return bt_state.args[0];
}


void nx_bt_stream_open(int handle)
{
  U8 packet[5];

  USB_SEND("stream_open()");

  /* we make sure that the callback won't be called anymore */

  /* send open stream message */

  packet[0] = 4; /* length */
  packet[1] = BT_MSG_OPEN_STREAM;
  packet[2] = (U8)handle;

  bt_set_checksum(packet+1, 4);

  nx__uart_write(packet, 5);

  while(nx__uart_is_writing());

  nx__uart_set_callback(NULL);

  /* set ARM_CMD to high to go in stream mode */
  *AT91C_PIOA_SODR = BT_ARM_CMD_PIN;

  bt_state.state = BT_STATE_STREAMING;
}

void nx_bt_stream_write(U8 *data, U32 length)
{
  USB_SEND("stream_write()");
  nx__uart_write(data, length);
}

bool nx_bt_stream_opened()
{
  return bt_state.state == BT_STATE_STREAMING;
}


bool nx_bt_stream_data_written()
{
  return !(nx__uart_is_writing());
}

void nx_bt_stream_read(U8 *buf, U32 length)
{
  USB_SEND("stream_read()");
  nx__uart_read(buf, length);
}

U32 nx_bt_stream_data_read()
{
  return nx__uart_data_read();
}



void nx_bt_stream_close()
{
  USB_SEND("stream_close()");

  /* we put back the callback in place: */
  nx__uart_set_callback(bt_uart_command_callback);

  /* return in command mode by lowering the ARM_CMD pin */
  *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
  bt_state.state = BT_STATE_WAITING;
}



/* to remove: */
void nx_bt_debug()
{
  //nx_display_uint(bt_state.last_heartbeat);
  //nx_display_end_line();
  USB_SEND((char *)bt_state.cmds);
}


