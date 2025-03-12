// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef _PBIO_PORT_LUMP_H_
#define _PBIO_PORT_LUMP_H_

#include <contiki.h>
#include <pbio/angle.h>
#include <pbio/port.h>
#include <pbdrv/ioport.h>
#include <pbdrv/uart.h>

#include <lego/lump.h>

typedef struct _pbio_port_lump_dev_t pbio_port_lump_dev_t;

/**
 * Structure containing information about a legodev device mode.
 */
typedef struct {
    /**< The number of values returned by the current mode. */
    uint8_t num_values;
    /**< The data type of the values returned by the current mode. */
    lump_data_type_t data_type;
    /**< Whether the mode supports setting data */
    bool writable;
    /**< Zero-terminated name of the mode. */
    char name[LUMP_MAX_NAME_SIZE + 1];
} pbio_port_lump_mode_info_t;

#if PBIO_CONFIG_PORT_LUMP

pbio_port_lump_dev_t *pbio_port_lump_init_instance(uint8_t device_index, struct process *parent_process);

PT_THREAD(pbio_port_lump_sync_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer));

PT_THREAD(pbio_port_lump_data_send_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer));

PT_THREAD(pbio_port_lump_data_recv_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev));

pbio_error_t pbio_port_lump_is_ready(pbio_port_lump_dev_t *lump_dev);

pbio_error_t pbio_port_lump_set_mode(pbio_port_lump_dev_t *lump_dev, uint8_t mode);

pbio_error_t pbio_port_lump_get_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, void **data);

pbio_error_t pbio_port_lump_set_mode_with_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, const void *data, uint8_t size);

pbio_error_t pbio_port_lump_assert_type_id(pbio_port_lump_dev_t *lump_dev, lego_device_type_id_t *type_id);

pbio_error_t pbio_port_lump_get_info(pbio_port_lump_dev_t *lump_dev, uint8_t *num_modes, uint8_t *current_mode, pbio_port_lump_mode_info_t **mode_info);

pbio_error_t pbio_port_lump_request_reset(pbio_port_lump_dev_t *lump_dev);

pbio_error_t pbio_port_lump_get_angle(pbio_port_lump_dev_t *lump_dev, pbio_angle_t *angle, bool get_abs_angle);

pbio_port_power_requirements_t pbio_port_lump_get_power_requirements(pbio_port_lump_dev_t *lump_dev);

#else // PBIO_CONFIG_PORT_LUMP

static inline pbio_port_lump_dev_t *pbio_port_lump_init_instance(uint8_t device_index, struct process *parent_process) {
    return NULL;
}

static inline pbio_error_t pbio_port_lump_is_ready(pbio_port_lump_dev_t *lump_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_set_mode(pbio_port_lump_dev_t *lump_dev, uint8_t mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_get_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, void **data) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_set_mode_with_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, const void *data, uint8_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_assert_type_id(pbio_port_lump_dev_t *lump_dev, lego_device_type_id_t *type_id) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_get_info(pbio_port_lump_dev_t *lump_dev, uint8_t *num_modes, uint8_t *current_mode, pbio_port_lump_mode_info_t **mode_info) {
    *mode_info = NULL;
    *current_mode = 0;
    *num_modes = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_request_reset(pbio_port_lump_dev_t *lump_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_port_lump_get_angle(pbio_port_lump_dev_t *lump_dev, pbio_angle_t *angle, bool get_abs_angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_port_power_requirements_t pbio_port_lump_get_power_requirements(pbio_port_lump_dev_t *lump_dev) {
    return PBIO_PORT_POWER_REQUIREMENTS_NONE;
}

static inline PT_THREAD(pbio_port_lump_sync_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer)) {
    PT_BEGIN(pt);
    PT_END(pt);
}

static inline PT_THREAD(pbio_port_lump_data_send_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer)) {
    PT_BEGIN(pt);
    PT_END(pt);
}

static inline PT_THREAD(pbio_port_lump_data_recv_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev)) {
    PT_BEGIN(pt);
    PT_END(pt);
}


#endif // PBIO_CONFIG_PORT_LUMP

#endif // _PBIO_PORT_LUMP_H_
