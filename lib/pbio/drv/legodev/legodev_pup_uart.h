// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_PUP_UART_H_
#define _INTERNAL_PBDRV_LEGODEV_PUP_UART_H_

#include <pbdrv/config.h>

#include <contiki.h>

#include <pbio/dcmotor.h>

/**
 * Opaque handle to a legodev pup UART device instance.
 */
typedef struct _pbdrv_legodev_pup_uart_dev_t pbdrv_legodev_pup_uart_dev_t;

#if PBDRV_CONFIG_LEGODEV_PUP_UART

pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_pup_uart_configure(uint8_t device_index, uint8_t uart_driver_index, pbio_dcmotor_t *dcmotor);

void pbdrv_legodev_pup_uart_set_dummy_info(pbdrv_legodev_pup_uart_dev_t *ludev, pbdrv_legodev_type_id_t type_id);

pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_get_uart_dev(pbdrv_legodev_dev_t *legodev);

PT_THREAD(pbdrv_legodev_pup_uart_thread(struct pt *pt, pbdrv_legodev_pup_uart_dev_t *port_data));

void pbdrv_legodev_pup_uart_process_poll(void);

#else // PBDRV_CONFIG_LEGODEV_PUP_UART

static inline pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_pup_uart_configure(uint8_t device_index, uint8_t uart_driver_index, pbio_dcmotor_t *dcmotor) {
    return NULL;
}

static inline pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_get_uart_dev(pbdrv_legodev_dev_t *legodev) {
    return NULL;
}

static inline PT_THREAD(pbdrv_legodev_pup_uart_thread(struct pt *pt, pbdrv_legodev_pup_uart_dev_t *port_data)) {
    PT_BEGIN(pt);
    PT_END(pt);
}

static inline void pbdrv_legodev_pup_uart_process_poll(void) {
}

#endif // PBDRV_CONFIG_LEGODEV_PUP_UART

#endif // _INTERNAL_PBDRV_LEGODEV_PUP_UART_H_
