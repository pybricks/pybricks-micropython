/* Driver for the NXT's USB port.
 *
 * This driver drives the onboard USB controller to make the NXT into
 * a functional USB 2.0 peripheral. Note that given the limitations of
 * the controller hardware, the brick cannot function as a host, only
 * as a peripheral.
 */

#include "base/at91sam7s256.h"

#include "base/mytypes.h"
#include "base/interrupts.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/util.h"
#include "base/drivers/usb.h"

/* The USB controller supports up to 4 endpoints. */
#define N_ENDPOINTS 4

/* Maximum data packet sizes. Endpoint 0 is a special case (control
 * endpoint).
 *
 * TODO: Discuss the need/use for separating recv/send.
 */
#define MAX_EP0_SIZE 8
#define MAX_RCV_SIZE 64
#define MAX_SND_SIZE 64


/* Various constants for the setup packets.
 *
 * TODO: clean up these. Most are unused.
 */
#define USB_BMREQUEST_DIR            0x80
#define USB_BMREQUEST_H_TO_D         0x0
#define USB_BMREQUEST_D_TO_H         0x80

#define USB_BMREQUEST_RCPT           0x0F
#define USB_BMREQUEST_RCPT_DEV       0x0 /* device */
#define USB_BMREQUEST_RCPT_INT       0x1 /* interface */
#define USB_BMREQUEST_RCPT_EPT       0x2 /* endpoint */
#define USB_BMREQUEST_RCPT_OTH       0x3 /* other */

#define USB_BREQUEST_GET_STATUS      0x0
#define USB_BREQUEST_CLEAR_FEATURE   0x1
#define USB_BREQUEST_SET_FEATURE     0x3
#define USB_BREQUEST_SET_ADDRESS     0x5
#define USB_BREQUEST_GET_DESCRIPTOR  0x6
#define USB_BREQUEST_SET_DESCRIPTOR  0x7
#define USB_BREQUEST_GET_CONFIG      0x8
#define USB_BREQUEST_SET_CONFIG      0x9
#define USB_BREQUEST_GET_INTERFACE   0xA
#define USB_BREQUEST_SET_INTERFACE   0xB

#define USB_WVALUE_TYPE        (0xFF << 8)
#define USB_DESC_TYPE_DEVICE           1
#define USB_DESC_TYPE_CONFIG           2
#define USB_DESC_TYPE_STR              3
#define USB_DESC_TYPE_INT              4
#define USB_DESC_TYPE_ENDPT            5
#define USB_DESC_TYPE_DEVICE_QUALIFIER 6

#define USB_WVALUE_INDEX       0xFF


/* The following definitions are 'raw' USB setup packets. They are all
 * standard responses to various setup requests by the USB host. These
 * packets are all constant, and mostly boilerplate. Don't be too
 * bothered if you skip over these to real code.
 *
 * If you want to understand the full meaning of every bit of these
 * packets, you should refer to the USB 2.0 specifications.
 *
 * One point of interest: the USB device space is partitionned by
 * vendor and product ID. As we are lacking money and real need, we
 * don't have a vendor ID to use. Therefore, we are currently
 * piggybacking on Lego's device space, using an unused product ID.
 */
static const U8 usb_device_descriptor[] = {
  18, USB_DESC_TYPE_DEVICE, /* Packet size and type. */
  0x00, 0x20, /* This packet is USB 2.0. */
  2, /* Class code. */
  0, /* Sub class code. */
  0, /* Device protocol. */
  MAX_EP0_SIZE, /* Maximum packet size for EP0 (control endpoint). */
  0x94, 0x06, /* Vendor ID : LEGO */
  0x00, 0xFF, /* Product ID : NXOS */
  0x00, 0x00, /* Product revision. */
  1, /* Index of the vendor string. */
  2, /* Index of the product string. */
  0, /* Index of the serial number (none for us). */
  1, /* The number of possible configurations. */
};

static const U8 usb_dev_qualifier_desc[] = {
  10, USB_DESC_TYPE_DEVICE_QUALIFIER, /* Packet size and type. */
  0x00, 0x20, /* This packet is USB 2.0. */
  2, /* Class code */
  0, /* Sub class code */
  0, /* Device protocol */
  MAX_EP0_SIZE, /* Maximum packet size for EP0. */
  1, /* The number of possible configurations. */
  0 /* Reserved for future use, must be zero. */
};


