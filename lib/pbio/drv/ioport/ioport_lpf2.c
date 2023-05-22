// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// LEGO Powered Up (Power Functions 2) I/O port

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_LPF2

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define debug_pr(fmt, ...)   printf((fmt), __VA_ARGS__)
#else
#define debug_pr(...)
#endif

#include <assert.h>
#include <stdbool.h>

#include <contiki.h>
#include <lego_uart.h>

#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/uartdev.h>

#include "ioport_lpf2.h"

/** The number of consecutive repeated detections needed for an affirmative ID. */
#define AFFIRMATIVE_MATCH_COUNT 20

typedef enum {
    DEV_ID_GROUP_GND,
    DEV_ID_GROUP_VCC,
    DEV_ID_GROUP_PULL_DOWN,
    DEV_ID_GROUP_OPEN,
} dev_id_group_t;

// Device connection manager state for each port
typedef struct {
    dev_id_group_t dev_id1_group;
    pbio_iodev_type_id_t type_id;
    pbio_iodev_type_id_t prev_type_id;
    uint8_t gpio_value;
    uint8_t prev_gpio_value;
    uint8_t dev_id_match_count;
} dcm_data_t;

typedef struct {
    pbio_iodev_t *iodev;
    const pbdrv_ioport_lpf2_port_platform_data_t *pdata;
    dcm_data_t dcm;
    struct pt pt;
    pbio_iodev_type_id_t connected_type_id;
    pbio_iodev_type_id_t prev_type_id;
} ioport_dev_t;

typedef struct {
    pbio_iodev_info_t info;
    pbio_iodev_mode_t mode;
} basic_info_t;

static const basic_info_t basic_infos[] = {
    [PBIO_IODEV_TYPE_ID_LPF2_MMOTOR] = {
        .info = {
            .type_id = PBIO_IODEV_TYPE_ID_LPF2_MMOTOR,
            .capability_flags = PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT,
            .num_modes = 1,
        },
        .mode = {
            .num_values = 1,
            .data_type = PBIO_IODEV_DATA_TYPE_INT8 | PBIO_IODEV_DATA_TYPE_WRITABLE,
        },
    },
    [PBIO_IODEV_TYPE_ID_LPF2_TRAIN] = {
        .info = {
            .type_id = PBIO_IODEV_TYPE_ID_LPF2_TRAIN,
            .capability_flags = PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT,
            .num_modes = 1,
        },
        .mode = {
            .num_values = 1,
            .data_type = PBIO_IODEV_DATA_TYPE_INT8 | PBIO_IODEV_DATA_TYPE_WRITABLE,
        },
    },
    [PBIO_IODEV_TYPE_ID_LPF2_LIGHT] = {
        .info = {
            .type_id = PBIO_IODEV_TYPE_ID_LPF2_LIGHT,
            .capability_flags = PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT,
            .num_modes = 1,
        },
        .mode = {
            .num_values = 1,
            .data_type = PBIO_IODEV_DATA_TYPE_INT8 | PBIO_IODEV_DATA_TYPE_WRITABLE,
        },
    },
    [PBIO_IODEV_TYPE_ID_LPF2_LIGHT1] = {
        .info = {
            .type_id = PBIO_IODEV_TYPE_ID_LPF2_LIGHT1,
            .capability_flags = PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT,
            .num_modes = 1,
        },
        .mode = {
            .num_values = 1,
            .data_type = PBIO_IODEV_DATA_TYPE_INT8 | PBIO_IODEV_DATA_TYPE_WRITABLE,
        },
    },
    [PBIO_IODEV_TYPE_ID_LPF2_LIGHT2] = {
        .info = {
            .type_id = PBIO_IODEV_TYPE_ID_LPF2_LIGHT2,
            .capability_flags = PBIO_IODEV_CAPABILITY_FLAG_NONE,
            .num_modes = 1,
        },
        .mode = {
            .num_values = 1,
            .data_type = PBIO_IODEV_DATA_TYPE_INT8 | PBIO_IODEV_DATA_TYPE_WRITABLE,
        },
    },
};

static pbio_iodev_t basic_devs[PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS];

static const pbio_iodev_type_id_t ioport_type_id_lookup[3][3] = {
    /* ID1 */ [DEV_ID_GROUP_GND] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBIO_IODEV_TYPE_ID_LPF2_POWER,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBIO_IODEV_TYPE_ID_LPF2_TURN,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT2,
    },
    /* ID1 */ [DEV_ID_GROUP_VCC] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBIO_IODEV_TYPE_ID_LPF2_TRAIN,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBIO_IODEV_TYPE_ID_LPF2_LMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT1,
    },
    /* ID1 */ [DEV_ID_GROUP_PULL_DOWN] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBIO_IODEV_TYPE_ID_LPF2_MMOTOR,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBIO_IODEV_TYPE_ID_LPF2_XMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT,
    },
};

