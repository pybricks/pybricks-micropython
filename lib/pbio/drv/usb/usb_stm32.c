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
#include <usbd_pybricks.h>

#include <pbdrv/usb.h>
#include <pbio/util.h>
#include <pbsys/command.h>

#include "../charger/charger.h"
#include "./usb_stm32.h"

#define UUID_SZ     (128 / 8)

PROCESS(pbdrv_usb_process, "USB");

static uint8_t usb_in_buf[USBD_PYBRICKS_MAX_PACKET_SIZE];
static uint8_t usb_out_buf[USBD_PYBRICKS_MAX_PACKET_SIZE];
static volatile uint32_t usb_in_sz;
static volatile uint32_t usb_out_sz;

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
  * @brief  Pybricks_Itf_Init
  *         Initializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t Pybricks_Itf_Init(void) {
    USBD_Pybricks_SetTxBuffer(&husbd, usb_out_buf, sizeof(usb_out_buf));
    USBD_Pybricks_SetRxBuffer(&husbd, usb_in_buf);
    usb_in_sz = 0;
    usb_out_sz = 0;

    return USBD_OK;
}

/**
  * @brief  Pybricks_Itf_DeInit
  *         DeInitializes the Pybricks media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t Pybricks_Itf_DeInit(void) {
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
static int8_t Pybricks_Itf_Receive(uint8_t *Buf, uint32_t *Len) {
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
static int8_t Pybricks_Itf_TransmitCplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
    usb_out_sz = 0;
    process_poll(&pbdrv_usb_process);
    return USBD_OK;
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

    USBD_Init(&husbd, &Pybricks_Desc, 0);
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
    static bool bcd_busy;

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

    for (;;) {
        PROCESS_WAIT_EVENT();

        if (bcd_busy) {
            bcd_busy = PT_SCHEDULE(pbdrv_usb_stm32_bcd_detect(&bcd_pt));
        }

        if (usb_in_sz) {
            if ((usb_in_sz >= UUID_SZ) &&
                pbio_uuid128_le_compare(usb_in_buf, pbio_pybricks_command_event_char_uuid)) {
                pbsys_command(usb_in_buf + UUID_SZ, usb_in_sz - UUID_SZ);
            }

            usb_in_sz = 0;

            // Prepare to receive the next packet
            USBD_Pybricks_ReceivePacket(&husbd);
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_STM32F4