static const U8 usb_nxos_full_config[] = {
  0x09, USB_DESC_TYPE_CONFIG, /* Descriptor size and type. */
  0x20, 0x00, /* Total length of the configuration, interface
               * description included.
               */
  1, /* The number of interfaces declared by this configuration. */
  1, /* The ID for this configuration. */
  0, /* Index of the configuration description string (none). */

  /* Configuration attributes bitmap. Bit 7 (MSB) must be 1, bit 6 is
   * 1 because the NXT is self-powered, bit 5 is 0 because the NXT
   * doesn't support remote wakeup, and bits 0-4 are 0 (reserved).
   */
  0x40,
  0, /* Device power consumption, for non self-powered devices. */


  /*
   * This is the descriptor for the interface associated with the
   * configuration.
   */
  0x09, USB_DESC_TYPE_INT, /* Descriptor size and type. */
  0x00, /* Interface index. */
  0x00, /* ID for this interface configuration. */
  0x02, /* The number of endpoints defined by this interface
         * (excluding EP0).
         */
  0xFF, /* Interface class ("Vendor specific"). */
  0xFF, /* Interface subclass (see above). */
  0xFF, /* Interface protocol (see above). */
  0x00, /* Index of the string descriptor for this interface (none). */


  /*
   * Descriptor for EP1.
   */
  7, USB_DESC_TYPE_ENDPT, /* Descriptor length and type. */
  0x1, /* Endpoint number. MSB is zero, meaning this is an OUT EP. */
  0x2, /* Endpoint type (bulk). */
  MAX_RCV_SIZE, 0x00, /* Maximum packet size (64). */
  0, /* EP maximum NAK rate (device never NAKs). */


  /*
   * Descriptor for EP2.
   */
  7, USB_DESC_TYPE_ENDPT, /* Descriptor length and type. */
  0x82, /* Endpoint number. MSB is one, meaning this is an IN EP. */
  0x2, /* Endpoint type (bulk). */
  MAX_RCV_SIZE, 0x00, /* Maximum packet size (64). */
  0, /* EP maximum NAK rate (device never NAKs). */
};


static const U8 usb_string_desc[] = {
  4, USB_DESC_TYPE_STR, /* Descriptor length and type. */
  0x09, 0x04, /* Supported language ID (US English). */
};

static const U8 usb_lego_str[] = {
  10, USB_DESC_TYPE_STR,
  'L', 0,
  'E', 0,
  'G', 0,
  'O', 0
};

static const U8 usb_nxt_str[] = {
  10, USB_DESC_TYPE_STR,
  'N', 0,
  'x', 0,
  'O', 0,
  'S', 0,
};


/* Internal lookup table mapping string descriptors to their indices
 * in the USB string descriptor table.
 */
static const U8 *usb_strings[] = {
  usb_lego_str,
  usb_nxt_str,
};


/*
 * The USB device state. Contains the current USB state (selected
 * configuration, etc.) and transitory state for data transfers.
 */
static volatile struct {
  /* The current state of the device. */
  enum usb_status {
    USB_UNINITIALIZED = 0,
    USB_READY,
    USB_BUSY,
    USB_SUSPENDED,
  } status;

  /* Holds the status the bus was in before entering suspend. */
  enum usb_status pre_suspend_status;

  /* When the host gives us an address, we must send a null ACK packet
   * back before actually changing addresses. This field stores the
   * address that should be set once the ACK is sent.
   */
  U32 new_device_address;

  /* The currently selected USB configuration. */
  U8 current_config;

  /* Holds the state of the data transmissions on both EP0 and
   * EP1. This only gets used if the transmission needed to be split
   * into several USB packets.
   */
  U8 *tx_data[N_ENDPOINTS]; /* TODO: Switch to 2, memory waste. */
  U32 tx_len[N_ENDPOINTS];

  /* Holds received data shifted from the controller. Receiving is
   * double-buffered, and the reader must flush the current buffer to
   * gain access to the other buffer.
   */
  U8   rx_current_user_buffer_idx; /* 0 or 1 */
  U8   rx_buffer[2][USB_BUFFER_SIZE];
  U16  rx_buffer_size[2]; /* data size waiting in the buffer */
  bool rx_overflow;

  /* The USB controller has two hardware input buffers. This remembers
   * the one currently in use.
   */
  U8 current_rx_bank;
} usb_state = {
  0
};


