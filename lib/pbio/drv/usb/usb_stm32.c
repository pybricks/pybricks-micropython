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
#include "./usb_stm32.h"

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
// Remember to set PBDRV_CONFIG_UART_DEBUG_FIRST_PORT in pbdrvconfig.
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

// These buffers need to be 32-bit aligned because the USB driver moves data
// to/from FIFOs in 32-bit chunks.
static uint8_t usb_in_buf[USBD_PYBRICKS_MAX_PACKET_SIZE] __aligned(4);
static uint8_t usb_response_buf[PBIO_PYBRICKS_USB_MESSAGE_SIZE(sizeof(uint32_t))] __aligned(4);
static uint8_t usb_status_buf[PBIO_PYBRICKS_USB_MESSAGE_SIZE(PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE)] __aligned(4);
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
 * Waits for USB to be plugged in and runs battery charger detection task.
 *
 * Based on HAL_PCDEx_BCD_VBUSDetect().
 *
 * @param [in]  pt  The protothread.
 */
static pbio_error_t pbdrv_usb_stm32_wait_until_usb_detected(pbio_os_state_t *state) {
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
    pbio_os_request_poll();
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
        *size = 0;
        return PBIO_ERROR_AGAIN;
    }

    *ptr++ = PBIO_PYBRICKS_IN_EP_MSG_EVENT;
    ptr_len--;

    *ptr++ = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
    ptr_len--;

    *size = MIN(*size, ptr_len);
    memcpy(ptr, data, *size);

    usb_stdout_sz = 1 + 1 + *size;

    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

uint32_t pbdrv_usb_stdout_tx_available(void) {
    if (!pbdrv_usb_stm32_is_events_subscribed) {
        return UINT32_MAX;
    }

    if (usb_stdout_sz) {
        return 0;
    }

    return sizeof(usb_stdout_buf) - 2; // 2 bytes for header
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

bool pbdrv_usb_connection_is_active(void) {
    return pbdrv_usb_stm32_is_events_subscribed;
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
    pbio_os_request_poll();
    return ret;
}

static USBD_StatusTypeDef Pybricks_Itf_ReadCharacteristic(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bRequest) {
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT:
            switch (req->wValue) {
                case 0x2A00: {
                    // GATT Device Name characteristic
                    const char *name = pbdrv_bluetooth_get_hub_name();
                    (void)USBD_CtlSendData(pdev, (uint8_t *)name, MIN(strlen(name), req->wLength));
                }
                break;

                case 0x2A26: {
                    // GATT Firmware Revision characteristic
                    const char *fw_version = PBIO_VERSION_STR;
                    (void)USBD_CtlSendData(pdev, (uint8_t *)fw_version, MIN(strlen(fw_version), req->wLength));
                }
                break;

                case 0x2A28: {
                    // GATT Software Revision characteristic
                    const char *sw_version = PBIO_PROTOCOL_VERSION_STR;
                    (void)USBD_CtlSendData(pdev, (uint8_t *)sw_version, MIN(strlen(sw_version), req->wLength));
                }
                break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS:
            switch (req->wValue) {
                case 0x0003: {
                    // Pybricks hub capabilities characteristic
                    uint8_t caps[PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE];
                    pbio_pybricks_hub_capabilities(caps,
                        USBD_PYBRICKS_MAX_PACKET_SIZE - 1,
                        PBSYS_CONFIG_APP_FEATURE_FLAGS,
                        pbsys_storage_get_maximum_program_size(),
                        PBSYS_CONFIG_HMI_NUM_SLOTS);
                    (void)USBD_CtlSendData(pdev, caps, MIN(sizeof(caps), req->wLength));
                }
                break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;
        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }

    return ret;
}

USBD_Pybricks_ItfTypeDef USBD_Pybricks_fops = {
    .Init = Pybricks_Itf_Init,
    .DeInit = Pybricks_Itf_DeInit,
    .Receive = Pybricks_Itf_Receive,
    .TransmitCplt = Pybricks_Itf_TransmitCplt,
    .ReadCharacteristic = Pybricks_Itf_ReadCharacteristic,
};

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return pbdrv_usb_bcd;
}

/**
 * Pybricks system command handler.
 */
static pbdrv_usb_receive_handler_t pbdrv_usb_receive_handler;

void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
    pbdrv_usb_receive_handler = handler;
}

/**
 * Buffer for scheduled status message.
 */
static uint8_t pbdrv_usb_status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool pbdrv_usb_status_data_pending;

