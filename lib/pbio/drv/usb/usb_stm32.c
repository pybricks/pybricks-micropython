// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Main file for STM32F4 USB driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4

#include <stdbool.h>
#include <string.h>

#include <contiki.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_pcd_ex.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_pybricks.h>

#include <pbdrv/clock.h>
#include <pbdrv/usb.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbsys/command.h>
#include <pbsys/status.h>

#include "../charger/charger.h"
#include "./usb_stm32.h"

PROCESS(pbdrv_usb_process, "USB");

static uint8_t usb_in_buf[USBD_PYBRICKS_MAX_PACKET_SIZE];
static uint8_t usb_response_buf[1 + sizeof(uint32_t)];
static uint8_t usb_status_buf[1 + 1 + sizeof(uint32_t)];
static uint8_t usb_stdout_buf[USBD_PYBRICKS_MAX_PACKET_SIZE];
static volatile uint32_t usb_in_sz;
static volatile uint32_t usb_response_sz;
static volatile uint32_t usb_status_sz;
static volatile uint32_t usb_stdout_sz;
static volatile clock_time_t usb_response_tx_time;
static volatile clock_time_t usb_stdout_tx_time;
static volatile bool transmitting;

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

/**
 * Queues data to be transmitted via USB.
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if @p data was queued, ::PBIO_ERROR_AGAIN
 *                          if @p data could not be queued at this time (e.g. buffer
 *                          is full), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active USB connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support USB.
 */
pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    uint8_t *ptr = usb_stdout_buf;
    uint32_t ptr_len = sizeof(usb_stdout_buf);

    // TODO: return PBIO_ERROR_INVALID_OP if not connected

    if (usb_stdout_sz) {
        return PBIO_ERROR_AGAIN;
    }

    *ptr = USBD_PYBRICKS_MSG_EVENT;
    ptr++;
    ptr_len--;

    *ptr = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
    ptr++;
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

/**
  * @brief  Pybricks_Itf_Init
  *         Initializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_Init(void) {
    USBD_Pybricks_SetRxBuffer(&husbd, usb_in_buf);
    usb_in_sz = 0;
    usb_response_sz = 0;
    usb_status_sz = 0;
    usb_stdout_sz = 0;
    transmitting = false;

    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_DeInit
  *         DeInitializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_DeInit(void) {
    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  * @param  Buf: Buffer of data to be transmitted
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_Receive(uint8_t *Buf, uint32_t *Len) {
    if (*Len > sizeof(usb_in_buf)) {
        return USBD_FAIL;
    }

    memcpy(usb_in_buf, Buf, *Len);
    usb_in_sz = *Len;
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
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static USBD_StatusTypeDef Pybricks_Itf_TransmitCplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
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

static USBD_Pybricks_ItfTypeDef USBD_Pybricks_fops = {
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

    USBD_Pybricks_Desc_Init();
    USBD_Init(&husbd, &USBD_Pybricks_Desc, 0);
    USBD_RegisterClass(&husbd, &USBD_Pybricks_ClassDriver);
    USBD_Pybricks_RegisterInterface(&husbd, &USBD_Pybricks_fops);
    USBD_Start(&husbd);

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
    static struct etimer timer;
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

    // Prepare to receive the first packet
    USBD_Pybricks_ReceivePacket(&husbd);

    etimer_set(&timer, 500);

    for (;;) {
        PROCESS_WAIT_EVENT();

        if (bcd_busy) {
            bcd_busy = PT_SCHEDULE(pbdrv_usb_stm32_bcd_detect(&bcd_pt));

            if (bcd_busy) {
                // All other USB functions are halted if BCD is busy
                continue;
            }
        }

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
            if (usb_in_buf[0] == USBD_PYBRICKS_MSG_COMMAND) {
                if (usb_response_sz == 0) {
                    result = pbsys_command(usb_in_buf + 1, usb_in_sz - 1);
                    usb_response_buf[0] = USBD_PYBRICKS_MSG_COMMAND_RESPONSE;
                    pbio_set_uint32_le(&usb_response_buf[1], result);
                    usb_response_sz = sizeof(usb_response_buf);

                    // Prepare to receive the next packet
                    usb_in_sz = 0;
                    USBD_Pybricks_ReceivePacket(&husbd);
                }
            } else {
                // Ignore unknown packet
                usb_in_sz = 0;
                USBD_Pybricks_ReceivePacket(&husbd);
            }
        }

        if (transmitting) {
            continue;
        }

        new_status_flags = pbsys_status_get_flags();

        // Transmit. Give priority to status updates, then
        // cycle between response and stdout buffers.
        if ((new_status_flags != prev_status_flags) || etimer_expired(&timer)) {

            usb_status_buf[0] = USBD_PYBRICKS_MSG_EVENT;
            usb_status_sz = 1 + pbio_pybricks_event_status_report(&usb_status_buf[1], new_status_flags);

            etimer_restart(&timer);
            prev_status_flags = new_status_flags;

            transmitting = true;
            USBD_Pybricks_TransmitPacket(&husbd, usb_status_buf, usb_status_sz);

        } else if (usb_response_sz &&
                   (!usb_stdout_sz || (usb_response_tx_time < usb_stdout_tx_time))) {

            transmitting = true;
            usb_response_tx_time = clock_time();
            USBD_Pybricks_TransmitPacket(&husbd, usb_response_buf, usb_response_sz);

        } else if (usb_stdout_sz) {

            transmitting = true;
            usb_stdout_tx_time = clock_time();
            USBD_Pybricks_TransmitPacket(&husbd, usb_stdout_buf, usb_stdout_sz);
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_STM32F4
