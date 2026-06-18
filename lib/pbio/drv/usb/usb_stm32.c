// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2025 The Pybricks Authors

// Main file for STM32F4 USB driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4

#include <string.h>
#include <stdbool.h>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_pcd_ex.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_pybricks.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/usb.h>
#include <pbio/protocol.h>
#include <pbio/os.h>
#include <pbio/util.h>
#include <pbio/version.h>
#include <pbsys/command.h>
#include <pbsys/config.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>

#include "../charger/charger.h"
#include "./usb.h"
#include "./usb_stm32.h"

// These buffers need to be 32-bit aligned because the USB driver moves data
// to/from FIFOs in 32-bit chunks.
static uint8_t usb_in_buf[USBD_PYBRICKS_MAX_PACKET_SIZE] __aligned(4);
static volatile uint32_t usb_in_sz;
static volatile bool transmitting;

static USBD_HandleTypeDef husbd;
static PCD_HandleTypeDef hpcd;

static volatile bool vbus_active;
static pbdrv_usb_bcd_t pbdrv_usb_bcd;

/**
 * Waits for USB to be plugged in and runs battery charger detection task.
 *
 * Based on HAL_PCDEx_BCD_VBUSDetect().
 *
 * @param [in]  pt  The protothread.
 */
pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    static pbio_os_timer_t timer;
    USB_OTG_GlobalTypeDef *USBx = hpcd.Instance;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_usb_bcd = PBDRV_USB_BCD_NONE;

    // Wait until USB plugged in.
    PBIO_OS_AWAIT_UNTIL(state, vbus_active);

    // Disable all other USB functions.
    HAL_PCDEx_ActivateBCD(&hpcd);

    // Enable Data Contact Detect.
    USBx->GCCFG |= USB_OTG_GCCFG_DCDEN;

    // Wait Detect flag or a timeout.
    pbio_os_timer_set(&timer, 1000);
    PBIO_OS_AWAIT_UNTIL(state, (USBx->GCCFG & USB_OTG_GCCFG_DCDET) || pbio_os_timer_is_expired(&timer));
    if (pbio_os_timer_is_expired(&timer)) {
        USBx->GCCFG &= ~USB_OTG_GCCFG_DCDEN;
        HAL_PCDEx_DeActivateBCD(&hpcd);
        pbdrv_usb_bcd = PBDRV_USB_BCD_NONSTANDARD;
        return PBIO_SUCCESS;
    }

    // Correct response received.
    PBIO_OS_AWAIT_MS(state, &timer, 100);

    // Primary detection: checks if connected to Standard Downstream Port
    // without charging capability.
    USBx->GCCFG &= ~USB_OTG_GCCFG_DCDEN;
    USBx->GCCFG |= USB_OTG_GCCFG_PDEN;

    PBIO_OS_AWAIT_MS(state, &timer, 100);

    if (!(USBx->GCCFG & USB_OTG_GCCFG_PDET)) {
        /* Case of Standard Downstream Port */
        USBx->GCCFG &= ~USB_OTG_GCCFG_PDEN;
        pbdrv_usb_bcd = PBDRV_USB_BCD_STANDARD_DOWNSTREAM;
    } else {
        /* start secondary detection to check connection to Charging Downstream
        Port or Dedicated Charging Port */
        USBx->GCCFG &= ~USB_OTG_GCCFG_PDEN;
        USBx->GCCFG |= USB_OTG_GCCFG_SDEN;

        PBIO_OS_AWAIT_MS(state, &timer, 100);

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

    // USB was unplugged in the middle of all this, so start over.
    if (!vbus_active) {
        PBIO_OS_ASYNC_RESET(state);
        pbio_os_request_poll();
        return PBIO_ERROR_AGAIN;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

bool pbdrv_usb_is_ready(void) {
    return vbus_active && pbdrv_usb_bcd != PBDRV_USB_BCD_NONE;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return pbdrv_usb_bcd;
}

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
    pbio_os_request_poll();
}

static void pbdrv_usb_stm32_reset_tx_state(void) {
    transmitting = false;
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
    pbio_os_request_poll();
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
    transmitting = false;
    pbio_os_request_poll();
    return USBD_OK;
}

static USBD_StatusTypeDef Pybricks_Itf_SetControlLineState(bool dtr) {
    // The DTR signal indicates whether a host application has opened the serial
    // port. This is the USB analog of a BLE host subscribing to notifications.
    pbdrv_usb_on_dtr_changed(dtr);
    return USBD_OK;
}

USBD_Pybricks_ItfTypeDef USBD_Pybricks_fops = {
    .Init = Pybricks_Itf_Init,
    .DeInit = Pybricks_Itf_DeInit,
    .Receive = Pybricks_Itf_Receive,
    .TransmitCplt = Pybricks_Itf_TransmitCplt,
    .SetControlLineState = Pybricks_Itf_SetControlLineState,
};

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    pbdrv_usb_stm32_reset_tx_state();
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_usb_tx_chunk(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    if (transmitting) {
        return PBIO_ERROR_BUSY;
    }

    transmitting = true;
    pbio_os_timer_set(&timer, PBDRV_USB_TRANSMIT_TIMEOUT);
    USBD_Pybricks_TransmitPacket(&husbd, (uint8_t *)data, size);
    PBIO_OS_AWAIT_UNTIL(state, !transmitting || pbio_os_timer_is_expired(&timer));

    if (pbio_os_timer_is_expired(&timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {

    if (!usb_in_sz) {
        return 0;
    }

    uint32_t size = usb_in_sz;
    memcpy(data, usb_in_buf, size);

    // Prepare to receive the next packet
    usb_in_sz = 0;
    USBD_Pybricks_ReceivePacket(&husbd);

    return size;
}

void pbdrv_usb_init_device(void) {
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
}

void pbdrv_usb_deinit_device(void) {
    USBD_Stop(&husbd);
    USBD_DeInit(&husbd);
}

#endif // PBDRV_CONFIG_USB_STM32F4