/* The flags in the UDP_CSR register are a little strange: writing to
 * them does not instantly change their value. Their value will change
 * to reflect the write when the USB controller has taken the change
 * into account. The driver must wait until the controller
 * acknowledges changes to CSR.
 *
 * These helpers set/clear CSR flags, and then loop waiting for the
 * controller to synchronize
 */
static inline void usb_csr_clear_flag(U8 endpoint, U32 flags) {
  AT91C_UDP_CSR[endpoint] &= ~(flags);
  while (AT91C_UDP_CSR[endpoint] & (flags));
}

static inline void usb_csr_set_flag(U8 endpoint, U32 flags) {
  AT91C_UDP_CSR[endpoint] |= (flags);
  while ( (AT91C_UDP_CSR[endpoint] & (flags)) != (flags));
}


/* Starts sending data to the host. If the data cannot fit into a
 * single USB packet, the data is split and scheduled to be sent in
 * several packets.
 */
static void usb_send_data(int endpoint, const U8 *ptr, U32 length) {
  U32 packet_size;

  /* The bus is now busy. */
  usb_state.status = USB_BUSY;

  if (endpoint == 0)
    packet_size = MIN(MAX_EP0_SIZE, length);
  else
    packet_size = MIN(MAX_SND_SIZE, length);

  /* If there is more data than can fit in a single packet, queue the
   * rest up.
   */
  if (length > packet_size) {
    length -= packet_size;
    usb_state.tx_data[endpoint] = (U8*)(ptr + packet_size);
    usb_state.tx_len[endpoint] = length;
  } else {
    usb_state.tx_data[endpoint] = NULL;
    usb_state.tx_len[endpoint] = 0;
  }

  /* Push a packet into the USB FIFO, and tell the controller to send. */
  while(packet_size) {
    AT91C_UDP_FDR[endpoint] = *ptr;
    ptr++;
    packet_size--;
  }
  usb_csr_set_flag(endpoint, AT91C_UDP_TXPKTRDY);
}


