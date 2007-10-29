#include "base/at91sam7s256.h"

#include "base/drivers/uart.h"
#include "base/drivers/bt.h"


#ifdef UART_DEBUG
#include "base/drivers/usb.h"
#include "base/util.h"

#define USB_SEND(txt) usb_send((U8*)txt, strlen(txt))

#else

#define USB_SEND(txt)

#endif


static const U8 bt_start_heart[] = {
  0x03, /* length */
  0x0C, /* start heart */
  0x00,
  0x00
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
            NULL, 0,
            NULL, 0);
  USB_SEND("bt_init() finished");
}
