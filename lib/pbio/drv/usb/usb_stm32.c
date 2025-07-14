// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2025 The Pybricks Authors

// Main file for STM32F4 USB driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4

#include <string.h>
#include <stdbool.h>

#include <contiki.h>
#include <lwrb/lwrb.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_pcd_ex.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_pybricks.h>

#include <pbdrv/usb.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbsys/command.h>
#include <pbsys/status.h>

#include "../charger/charger.h"
#include "./usb_stm32.h"

PROCESS(pbdrv_usb_process, "USB");

// These buffers need to be 32-bit aligned because the USB driver moves data
// to/from FIFOs in 32-bit chunks.
static uint8_t usb_in_buf[USBD_PYBRICKS_MAX_PACKET_SIZE] __aligned(4);
static uint8_t usb_response_buf[1 + sizeof(uint32_t)] __aligned(4);
static uint8_t usb_status_buf[1 + PBSYS_STATUS_REPORT_SIZE] __aligned(4);
static uint8_t usb_stdout_buf[USBD_PYBRICKS_MAX_PACKET_SIZE] __aligned(4);
static volatile uint32_t usb_in_sz;
static volatile uint32_t usb_response_sz;
static volatile uint32_t usb_status_sz;
static volatile uint32_t usb_stdout_sz;
static volatile bool transmitting;
static volatile bool pbdrv_usb_stm32_is_events_subscribed;

static USBD_HandleTypeDef husbd;
static PCD_HandleTypeDef hpcd;

static volatile bool vbus_active;
static pbdrv_usb_bcd_t pbdrv_usb_bcd;

/**
 * Battery charger detection task.
 *
 * This is basically HAL_PCDEx_BCD_VBUSDetect() converted to a protothread.
 *
 * @param [in]  pt  The protothread.
 */
static PT_THREAD(pbdrv_usb_stm32_bcd_detect(struct pt *pt)) {
    static struct etimer timer;
    USB_OTG_GlobalTypeDef *USBx = hpcd.Instance;

    PT_BEGIN(pt);

    // disable all other USB functions
    HAL_PCDEx_ActivateBCD(&hpcd);

    /* Enable DCD : Data Contact Detect */
    USBx->GCCFG |= USB_OTG_GCCFG_DCDEN;

    /* Wait Detect flag or a timeout is happen*/
    etimer_set(&timer, 1000);
    while (!(USBx->GCCFG & USB_OTG_GCCFG_DCDET)) {
        /* Check for the Timeout */
        if (etimer_expired(&timer)) {
            USBx->GCCFG &= ~USB_OTG_GCCFG_DCDEN;
            HAL_PCDEx_DeActivateBCD(&hpcd);
            pbdrv_usb_bcd = PBDRV_USB_BCD_NONSTANDARD;
            PT_EXIT(pt);
        }
        PT_YIELD(pt);
    }

    /* Correct response received */
    etimer_set(&timer, 100);
    PT_WAIT_UNTIL(pt, etimer_expired(&timer));

    /*Primary detection: checks if connected to Standard Downstream Port
    (without charging capability) */
    USBx->GCCFG &= ~USB_OTG_GCCFG_DCDEN;
    USBx->GCCFG |= USB_OTG_GCCFG_PDEN;

    etimer_set(&timer, 100);
    PT_WAIT_UNTIL(pt, etimer_expired(&timer));

    if (!(USBx->GCCFG & USB_OTG_GCCFG_PDET)) {
        /* Case of Standard Downstream Port */
        USBx->GCCFG &= ~USB_OTG_GCCFG_PDEN;
        pbdrv_usb_bcd = PBDRV_USB_BCD_STANDARD_DOWNSTREAM;
    } else {
        /* start secondary detection to check connection to Charging Downstream
        Port or Dedicated Charging Port */
        USBx->GCCFG &= ~USB_OTG_GCCFG_PDEN;
        USBx->GCCFG |= USB_OTG_GCCFG_SDEN;

        etimer_set(&timer, 100);
        PT_WAIT_UNTIL(pt, etimer_expired(&timer));

        if ((USBx->GCCFG) & USB_OTG_GCCFG_SDET) {
            /* case Dedicated Charging Port  */
            pbdrv_usb_bcd = PBDRV_USB_BCD_DEDICATED_CHARGING;
        } else {
            /* case Charging Downstream Port  */
            pbdrv_usb_bcd = PBDRV_USB_BCD_CHARGING_DOWNSTREAM;
        }

        USBx->GCCFG &= ~USB_OTG_GCCFG_SDEN;
    }

    // enable all other USB functions
    HAL_PCDEx_DeActivateBCD(&hpcd);

    PT_END(pt);
}