/* Read one data packet from the USB controller. */
static void usb_read_data(int endpoint) {
  U8 buf;
  U16 i;
  U16 total;

  /* Given our configuration, we should only be getting packets on
   * endpoint 1. Ignore data on any other endpoint.
   */
  if (endpoint != 1) {
    usb_csr_clear_flag(endpoint, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
    return;
  }

  total = (AT91C_UDP_CSR[endpoint] & AT91C_UDP_RXBYTECNT) >> 16;

  /* if the buffer currently used by the user program is empty, then
   * we will write in this one, else we will write in the other one */

  if (usb_state.rx_buffer_size[usb_state.rx_current_user_buffer_idx] == 0) {
    buf = usb_state.rx_current_user_buffer_idx;
  } else {
    buf = (usb_state.rx_current_user_buffer_idx == 0 ? 1 : 0);
  }

  /* if we are writing in a buffer containing something,
   * it means we are overloaded */
  if (usb_state.rx_buffer_size[buf] > 0) {
    usb_state.rx_overflow = TRUE;
  }

  usb_state.rx_buffer_size[buf] = total;
  for (i = 0 ; i < total; i++)
    usb_state.rx_buffer[buf][i] = AT91C_UDP_FDR[1];


  /* Acknowledge reading the current RX bank, and switch to the other. */
  usb_csr_clear_flag(1, usb_state.current_rx_bank);
  if (usb_state.current_rx_bank == AT91C_UDP_RX_DATA_BK0)
    usb_state.current_rx_bank = AT91C_UDP_RX_DATA_BK1;
  else
    usb_state.current_rx_bank = AT91C_UDP_RX_DATA_BK0;
}


/* A stall is USB's way of sending back an error (either "not
 * understood" or "not handled by this device").
 * The connexion will be reinitialized by the host.
 */
static void usb_send_stall() {
  usb_state.status = USB_UNINITIALIZED;
  usb_csr_set_flag(0, AT91C_UDP_FORCESTALL);
}


/* During setup, we need to send packets with null data. */
static void usb_send_null() {
  usb_send_data(0, NULL, 0);
}


/* Handle receiving and responding to setup packets on EP0. */
static U32 usb_manage_setup_packet() {
  /* The structure of a USB setup packet. */
  struct {
    U8 request_attrs; /* Request characteristics. */
    U8 request; /* Request type. */
    U16 value; /* Request-specific value. */
    U16 index; /* Request-specific index. */
    U16 length; /* The number of bytes transferred in the (optional)
                 * second phase of the control transfer. */
  } packet;
  U16 response;
  U32 size;
  U8 index;

  /* Read the packet from the FIFO into the above packet struct. */
  packet.request_attrs = AT91C_UDP_FDR[0];
  packet.request       = AT91C_UDP_FDR[0];
  packet.value         = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);
  packet.index         = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);
  packet.length        = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);


  if ((packet.request_attrs & USB_BMREQUEST_DIR) == USB_BMREQUEST_D_TO_H) {
    usb_csr_set_flag(0, AT91C_UDP_DIR); /* TODO: contradicts atmel doc p475 */
  }

  usb_csr_clear_flag(0, AT91C_UDP_RXSETUP);


  response = 0;


  /* Respond to the control request. */
  switch (packet.request) {
  case USB_BREQUEST_GET_STATUS:
    /* The host wants to know our status.
     *
     * If it wants the device status, just reply that the NXT is still
     * self-powered (as first declared by the setup packets). If it
     * wants endpoint status, reply that the endpoint has not
     * halted. Any other status request types are reserved, which
     * translates to replying zero.
     */
    if ((packet.request_attrs & USB_BMREQUEST_RCPT) == USB_BMREQUEST_RCPT_DEV)
      response = 1;
    else
      response = 0;

    usb_send_data(0, (U8*)&response, 2);
    break;

  case USB_BREQUEST_CLEAR_FEATURE:
  case USB_BREQUEST_SET_INTERFACE:
  case USB_BREQUEST_SET_FEATURE:
    /* TODO: Refer back to the specs and send the right
     * replies. This is wrong, even though it happens to not break
     * on linux.
     */
    usb_send_null();
    break;

  case USB_BREQUEST_SET_ADDRESS:
    /* The host has given the NXT a new USB address. This address
     * must be set AFTER sending the ack packet. Therefore, we just
     * remember the new address, and the interrupt handler will set
     * it when the transmission completes.
     */
    usb_state.new_device_address = packet.value;
    usb_send_null();

    /* If the address change is to 0, do it immediately.
     *
     * TODO: Why? And when does this happen?
     */
    if (usb_state.new_device_address == 0) {
      *AT91C_UDP_FADDR = AT91C_UDP_FEN;
      *AT91C_UDP_GLBSTATE = 0;
      }
    break;

  case USB_BREQUEST_GET_DESCRIPTOR:
    /* The host requested a descriptor. */

    index = (packet.value & USB_WVALUE_INDEX);
    switch ((packet.value & USB_WVALUE_TYPE) >> 8) {
    case USB_DESC_TYPE_DEVICE: /* Device descriptor */
      size = usb_device_descriptor[0];
      usb_send_data(0, usb_device_descriptor,
                    MIN(size, packet.length));
      break;

    case USB_DESC_TYPE_CONFIG: /* Configuration descriptor */
      usb_send_data(0, usb_nxos_full_config,
                    MIN(usb_nxos_full_config[2], packet.length));

      /* TODO: Why? This is not specified in the USB specs. */
      if (usb_nxos_full_config[2] < packet.length)
        usb_send_null();
      break;

    case USB_DESC_TYPE_STR: /* String or language info. */
      if ((packet.value & USB_WVALUE_INDEX) == 0) {
        usb_send_data(0, usb_string_desc,
                      MIN(usb_string_desc[0], packet.length));
      } else {
        /* The host wants a specific string. */
        /* TODO: This should check if the requested string exists. */
        usb_send_data(0, usb_strings[index-1],
                      MIN(usb_strings[index-1][0],
                          packet.length));
      }
      break;

    case USB_DESC_TYPE_DEVICE_QUALIFIER: /* Device qualifier descriptor. */
      size = usb_dev_qualifier_desc[0];
      usb_send_data(0, usb_dev_qualifier_desc,
                    MIN(size, packet.length));
      break;

    default: /* Unknown descriptor, tell the host by stalling. */
      usb_send_stall();
    }
    break;

    case USB_BREQUEST_GET_CONFIG:
      /* The host wants to know the ID of the current configuration. */
      usb_send_data(0, (U8 *)&(usb_state.current_config), 1);
      break;

    case USB_BREQUEST_SET_CONFIG:
      /* The host selected a new configuration. */
      usb_state.current_config = packet.value;

      /* we ack */
      usb_send_null();

      /* we set the register in configured mode */
      *AT91C_UDP_GLBSTATE = packet.value > 0 ?
	(AT91C_UDP_CONFG | AT91C_UDP_FADDEN)
	:AT91C_UDP_FADDEN;

      /* TODO: Make this a little nicer. Not quite sure how. */
      AT91C_UDP_CSR[1] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
      while (AT91C_UDP_CSR[1] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT));
      AT91C_UDP_CSR[2] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN;
      while (AT91C_UDP_CSR[2] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN));
      AT91C_UDP_CSR[3] = 0;
      while (AT91C_UDP_CSR[3] != 0);

      usb_state.status = USB_READY;
      break;

  case USB_BREQUEST_GET_INTERFACE: /* TODO: This should respond, not stall. */
  case USB_BREQUEST_SET_DESCRIPTOR:
  default:
    usb_send_stall();
    break;
  }

  return packet.request;
}


