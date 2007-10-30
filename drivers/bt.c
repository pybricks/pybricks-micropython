#include "base/at91sam7s256.h"

#include "base/mytypes.h"

#include "base/drivers/uart.h"
#include "base/drivers/bt.h"


#ifdef UART_DEBUG
#include "base/drivers/usb.h"
#include "base/util.h"
#define USB_SEND(txt) usb_send((U8*)txt, strlen(txt))
#else
#define USB_SEND(txt)
#endif

#define BT_BUFFER_SIZE 4


static const U8 bt_start_heart[] = {
  0x03, /* length */
  0x0C, /* start heart */
  0xFF, /* sum (high) */
  0xF1  /* sum (low) */
};



static volatile struct {
  U32 state; /* not used atm */

  U8 buf_a[BT_BUFFER_SIZE];
  U8 buf_b[BT_BUFFER_SIZE];

} bt_state = {
  0
};



static void bt_uart_callback(U8 *buffer)
{
  USB_SEND("bt_uart_callback()");

  USB_SEND("bt_uart_callback() finished");
}


void bt_init()
{
  USB_SEND("bt_init()");

  uart_init(bt_uart_callback,
            &bt_state.buf_a, BT_BUFFER_SIZE,
            &bt_state.buf_b, BT_BUFFER_SIZE);

  uart_write(&bt_start_heart, sizeof(bt_start_heart));

  USB_SEND("bt_init() finished");
}
