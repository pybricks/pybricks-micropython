// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// USB serial port driver (CDC/ACM) for STM32 MCUs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4_CDC

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include <contiki.h>
#include <contiki-lib.h>

#include <usbd_cdc.h>
#include <usbd_core.h>

PROCESS(pbdrv_usb_process, "USB");

static uint8_t usb_in_buf[CDC_DATA_FS_MAX_PACKET_SIZE];
static uint8_t usb_out_buf[CDC_DATA_FS_MAX_PACKET_SIZE - 1];
static volatile bool usb_in_busy;
static volatile bool usb_out_busy;

static volatile bool usb_connected;

// size must be power of 2 for ringbuf! also can't be > 255!
static uint8_t stdout_data[128];
static uint8_t stdin_data[128];
static struct ringbuf stdout_buf;
static struct ringbuf stdin_buf;

static USBD_CDC_LineCodingTypeDef LineCoding = {
    .bitrate = 115200,
    .format = CDC_STOP_BITS_1,
    .paritytype = CDC_PARITY_NONE,
    .datatype = 8,
};

static USBD_HandleTypeDef USBD_Device;
extern USBD_DescriptorsTypeDef VCP_Desc;

/**
  * @brief  CDC_Itf_Init
  *         Initializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Init(void) {
    ringbuf_init(&stdin_buf, stdin_data, (uint8_t)PBIO_ARRAY_SIZE(stdin_data));
    ringbuf_init(&stdout_buf, stdout_data, (uint8_t)PBIO_ARRAY_SIZE(stdout_data));
    USBD_CDC_SetTxBuffer(&USBD_Device, usb_out_buf, 0);
    USBD_CDC_SetRxBuffer(&USBD_Device, usb_in_buf);
    usb_in_busy = false;
    usb_out_busy = false;
    usb_connected = false;

    return USBD_OK;
}

/**
  * @brief  CDC_Itf_DeInit
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_DeInit(void) {
    usb_connected = false;
    return USBD_OK;
}

/**
  * @brief  CDC_Itf_Control
  *         Manage the CDC class requests
  * @param  Cmd: Command code
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
    switch (cmd) {
        case CDC_SEND_ENCAPSULATED_COMMAND:
            break;

        case CDC_GET_ENCAPSULATED_RESPONSE:
            break;

        case CDC_SET_COMM_FEATURE:
            break;

        case CDC_GET_COMM_FEATURE:
            break;

        case CDC_CLEAR_COMM_FEATURE:
            break;

        case CDC_SET_LINE_CODING:
            LineCoding.bitrate = pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24);
            LineCoding.format = pbuf[4];
            LineCoding.paritytype = pbuf[5];
            LineCoding.datatype = pbuf[6];
            break;

        case CDC_GET_LINE_CODING:
            pbuf[0] = (uint8_t)(LineCoding.bitrate);
            pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
            pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
            pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
            pbuf[4] = LineCoding.format;
            pbuf[5] = LineCoding.paritytype;
            pbuf[6] = LineCoding.datatype;
            break;

        case CDC_SET_CONTROL_LINE_STATE: {
            USBD_SetupReqTypedef *req = (void *)pbuf;
            // REVISIT: MicroPython defers the connection state here to allow
            // some time to disable local echo on the remote terminal
            usb_connected = !!(req->wValue & CDC_CONTROL_LINE_DTR);
            break;
        }
        case CDC_SEND_BREAK:
            break;
    }

    return USBD_OK;
}

/**
  * @brief  CDC_Itf_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  * @param  Buf: Buffer of data to be transmitted
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Receive(uint8_t *Buf, uint32_t *Len) {
    for (int i = 0; i < *Len; i++) {
        ringbuf_put(&stdin_buf, Buf[i]);
    }
    usb_in_busy = false;
    // process_poll(&pbdrv_usb_process);
    return USBD_OK;
}

/**
  * @brief  CDC_Itf_TransmitCplt
  *         Data transmitted callback
  *
  * @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_TransmitCplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
    usb_out_busy = false;
    // process_poll(&pbdrv_usb_process);
    return USBD_OK;
}

static USBD_CDC_ItfTypeDef USBD_CDC_fops = {
    .Init = CDC_Itf_Init,
    .DeInit = CDC_Itf_DeInit,
    .Control = CDC_Itf_Control,
    .Receive = CDC_Itf_Receive,
    .TransmitCplt = CDC_Itf_TransmitCplt,
};

static void pbdrv_stm32_usb_serial_init() {
    USBD_Init(&USBD_Device, &VCP_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
    USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
    USBD_Start(&USBD_Device);
}

static void pbdrv_stm32_usb_serial_transmit() {
    static int tx_size = 0;

    if (usb_out_busy) {
        return;
    }

    // If tx_size > 0 it means we have a pending retry, otherwise we get as
    // much as we can from the stdout buffer.
    if (tx_size == 0) {
        tx_size = ringbuf_elements(&stdout_buf);
        if (tx_size > PBIO_ARRAY_SIZE(usb_out_buf)) {
            tx_size = PBIO_ARRAY_SIZE(usb_out_buf);
        }
        if (tx_size > 0) {
            for (int i = 0; i < tx_size; i++) {
                usb_out_buf[i] = ringbuf_get(&stdout_buf);
            }
        }
    }

    if (tx_size > 0) {
        USBD_CDC_SetTxBuffer(&USBD_Device, usb_out_buf, tx_size);
        if (USBD_CDC_TransmitPacket(&USBD_Device) == USBD_OK) {
            usb_out_busy = true;
            tx_size = 0;
        }
    }
}

static void pbdrv_stm32_usb_serial_receive() {
    if (usb_in_busy) {
        return;
    }

    if (USBD_CDC_ReceivePacket(&USBD_Device) == USBD_OK) {
        usb_in_busy = true;
    }
}

pbio_error_t pbsys_stdout_put_char(uint8_t c) {
    if (!usb_connected) {
        // don't lock up print() when USB not connected - data is discarded
        return PBIO_SUCCESS;
    }
    if (ringbuf_put(&stdout_buf, c) == 0) {
        return PBIO_ERROR_AGAIN;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbsys_stdin_get_char(uint8_t *c) {
    if (ringbuf_elements(&stdin_buf) == 0) {
        return PBIO_ERROR_AGAIN;
    }
    *c = ringbuf_get(&stdin_buf);
    return PBIO_SUCCESS;
}

PROCESS_THREAD(pbdrv_usb_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    pbdrv_stm32_usb_serial_init();
    etimer_set(&timer, clock_from_msec(5));

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL || (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)));
        etimer_reset(&timer);
        pbdrv_stm32_usb_serial_transmit();
        pbdrv_stm32_usb_serial_receive();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_STM32F4_CDC