void pbdrv_usb_schedule_status_update(const uint8_t *status_msg) {
    // Ignore if message identical to last.
    if (!memcmp(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data))) {
        return;
    }

    // Schedule to send whenever the Bluetooth process gets round to it.
    memcpy(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data));
    pbdrv_usb_status_data_pending = true;
    pbio_os_request_poll();
}

/**
 * Non-blocking poll handler to process pending incoming messages.
 */
static void pbdrv_usb_handle_data_in(void) {
    if (!usb_in_sz) {
        return;
    }

    switch (usb_in_buf[0]) {
        case PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE:
            pbdrv_usb_stm32_is_events_subscribed = usb_in_buf[1];
            usb_response_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_RESPONSE;
            pbio_set_uint32_le(&usb_response_buf[1], PBIO_PYBRICKS_ERROR_OK);
            usb_response_sz = sizeof(usb_response_buf);

            // Schedule sending current status immediately after subscribing.
            pbdrv_usb_status_data_pending = true;
            break;
        case PBIO_PYBRICKS_OUT_EP_MSG_COMMAND:
            if (usb_response_sz == 0 && pbdrv_usb_receive_handler) {
                pbio_pybricks_error_t result = pbdrv_usb_receive_handler(usb_in_buf + 1, usb_in_sz - 1);
                usb_response_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_RESPONSE;
                pbio_set_uint32_le(&usb_response_buf[1], result);
                usb_response_sz = sizeof(usb_response_buf);
            }
            break;
    }

    // Prepare to receive the next packet
    usb_in_sz = 0;
    USBD_Pybricks_ReceivePacket(&husbd);
}

static void pbdrv_usb_handle_data_out(void) {
    #if PBDRV_CONFIG_USB_CHARGE_ONLY
    // Nothing to do when only charging.
    return;
    #endif

    static pbio_os_timer_t transmit_timer;

    if (transmitting) {
        if (pbio_os_timer_is_expired(&transmit_timer)) {
            // Transmission has taken too long, so reset the state to allow
            // new transmissions. This can happen if the host stops reading
            // data for some reason.
            pbdrv_usb_stm32_reset_tx_state();
        }
        // Still transmitting or expired, so can't do anything else for now.
        return;
    }

    // We are not currently transmitting, so we can transmit something new.
    // Give priority to response, then status updates, then stdout.
    if (usb_response_sz) {
        transmitting = true;
        USBD_Pybricks_TransmitPacket(&husbd, usb_response_buf, usb_response_sz);
    } else if (pbdrv_usb_stm32_is_events_subscribed) {
        if (pbdrv_usb_status_data_pending) {
            pbdrv_usb_status_data_pending = false;

            usb_status_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_EVENT;
            _Static_assert(sizeof(usb_status_buf) + 1 >= PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE,
                "size of status report does not match size of event");

            memcpy(&usb_status_buf[1], pbdrv_usb_status_data, sizeof(pbdrv_usb_status_data));
            usb_status_sz = PBIO_PYBRICKS_USB_MESSAGE_SIZE(sizeof(pbdrv_usb_status_data));

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
        pbio_os_timer_set(&transmit_timer, 50);
    }
}

static pbio_os_process_t pbdrv_usb_process;

static pbio_error_t pbdrv_usb_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    // Runs every time. If there is no connection, there just won't be data.
    pbdrv_usb_handle_data_in();

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        DEBUG_PRINT("Waiting for USB to be plugged in.\n");
        PBIO_OS_AWAIT(state, &sub, pbdrv_usb_stm32_wait_until_usb_detected(&sub));
        DEBUG_PRINT("Detected USB bcd type: %d\n", pbdrv_usb_bcd);

        // Handle outgoing data until unplugged or process cancelled.
        DEBUG_PRINT("Ready to transmit.\n");
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);
        if (vbus_active && pbdrv_usb_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL) {
            pbdrv_usb_handle_data_out();
            return PBIO_ERROR_AGAIN;
        }

        PBIO_OS_AWAIT_WHILE(state, vbus_active);
        pbdrv_usb_stm32_reset_tx_state();
        DEBUG_PRINT("USB unplugged.\n");
    }

    // Unreachable. On cancellation, the charger detection process keeps
    // running. It will just skip the data handler.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

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

    pbio_os_process_start(&pbdrv_usb_process, pbdrv_usb_process_thread, NULL);
}

void pbdrv_usb_deinit(void) {
    USBD_Stop(&husbd);
    USBD_DeInit(&husbd);
    pbio_os_process_make_request(&pbdrv_usb_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBDRV_CONFIG_USB_STM32F4