/* The main USB interrupt handler. */
static void usb_isr() {
  U8 endpoint = 127;
  U32 csr, isr;

  isr = *AT91C_UDP_ISR;

  /* We sent a stall, the host has acknowledged the stall. */
  if (AT91C_UDP_CSR[0] & AT91C_UDP_ISOERROR)
    usb_csr_clear_flag(0, AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR);

  /* End of bus reset. Starting the device setup procedure. */
  if (isr & AT91C_UDP_ENDBUSRES) {
    usb_state.status = USB_UNINITIALIZED;

    /* Disable and clear all interruptions, reverting to the base
     * state.
     */
    *AT91C_UDP_IDR = ~0;
    *AT91C_UDP_ICR = ~0;

    /* Reset all endpoint FIFOs. */
    *AT91C_UDP_RSTEP = ~0;
    *AT91C_UDP_RSTEP = 0;

    /* Reset internal state. */
    usb_state.current_rx_bank = AT91C_UDP_RX_DATA_BK0;
    usb_state.current_config  = 0;

    /* Reset EP0 to a basic control endpoint. */
    /* TODO: The while is ugly. Fix it. */
    AT91C_UDP_CSR[0] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL;
    while (AT91C_UDP_CSR[0] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL));

    /* Enable interrupt handling for all three endpoints, as well as
     * suspend/resume.
     */
    *AT91C_UDP_IER = (AT91C_UDP_EPINT0 | AT91C_UDP_EPINT1 |
                      AT91C_UDP_EPINT2 | AT91C_UDP_EPINT3 |
                      AT91C_UDP_RXSUSP | AT91C_UDP_RXRSM);

    /* Enable the function endpoints, setting address 0, and return
     * immediately. Given that we've just reset everything, there's no
     * point in continuing.
     */
    *AT91C_UDP_FADDR = AT91C_UDP_FEN;

    return;
  }

  if (isr & AT91C_UDP_WAKEUP) {
    *AT91C_UDP_ICR = AT91C_UDP_WAKEUP;
    isr &= ~AT91C_UDP_WAKEUP;
  }


  if (isr & AT91C_UDP_SOFINT) {
    *AT91C_UDP_ICR = AT91C_UDP_SOFINT;
    isr &= ~AT91C_UDP_SOFINT;
  }


  if (isr & AT91C_UDP_RXSUSP) {
    *AT91C_UDP_ICR = AT91C_UDP_RXSUSP;
    isr &= ~AT91C_UDP_RXSUSP;
    usb_state.pre_suspend_status = usb_state.status;
    usb_state.status = USB_SUSPENDED;
  }

  if (isr & AT91C_UDP_RXRSM) {
    *AT91C_UDP_ICR = AT91C_UDP_RXRSM;
    isr &= ~AT91C_UDP_RXRSM;
    usb_state.status = usb_state.pre_suspend_status;
  }



  for (endpoint = 0; endpoint < N_ENDPOINTS ; endpoint++) {
    if (isr & (1 << endpoint))
      break;
  }


  if (endpoint == 0) {

    if (AT91C_UDP_CSR[0] & AT91C_UDP_RXSETUP) {
      csr = usb_manage_setup_packet();
      return;
    }
  }


  if (endpoint < N_ENDPOINTS) { /* if an endpoint was specified */
    csr = AT91C_UDP_CSR[endpoint];

    if (csr & AT91C_UDP_RX_DATA_BK0
	|| csr & AT91C_UDP_RX_DATA_BK1) {
      usb_read_data(endpoint);
      return;
    }

    if (csr & AT91C_UDP_TXCOMP) {

      /* then it means that we sent a data and the host has acknowledged it */
      usb_state.status = USB_READY; /* TODO: check for race with
                                       usb_send_data further down. */
      /* so first we will reset this flag */
      usb_csr_clear_flag(endpoint, AT91C_UDP_TXCOMP);

      if (usb_state.new_device_address > 0) {
	/* the previous message received was SET_ADDR */
	/* now that the computer ACK our send_null(), we can
	 * set this address for real */

	/* we set the specified usb address in the controller */
	*AT91C_UDP_FADDR    = AT91C_UDP_FEN | usb_state.new_device_address;
	/* and we tell the controller that we are in addressed mode now */
	*AT91C_UDP_GLBSTATE = AT91C_UDP_FADDEN;
	usb_state.new_device_address = 0;
      }


      /* and we will send the following data */
      if (usb_state.tx_len[endpoint] > 0
	  && usb_state.tx_data[endpoint] != NULL) {
	usb_send_data(endpoint, usb_state.tx_data[endpoint],
		      usb_state.tx_len[endpoint]);
      }
      return;
    }

  }


  /* We clear also the unused bits,
   * just "to be sure" */
  if (isr) {
    *AT91C_UDP_ICR = 0xFFFFC4F0;
  }
}