// Device-specific USB driver implementation.

/**
 * Callback for connecting USB OTG FS interrupt in platform.c.
 */
void pbdrv_usb_stm32_handle_otg_fs_irq(void) {
    HAL_PCD_IRQHandler(&hpcd);
}

/**
 * Callback for connecting VBUS interrupt in platform.c.
 *
 * @param [in]  active  True indicates VBUS is active (5V power is present),
 *                      otherwise false.
 */
void pbdrv_usb_stm32_handle_vbus_irq(bool active) {
    vbus_active = active;
    process_poll(&pbdrv_usb_process);
}

// REVISIT: this needs to be moved to a common place where it can be shared with Bluetooth
static pbdrv_usb_rx_callback_t stdin_event_callback;
static lwrb_t stdin_ring_buf;

/**
 * Gets the number of bytes currently free for writing in stdin.
 * @return              The number of bytes.
 */
uint32_t pbdrv_usb_rx_get_free(void) {
    return lwrb_get_free(&stdin_ring_buf);
}

/**
 * Writes data to the stdin buffer.
 *
 * This does not currently return the number of bytes written, so first call
 * pbdrv_usb_rx_get_free() to ensure enough free space.
 *
 * @param [in]  data    The data to write to the stdin buffer.
 * @param [in]  size    The size of @p data in bytes.
 */
void pbdrv_usb_rx_write(const uint8_t *data, uint32_t size) {
    if (stdin_event_callback) {
        // If there is a callback hook, we have to process things one byte at
        // a time.
        for (uint32_t i = 0; i < size; i++) {
            if (!stdin_event_callback(data[i])) {
                lwrb_write(&stdin_ring_buf, &data[i], 1);
            }
        }
    } else {
        lwrb_write(&stdin_ring_buf, data, size);
    }
}

/**
 * Sets the UART Rx callback function.
 * @param callback  [in]    The callback or NULL.
 */
void pbdrv_usb_rx_set_callback(pbdrv_usb_rx_callback_t callback) {
    stdin_event_callback = callback;
}

/**
 * Gets the number of bytes currently available to be read from the UART Rx
 * characteristic.
 * @return              The number of bytes.
 */
uint32_t pbdrv_usb_rx_get_available(void) {
    return lwrb_get_full(&stdin_ring_buf);
}

/**
 * Reads data from the stdin buffer.
 * @param data  [in]        A buffer to receive a copy of the data.
 * @param size  [in, out]   The number of bytes to read (@p data must be at least
 *                          this big). After return @p size contains the number
 *                          of bytes actually read.
 * @return                  ::PBIO_SUCCESS if @p data was read, ::PBIO_ERROR_AGAIN
 *                          if @p data could not be read at this time (i.e. buffer
 *                          is empty), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbdrv_usb_rx(uint8_t *data, uint32_t *size) {
    if ((*size = lwrb_read(&stdin_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Flushes data from the UART Rx characteristic so that ::pbdrv_usb_rx
 * can be used to wait for new data.
 */
void pbdrv_usb_rx_flush(void) {
    lwrb_reset(&stdin_ring_buf);
}

/**
 * Queues data to be transmitted via USB.
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if some @p data was queued, ::PBIO_ERROR_AGAIN
 *                          if no @p data could not be queued at this time (e.g. buffer
 *                          is full), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active USB connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support USB.
 */
pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    #if PBDRV_CONFIG_USB_CHARGE_ONLY
    return PBIO_ERROR_NOT_IMPLEMENTED;
    #endif

    if (!pbdrv_usb_stm32_is_events_subscribed) {
        // If the app hasn't subscribed to events, we can't send stdout.
        return PBIO_ERROR_INVALID_OP;
    }

    uint8_t *ptr = usb_stdout_buf;
    uint32_t ptr_len = sizeof(usb_stdout_buf);

    if (usb_stdout_sz) {
        return PBIO_ERROR_AGAIN;
    }

    *ptr++ = USBD_PYBRICKS_IN_EP_MSG_EVENT;
    ptr_len--;

    *ptr++ = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
    ptr_len--;

    *size = MIN(*size, ptr_len);
    memcpy(ptr, data, *size);

    usb_stdout_sz = 1 + 1 + *size;

    process_poll(&pbdrv_usb_process);

    return PBIO_SUCCESS;
}