PROCESS(pbdrv_ioport_lpf2_process, "I/O port");

static ioport_dev_t ioport_devs[PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS];

void pbdrv_ioport_lpf2_init(void) {
    process_start(&pbdrv_ioport_lpf2_process);
}

void pbdrv_ioport_lpf2_shutdown(void) {
    process_exit(&pbdrv_ioport_lpf2_process);
}

static void ioport_enable_uart(ioport_dev_t *ioport) {
    const pbdrv_ioport_lpf2_port_platform_data_t *pdata = ioport->pdata;

    pbdrv_gpio_alt(&pdata->uart_rx, pdata->alt);
    pbdrv_gpio_alt(&pdata->uart_tx, pdata->alt);
    pbdrv_gpio_out_low(&pdata->uart_buf);
}

static void init_one(uint8_t ioport) {
    const pbdrv_ioport_lpf2_port_platform_data_t *pdata =
        &pbdrv_ioport_lpf2_platform_data.ports[ioport];

    ioport_devs[ioport].pdata = pdata;

    PT_INIT(&ioport_devs[ioport].pt);

    pbdrv_gpio_input(&pdata->id1);
    pbdrv_gpio_input(&pdata->id2);
    pbdrv_gpio_input(&pdata->uart_buf);
    pbdrv_gpio_input(&pdata->uart_tx);
    pbdrv_gpio_input(&pdata->uart_rx);

    // These should be set by default already, but it seems that the bootloader
    // on the Technic hub changes these and causes wrong detection if we don't
    // make sure pull is disabled.
    pbdrv_gpio_set_pull(&pdata->id1, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pdata->id2, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pdata->uart_buf, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pdata->uart_tx, PBDRV_GPIO_PULL_NONE);
    pbdrv_gpio_set_pull(&pdata->uart_rx, PBDRV_GPIO_PULL_NONE);

    basic_devs[ioport].port = PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT + ioport;
}