void usb_disable() {
  aic_disable(AT91C_ID_UDP);

  *AT91C_PIOA_PER = (1 << 16);
  *AT91C_PIOA_OER = (1 << 16);
  *AT91C_PIOA_SODR = (1 << 16);
  systick_wait_ms(200);
}


static inline void usb_enable() {
  /* Enable the UDP pull up by outputting a zero on PA.16 */
  /* Enabling the pull up will tell to the host (the computer) that
   * we are ready for a communication
   */
  *AT91C_PIOA_PER = (1 << 16);
  *AT91C_PIOA_OER = (1 << 16);
  *AT91C_PIOA_CODR = (1 << 16);
  systick_wait_ms(200);
}


void usb_init() {

  usb_disable();

  interrupts_disable();



  /* usb pll was already set in init.S */

  /* enable peripheral clock */
  *AT91C_PMC_PCER = (1 << AT91C_ID_UDP);

  /* enable system clock */
  *AT91C_PMC_SCER = AT91C_PMC_UDP;

  /* disable all the interruptions */
  *AT91C_UDP_IDR = ~0;

  /* reset all the endpoints */
  *AT91C_UDP_RSTEP = 0xF;
  *AT91C_UDP_RSTEP = 0;

  *AT91C_UDP_ICR = 0xFFFFFFFF;

  /* Install the interruption routine */

  /* the first interruption we will get is an ENDBUSRES
   * this interruption is always emit (can't be disable with UDP_IER)
   */
  /* other interruptions will be enabled when needed */
  aic_install_isr(AT91C_ID_UDP, AIC_PRIO_DRIVER,
		  AIC_TRIG_LEVEL, usb_isr);


  interrupts_enable();

  usb_enable();
}


bool usb_can_send() {
  return (usb_state.status == USB_READY);
}


void usb_send(U8 *data, U32 length) {
  if (usb_state.status == USB_UNINITIALIZED
      || usb_state.status == USB_SUSPENDED)
    return;

  while (usb_state.status != USB_READY);

  /* start sending the data */
  usb_send_data(2, data, length);
}

bool usb_is_connected() {
  return (usb_state.status != USB_UNINITIALIZED);
}


U16 usb_has_data() {
  return usb_state.rx_buffer_size[usb_state.rx_current_user_buffer_idx];
}


volatile void *usb_get_buffer() {
  return (usb_state.rx_buffer[usb_state.rx_current_user_buffer_idx]);
}


bool usb_overloaded() {
  return usb_state.rx_overflow;
}

void usb_flush_buffer() {
  usb_state.rx_overflow = FALSE;

  usb_state.rx_buffer_size[usb_state.rx_current_user_buffer_idx] = 0;

  usb_state.rx_current_user_buffer_idx =
    (usb_state.rx_current_user_buffer_idx == 0) ? 1 : 0;
}