/**
 * Indicates if there is stdout data waiting to be transmitted over USB.
 * @retval  false if stdout data is currently being transmitted.
 */
bool pbdrv_usb_stdout_tx_is_idle(void) {
    return usb_stdout_sz == 0;
}

static void pbdrv_usb_stm32_reset_tx_state(void) {
    usb_response_sz = 0;
    usb_status_sz = 0;
    usb_stdout_sz = 0;
    transmitting = false;
    pbdrv_usb_stm32_is_events_subscribed = false;
}

/**
  * @brief  Pybricks_Itf_Init
  *         Initializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_Init(void) {
    USBD_Pybricks_SetRxBuffer(&husbd, usb_in_buf);
    usb_in_sz = 0;
    pbdrv_usb_stm32_reset_tx_state();

    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_DeInit
  *         DeInitializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_DeInit(void) {
    pbdrv_usb_stm32_reset_tx_state();
    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_DataRx
  *         Data received over USB OUT endpoint are sent over Pybricks interface
  *         through this function.
  * @param  Buf: Buffer of data to be transmitted
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_Receive(uint8_t *Buf, uint32_t Len) {

    usb_in_sz = Len;
    process_poll(&pbdrv_usb_process);
    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_TransmitCplt
  *         Data transmitted callback
  *
  * @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data that was transmitted
  * @param  Len: Number of data transmitted (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_TransmitCplt(uint8_t *Buf, uint32_t Len, uint8_t epnum) {
    USBD_StatusTypeDef ret = USBD_OK;

    if (Buf == usb_response_buf) {
        usb_response_sz = 0;
    } else if (Buf == usb_status_buf) {
        usb_status_sz = 0;
    } else if (Buf == usb_stdout_buf) {
        usb_stdout_sz = 0;
    } else {
        ret = USBD_FAIL;
    }

    transmitting = false;
    process_poll(&pbdrv_usb_process);
    return ret;
}

USBD_Pybricks_ItfTypeDef USBD_Pybricks_fops = {
    .Init = Pybricks_Itf_Init,
    .DeInit = Pybricks_Itf_DeInit,
    .Receive = Pybricks_Itf_Receive,
    .TransmitCplt = Pybricks_Itf_TransmitCplt,
};

// Common USB driver implementation.

void pbdrv_usb_init(void) {
    // Link the driver data structures
    husbd.pData = &hpcd;
    hpcd.pData = &husbd;

    #if PBDRV_CONFIG_USB_CHARGE_ONLY
    USBD_Init(&husbd, NULL, 0);
    #else
    USBD_Pybricks_Desc_Init();
    USBD_Init(&husbd, &USBD_Pybricks_Desc, 0);
    USBD_RegisterClass(&husbd, &USBD_Pybricks_ClassDriver);
    USBD_Pybricks_RegisterInterface(&husbd, &USBD_Pybricks_fops);
    USBD_Start(&husbd);
    #endif

    static uint8_t stdin_buf[USBD_PYBRICKS_MAX_PACKET_SIZE * 2];
    lwrb_init(&stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));

    process_start(&pbdrv_usb_process);

    // VBUS may already be active
    process_poll(&pbdrv_usb_process);
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return pbdrv_usb_bcd;
}

// Event loop

PROCESS_THREAD(pbdrv_usb_process, ev, data) {
    static struct pt bcd_pt;
    static PBIO_ONESHOT(no_vbus_oneshot);
    static PBIO_ONESHOT(bcd_oneshot);
    static PBIO_ONESHOT(pwrdn_oneshot);
    static bool bcd_busy;
    static pbio_pybricks_error_t result;
    static struct etimer status_timer;
    static struct etimer transmit_timer;
    static uint32_t prev_status_flags = ~0;
    static uint32_t new_status_flags;

    PROCESS_POLLHANDLER({
        if (!bcd_busy && pbio_oneshot(!vbus_active, &no_vbus_oneshot)) {
            pbdrv_usb_bcd = PBDRV_USB_BCD_NONE;
        }

        // pbdrv_usb_stm32_bcd_detect() needs to run completely to the end,
        // otherwise the USB will be left in an inconsistent state. So we
        // have to wait for both the completion of the previous call and the
        // oneshot trigger before starting again.
        if (!bcd_busy && pbio_oneshot(vbus_active, &bcd_oneshot)) {
            PT_INIT(&bcd_pt);
            bcd_busy = true;
        }
    })

    PROCESS_BEGIN();

    etimer_set(&status_timer, 500);

    for (;;) {
        PROCESS_WAIT_EVENT();

        if (bcd_busy) {
            bcd_busy = PT_SCHEDULE(pbdrv_usb_stm32_bcd_detect(&bcd_pt));

            if (bcd_busy) {
                // All other USB functions are halted if BCD is busy
                continue;
            }
        }

        #if PBDRV_CONFIG_USB_CHARGE_ONLY
        // Communication logic skipped when charging only.
        continue;
        #endif

        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
            if (pbio_oneshot(true, &pwrdn_oneshot)) {
                USBD_Stop(&husbd);
                USBD_DeInit(&husbd);
            }

            // USB communication is stopped after a shutdown, but
            // the process is still needed to track the BCD state
            continue;
        }

        if (usb_in_sz) {
            switch (usb_in_buf[0]) {
                case USBD_PYBRICKS_OUT_EP_MSG_SUBSCRIBE:
                    pbdrv_usb_stm32_is_events_subscribed = usb_in_buf[1];
                    usb_response_buf[0] = USBD_PYBRICKS_IN_EP_MSG_RESPONSE;
                    pbio_set_uint32_le(&usb_response_buf[1], PBIO_PYBRICKS_ERROR_OK);
                    usb_response_sz = sizeof(usb_response_buf);
                    break;
                case USBD_PYBRICKS_OUT_EP_MSG_COMMAND:
                    if (usb_response_sz == 0) {
                        result = pbsys_command(usb_in_buf + 1, usb_in_sz - 1, PBSYS_COMMAND_TRANSPORT_USB);
                        usb_response_buf[0] = USBD_PYBRICKS_IN_EP_MSG_RESPONSE;
                        pbio_set_uint32_le(&usb_response_buf[1], result);
                        usb_response_sz = sizeof(usb_response_buf);
                    }
                    break;
            }

            // Prepare to receive the next packet
            usb_in_sz = 0;
            USBD_Pybricks_ReceivePacket(&husbd);
        }

        if (transmitting) {
            if (etimer_expired(&transmit_timer)) {
                // Transmission has taken too long, so reset the state to allow
                // new transmissions. This can happen if the host stops reading
                // data for some reason.
                pbdrv_usb_stm32_reset_tx_state();
            }

            continue;
        }

        new_status_flags = pbsys_status_get_flags();

        // Transmit. Give priority to response, then status updates, then stdout.
        if (usb_response_sz) {
            transmitting = true;
            USBD_Pybricks_TransmitPacket(&husbd, usb_response_buf, usb_response_sz);
        } else if (pbdrv_usb_stm32_is_events_subscribed) {
            if ((new_status_flags != prev_status_flags) || etimer_expired(&status_timer)) {
                usb_status_buf[0] = USBD_PYBRICKS_IN_EP_MSG_EVENT;
                _Static_assert(sizeof(usb_status_buf) + 1 >= PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE,
                    "size of status report does not match size of event");
                usb_status_sz = 1 + pbsys_status_get_status_report(&usb_status_buf[1]);

                etimer_restart(&status_timer);
                prev_status_flags = new_status_flags;

                transmitting = true;
                USBD_Pybricks_TransmitPacket(&husbd, usb_status_buf, usb_status_sz);
            } else if (usb_stdout_sz) {
                transmitting = true;
                USBD_Pybricks_TransmitPacket(&husbd, usb_stdout_buf, usb_stdout_sz);
            }
        }

        if (transmitting) {
            // If the FIFO isn't emptied quickly, then there probably isn't an
            // app anymore. This timer is used to detect such a condition.
            etimer_set(&transmit_timer, 5);
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_STM32F4