// TODO: This should be moved to a common ioport_core.c file or removed entirely
pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    if (port < PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT || port > PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }

    ioport_dev_t *ioport = &ioport_devs[port - PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT];

    if (ioport->dcm.dev_id_match_count < AFFIRMATIVE_MATCH_COUNT) {
        // the device connection manager hasn't figured out what is or isn't connected yet
        return PBIO_ERROR_AGAIN;
    }

    *iodev = ioport->iodev;
    if (*iodev == NULL) {
        return PBIO_ERROR_NO_DEV;
    }

    // If there is an iodev but we don't know which one yet, it is syncing
    if ((*iodev)->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id) {
    #if PBDRV_CONFIG_IOPORT_LPF2_MOVE_HUB_HACK
    if (port == PBIO_PORT_ID_A || port == PBIO_PORT_ID_B) {
        *type_id = PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR;
        return PBIO_SUCCESS;
    }
    #endif

    // Try to get iodev.
    pbio_iodev_t *iodev;
    pbio_error_t err = pbdrv_ioport_get_iodev(port, &iodev);

    // PBIO_ERROR_NO_DEV is allowed; it means nothing is attached.
    if (err == PBIO_ERROR_NO_DEV) {
        *type_id = PBIO_IODEV_TYPE_ID_NONE;
        return PBIO_SUCCESS;
    }

    if (err != PBIO_SUCCESS) {
        return err;
    }

    // This operation is invalid for non-motors like powered sensors.
    if (!PBIO_IODEV_IS_DC_OUTPUT(iodev)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get the device ID for the motor or light.
    *type_id = iodev->info->type_id;

    return PBIO_SUCCESS;
}

// This is the device connection manager (dcm). It monitors the ID1 and ID2 pins
// on the port to see when devices are connected or disconnected.
// It is expected for there to be a 2ms delay between calls to this function.
static PT_THREAD(poll_dcm(ioport_dev_t * ioport)) {
    struct pt *pt = &ioport->pt;
    dcm_data_t *data = &ioport->dcm;
    const pbdrv_ioport_lpf2_port_platform_data_t pdata = *ioport->pdata;

    PT_BEGIN(pt);

    data->type_id = PBIO_IODEV_TYPE_ID_NONE;
    data->dev_id1_group = DEV_ID_GROUP_OPEN;

    // set ID1 high
    pbdrv_gpio_out_high(&pdata.uart_tx);
    pbdrv_gpio_out_low(&pdata.uart_buf);

    // set ID2 as input
    pbdrv_gpio_input(&pdata.id2);

    PT_YIELD(pt);

    // save current ID2 value
    data->prev_gpio_value = pbdrv_gpio_input(&pdata.id2);

    // set ID1 low
    pbdrv_gpio_out_low(&pdata.uart_tx);

    PT_YIELD(pt);

    // read ID2
    data->gpio_value = pbdrv_gpio_input(&pdata.id2);

    // if ID2 changed from high to low
    if (data->prev_gpio_value == 1 && data->gpio_value == 0) {
        // we have touch sensor
        data->type_id = PBIO_IODEV_TYPE_ID_LPF2_TOUCH;

        // set ID1 as input
        pbdrv_gpio_out_high(&pdata.uart_buf);
        pbdrv_gpio_input(&pdata.uart_tx);

        PT_YIELD(pt);

        // ID1 is inverse of touch sensor value
        // TODO: save this value to sensor data
        // sensor_data = !pbdrv_gpio_input(&pdata.id1);
    }
    // if ID2 changed from low to high
    else if (data->prev_gpio_value == 0 && data->gpio_value == 1) {
        data->type_id = PBIO_IODEV_TYPE_ID_LPF2_TPOINT;
    } else {
        // read ID1
        data->prev_gpio_value = pbdrv_gpio_input(&pdata.id1);

        // set ID1 high
        pbdrv_gpio_out_high(&pdata.uart_tx);

        PT_YIELD(pt);

        // read ID1
        data->gpio_value = pbdrv_gpio_input(&pdata.id1);

        // if ID1 did not change and is high
        if (data->prev_gpio_value == 1 && data->gpio_value == 1) {
            // we have ID1 == VCC
            data->dev_id1_group = DEV_ID_GROUP_VCC;
        }
        // if ID1 did not change and is low
        else if (data->prev_gpio_value == 0 && data->gpio_value == 0) {
            // we have ID1 == GND
            data->dev_id1_group = DEV_ID_GROUP_GND;
        } else {
            // set ID1 as input
            pbdrv_gpio_out_high(&pdata.uart_buf);
            pbdrv_gpio_input(&pdata.uart_tx);

            PT_YIELD(pt);

            // read ID1
            if (pbdrv_gpio_input(&pdata.id1) == 1) {
                // we have ID1 == open
                data->dev_id1_group = DEV_ID_GROUP_OPEN;
            } else {
                // we have ID1 == pull down
                data->dev_id1_group = DEV_ID_GROUP_PULL_DOWN;
            }
        }

        PT_YIELD(pt);

        // set ID1 as input
        pbdrv_gpio_out_high(&pdata.uart_buf);
        pbdrv_gpio_input(&pdata.uart_tx);

        // set ID2 high
        pbdrv_gpio_out_high(&pdata.id2);

        PT_YIELD(pt);

        // read ID1
        data->prev_gpio_value = pbdrv_gpio_input(&pdata.id1);

        // set ID2 low
        pbdrv_gpio_out_low(&pdata.id2);

        PT_YIELD(pt);

        // read ID1
        data->gpio_value = pbdrv_gpio_input(&pdata.id1);

        // if ID1 changed from high to low
        if (data->prev_gpio_value == 1 && data->gpio_value == 0) {
            // if we have ID1 = open
            if (data->dev_id1_group == DEV_ID_GROUP_OPEN) {
                // then we have this
                data->type_id = PBIO_IODEV_TYPE_ID_LPF2_3_PART;
            }
        }
        // if ID1 changed from low to high
        else if (data->prev_gpio_value == 0 && data->gpio_value == 1) {
            // something might explode
            data->type_id = PBIO_IODEV_TYPE_ID_LPF2_EXPLOD;
        } else {
            // set ID1 high
            pbdrv_gpio_out_high(&pdata.uart_tx);
            pbdrv_gpio_out_low(&pdata.uart_buf);

            // set ID2 high
            pbdrv_gpio_out_high(&pdata.id2);

            PT_YIELD(pt);

            // if ID2 is high
            if (pbdrv_gpio_input(&pdata.uart_rx) == 1) {
                // set ID2 low
                pbdrv_gpio_out_low(&pdata.id2);

                // There must be some capacitance in the circuit because the
                // uart_rx pin seems to need some extra help being driven low.
                // Otherwise, pbdrv_gpio_input(&pdata.uart_rx) below can
                // sometimes read the wrong value, resulting in improper
                // detection.
                pbdrv_gpio_out_low(&pdata.uart_rx);

                PT_YIELD(pt);

                // if ID2 is low
                if (pbdrv_gpio_input(&pdata.uart_rx) == 0) {
                    if (data->dev_id1_group < 3) {
                        data->type_id = ioport_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_PULL_DOWN];
                    }
                } else {
                    if (data->dev_id1_group < 3) {
                        data->type_id = ioport_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_VCC];
                    }
                }
            } else {
                // we know the device now
                if (data->dev_id1_group < 3) {
                    data->type_id = ioport_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_GND];
                } else {
                    data->type_id = PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART;
                }
            }
        }
    }

    PT_YIELD(pt);

    // set ID2 as input
    pbdrv_gpio_input(&pdata.id2);

    // set ID1 high
    pbdrv_gpio_out_high(&pdata.uart_tx);
    pbdrv_gpio_out_low(&pdata.uart_buf);

    // We don't consider the detected device "affirmative" until we have
    // detected the same device multiple times in a row. Similarly,
    if (data->type_id == data->prev_type_id) {
        if (data->dev_id_match_count < UINT8_MAX) {
            data->dev_id_match_count++;
        }

        if (data->dev_id_match_count >= AFFIRMATIVE_MATCH_COUNT) {
            ioport->connected_type_id = data->type_id;
        }
    } else {
        data->dev_id_match_count = 0;
    }

    data->prev_type_id = data->type_id;

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_ioport_lpf2_process, ev, data) {
    static struct etimer timer;

    PROCESS_EXITHANDLER({
        // make sure all pins are set to input so they aren't supplying power
        // to the I/O device.
        for (int i = 0; i < PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS; i++) {
            init_one(i);
        }

        // TODO: we need to ensure H-bridge power is off here to avoid potentially
        // damaging custom I/O devices.

        // Turn off power on pin 4 on all ports
        // NB: there seems to be a hardware? issue on move/city hubs that if we
        // set this to output low instead of input, the hub will turn back on
        // as soon as the user releases the power button
        pbdrv_gpio_input(&pbdrv_ioport_lpf2_platform_data.port_vcc);

        PROCESS_EXIT();
    });

    PROCESS_BEGIN();

    etimer_set(&timer, 2);

    for (int i = 0; i < PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS; i++) {
        init_one(i);
    }

    // Turn on power on pin 4 on all ports
    pbdrv_gpio_out_high(&pbdrv_ioport_lpf2_platform_data.port_vcc);

    for (;;) {
        PROCESS_WAIT_EVENT();

        // If pbio_uartdev_process tells us the uart device was removed, reset
        // ioport id so the next timer event will take care of resetting the ioport
        if (ev == PROCESS_EVENT_SERVICE_REMOVED) {
            for (int i = 0; i < PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS; i++) {
                ioport_dev_t *ioport = &ioport_devs[i];

                if (ioport->iodev == (pbio_iodev_t *)data) {
                    debug_pr("ioport(%c): Received stop.\n", i + PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT);
                    ioport->connected_type_id = PBIO_IODEV_TYPE_ID_NONE;
                }
            }
        } else if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);

            for (int i = 0; i < PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS; i++) {
                ioport_dev_t *ioport = &ioport_devs[i];

                if (ioport->connected_type_id != PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                    poll_dcm(ioport);
                }

                if (ioport->connected_type_id != ioport->prev_type_id) {
                    debug_pr("ioport(%c): Type changed from %d to %d.\n", i + PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT,
                        ioport->prev_type_id, ioport->connected_type_id);
                    ioport->prev_type_id = ioport->connected_type_id;
                    if (ioport->connected_type_id == PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                        debug_pr("ioport(%c): UART device detected.\n", i + PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT);
                        ioport_enable_uart(ioport);
                        _Static_assert(PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS == PBIO_CONFIG_UARTDEV_NUM_DEV,
                            "code assumes port ID is same as uartdev ID");
                        pbio_uartdev_ready(i);
                        pbio_uartdev_get(i, &ioport->iodev);
                    } else if (ioport->connected_type_id == PBIO_IODEV_TYPE_ID_NONE) {
                        debug_pr("ioport(%c): Device unplugged.\n", i + PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT);
                        ioport->iodev = NULL;
                    } else {
                        debug_pr("ioport(%c): Passive device detected.\n", i + PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT);
                        assert(ioport->connected_type_id < PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART);
                        ioport->iodev = &basic_devs[i];
                        ioport->iodev->info = &basic_infos[ioport->connected_type_id].info;
                    }
                }
            }
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_IOPORT_LPF2
