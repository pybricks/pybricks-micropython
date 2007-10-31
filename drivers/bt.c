#include "base/at91sam7s256.h"

#include "base/mytypes.h"

#include "base/drivers/systick.h"
#include "base/drivers/uart.h"
#include "base/drivers/bt.h"


#ifdef UART_DEBUG
#include "base/drivers/usb.h"
#include "base/util.h"
#define USB_SEND(txt) usb_send((U8*)txt, strlen(txt))
#else
#define USB_SEND(txt)
#endif

#define BT_RST_PIN AT91C_PIO_PA11
#define BT_ARM_CMD_PIN AT91C_PIO_PA27
/* BT_BC4_CMD is connected to the channel 4 of the Analog to Digital converter */
#define BT_CS_PIN   AT91C_PIO_PA31


/* to remove: */
#define BT_CMD_BUF 128



static const U8 bt_msg_start_heart[] = {
  0x03, /* length */
  0x0C, /* start heart */
  0xFF, /* checksum (high) */
  0xF4  /* checksum (low) */
};


static const U8 bt_msg_get_version[] = {
  0x03, /* length */
  0x2F, /* get version */
  0xFF, /* checksum (hi) */
  0xD1  /* checksum (lo) */
};


static const U8 bt_set_discoverable_true[] = {
  0x04, /* length */
  0x1C, /* set discoverable */
  0x01, /* => true */
  0xFF,
  0xE3
};

static const U8 bt_set_discoverable_false[] = {
  0x04, /* length */
  0x1C, /* set discoverable */
  0x00, /* => true */
  0xFF,
  0xE4
};


static volatile struct {
  U32 state; /* not used atm */

  /* to remove */
  U16 cmd_pos;
  U8 cmds[BT_CMD_BUF];

} bt_state = {
  0
};




/* len => checksum included
 */
static U16 bt_get_checksum(U8 *msg, U8 len, bool count_len)
{
  U8 i;
  U16 checksum;

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
void bt_set_checksum(U8 *msg, U8 len) {
  U16 checksum = bt_get_checksum(msg, len, FALSE);

  msg[len] = ((checksum >> 8) & 0xFF);
  msg[len+1] = checksum & 0xFF;
}


/* len => length excepted, but checksum included */
static bool bt_check_checksum(U8 *msg, U8 len) {

  /* Strangess: Must include the packet length in the checksum ?! */
  /* TODO : Figure this out */
  U16 checksum = bt_get_checksum(msg, len, TRUE);
  U8 hi, lo;

  hi = ((checksum >> 8) & 0xFF);
  lo = checksum & 0xFF;

  return (hi == msg[len-2] && lo == msg[len-1]);
}



static void bt_uart_callback(U8 *msg, U8 len)
{
  /* we check first the checksum and ignore the message if the checksum is invalid */
  if (!bt_check_checksum(msg, len))
    return;

  /* to remove: */
  if (bt_state.cmd_pos < BT_CMD_BUF) {
    bt_state.cmds[bt_state.cmd_pos] = msg[0];
    bt_state.cmd_pos++;
  }
}



void bt_init()
{
  USB_SEND("bt_init()");

  /* we put the ARM CMD pin to 0 => command mode */
  /* and we put the RST PIN to 0 => Reseting */
  *AT91C_PIOA_PER = BT_RST_PIN | BT_ARM_CMD_PIN;
  *AT91C_PIOA_PPUDR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_CODR = BT_RST_PIN;
  *AT91C_PIOA_OER = BT_RST_PIN | BT_ARM_CMD_PIN;

  uart_init(bt_uart_callback);

  /* we release the reset pin => 1 */
  *AT91C_PIOA_SODR = BT_RST_PIN;

  systick_wait_ms(1000);

  uart_write(&bt_msg_start_heart, sizeof(bt_msg_start_heart));

  USB_SEND("bt_init() finished");
}



void bt_set_friendly_name(char *name)
{
  int i;
  U8 packet[20] = { 0 };

  packet[0] = 19; /* length */
  packet[1] = 0x21; /* set friendly name */

  for (i = 0 ; i < 16 && name[i] != '\0' ; i++)
    packet[i+2] = name[i];

  bt_set_checksum(packet, 19);

  uart_write(packet, 19);

  while(uart_is_writing()); /* to avoid some stack issues */
}


void bt_set_discoverable(bool d)
{
  if (d)
    uart_write(&bt_set_discoverable_true, sizeof(bt_set_discoverable_true));
  else
    uart_write(&bt_set_discoverable_false, sizeof(bt_set_discoverable_false));
}


/* TO REMOVE : */

void bt_debug()
{
  USB_SEND("===");
  USB_SEND((char *)bt_state.cmds);
}


