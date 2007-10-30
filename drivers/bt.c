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
  0xFF, /* sum (high) */
  0xF4  /* sum (low) */
};


static const U8 bt_msg_get_version[] = {
  0x03, /* length */
  0x2F, /* get version */
  0xFF,
  0xD1
};


static volatile struct {
  U32 state; /* not used atm */

  /* to remove */
  U16 cmd_pos;
  U8 cmds[BT_CMD_BUF];

} bt_state = {
  0
};



static void bt_uart_callback(U8 *buffer, U8 packet_size)
{
  /* to remove: */
  if (bt_state.cmd_pos < BT_CMD_BUF) {
    bt_state.cmds[bt_state.cmd_pos] = buffer[0];
    bt_state.cmd_pos++;
  }
}


void bt_init()
{
  USB_SEND("bt_init()");

  /* we put the ARM CMD pin to 0 */
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

  uart_write(&bt_msg_get_version, sizeof(bt_msg_get_version));
  uart_write(&bt_msg_start_heart, sizeof(bt_msg_start_heart));

  USB_SEND("bt_init() finished");
}


/* TO REMOVE : */

void bt_debug()
{
  USB_SEND("===");
  USB_SEND((char *)bt_state.cmds);
}


