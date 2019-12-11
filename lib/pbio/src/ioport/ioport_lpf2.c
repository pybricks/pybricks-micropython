// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner

// LEGO Power Functions 2 I/O port

#include <pbio/config.h>

#if PBIO_CONFIG_IOPORT_LPF2

#include <stdbool.h>
#include <stdio.h>

#include "pbdrv/config.h"
#include "pbdrv/gpio.h"
#include "pbio/error.h"
#include "pbio/iodev.h"
#include "sys/etimer.h"
#include "sys/process.h"

typedef enum _dev_id1_group_t {
    DEV_ID1_GROUP_GND,
    DEV_ID1_GROUP_VCC,
    DEV_ID1_GROUP_PULL_DOWN,
    DEV_ID1_GROUP_OPEN,
} dev_id1_group_t;

// GPIOs associated with ID1 and ID2 pins
typedef struct _pbdrv_ioport_pins_t {
    const pbdrv_gpio_t id1;        // pin 5 in
    const pbdrv_gpio_t id2;        // pin 6 in/out
    const pbdrv_gpio_t uart_buf;
    const pbdrv_gpio_t uart_tx;    // pin 5 out
    const pbdrv_gpio_t uart_rx;    // pin 6 in
    const uint8_t alt;
} pbdrv_ioport_pins_t;

// Device connection manager state for each port
typedef struct _dcm_data_t {
    dev_id1_group_t dev_id1_group;
    pbio_iodev_type_id_t type_id;
    pbio_iodev_type_id_t prev_type_id;
    uint8_t prev_gpio_value;
    uint8_t dev_id_match_count;
} dcm_data_t;

typedef struct {
    void (*init)(const pbdrv_ioport_pins_t *pins);
    void (*gpio_out_low)(const pbdrv_gpio_t *gpio);
    void (*gpio_out_high)(const pbdrv_gpio_t *gpio);
    uint8_t (*gpio_input)(const pbdrv_gpio_t *gpio);
    void (*mux_uart)(const pbdrv_gpio_t *gpio);
} pbio_ioport_lpf2_funcs_t;

typedef struct {
    const pbio_ioport_lpf2_funcs_t *funcs;
    const pbdrv_ioport_pins_t *pins;
    dcm_data_t dcm;
    struct pt pt;
    pbio_iodev_type_id_t connected_type_id;
    pbio_iodev_type_id_t prev_type_id;
} ioport_dev_t;

static const pbio_iodev_type_id_t ioport_type_id_lookup[3][3] = {
    [DEV_ID1_GROUP_GND] = {
        [0] = PBIO_IODEV_TYPE_ID_LPF2_POWER,
        [1] = PBIO_IODEV_TYPE_ID_LPF2_TURN,
        [2] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT2,
    },
    [DEV_ID1_GROUP_VCC] = {
        [0] = PBIO_IODEV_TYPE_ID_LPF2_TRAIN,
        [1] = PBIO_IODEV_TYPE_ID_LPF2_LMOTOR,
        [2] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT1,
    },
    [DEV_ID1_GROUP_PULL_DOWN] = {
        [0] = PBIO_IODEV_TYPE_ID_LPF2_MMOTOR,
        [1] = PBIO_IODEV_TYPE_ID_LPF2_XMOTOR,
        [2] = PBIO_IODEV_TYPE_ID_LPF2_LIGHT,
    },
};

static struct {
    pbio_iodev_info_t info;
    pbio_iodev_mode_t modes[PBIO_IODEV_MAX_NUM_MODES];
} ioport_info[PBDRV_CONFIG_NUM_IO_PORT];

pbio_iodev_t iodevs[PBDRV_CONFIG_NUM_IO_PORT];

PROCESS(pbio_ioport_lpf2_process, "I/O port");

static void ioport_init(const pbdrv_ioport_pins_t *pins) {
    pbdrv_gpio_alt(&pins->uart_tx, pins->alt);
    pbdrv_gpio_alt(&pins->uart_rx, pins->alt);
}

static const pbio_ioport_lpf2_funcs_t ioport_funcs = {
    .init = ioport_init,
    .gpio_out_low = ioport_gpio_out_low,
    .gpio_out_high = ioport_gpio_out_high,
    .gpio_input = ioport_gpio_input,
    .mux_uart = ioport_gpio_uart,
};

static ioport_dev_t ioport_devs[PBDRV_CONFIG_NUM_IO_PORT] = {
    [0] = { // Port C - USART4
        .funcs = &ioport_funcs,
        .pins = &(const pbdrv_ioport_pins_t){
            .id1        = { .bank = GPIOB, .pin = 7  },
            .id2        = { .bank = GPIOC, .pin = 15 },
            .uart_buf   = { .bank = GPIOB, .pin = 4  },
            .uart_tx    = { .bank = GPIOC, .pin = 10 },
            .uart_rx    = { .bank = GPIOC, .pin = 11 },
            .alt        = 0,
        },
    },
    [1] = { // Port D - USART3
        .funcs = &ioport_funcs,
        .pins = &(const pbdrv_ioport_pins_t){
            .id1        = { .bank = GPIOB, .pin = 10 },
            .id2        = { .bank = GPIOA, .pin = 12 },
            .uart_buf   = { .bank = GPIOB, .pin = 0  },
            .uart_tx    = { .bank = GPIOC, .pin = 4  },
            .uart_rx    = { .bank = GPIOC, .pin = 5  },
            .alt        = 1,
        },
    },
};

static void ioport_enable_uart(ioport_dev_t *ioport) {
    const pbdrv_ioport_pins_t *pins = ioport->pins;
    const pbio_ioport_lpf2_funcs_t *funcs = ioport->funcs;

    funcs->mux_uart(&pins->uart_rx);
    funcs->mux_uart(&pins->uart_tx);
    funcs->gpio_out_low(&pins->uart_buf);
}

static void init_one(uint8_t ioport) {
    const pbdrv_ioport_pins_t *pins = ioport_devs[ioport].pins;
    const pbio_ioport_lpf2_funcs_t funcs = *ioport_devs[ioport].funcs;

    PT_INIT(&ioport_devs[ioport].pt);

    iodevs[ioport].port = PBDRV_CONFIG_FIRST_IO_PORT + ioport;
    iodevs[ioport].info = &ioport_info[ioport].info;

    funcs.init(pins);
    funcs.gpio_input(&pins->id1);
    funcs.gpio_input(&pins->id2);
    funcs.gpio_input(&pins->uart_buf);
    funcs.gpio_input(&pins->uart_tx);
    funcs.gpio_input(&pins->uart_rx);
}

pbio_error_t pbio_ioport_lpf2_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    if (port < PBDRV_CONFIG_FIRST_IO_PORT || port > PBDRV_CONFIG_LAST_IO_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *iodev = &iodevs[port - PBDRV_CONFIG_FIRST_IO_PORT];

    return PBIO_SUCCESS;
}

// This is the device connection manager (dcm). It monitors the ID1 and ID2 pins
// on the port to see when devices are connected or disconnected.
// It is expected for there to be a 2ms delay between calls to this function.
static PT_THREAD(poll_dcm(ioport_dev_t *ioport)) {
    struct pt *pt = &ioport->pt;
    dcm_data_t *data = &ioport->dcm;
    const pbdrv_ioport_pins_t pins = *ioport->pins;
    const pbio_ioport_lpf2_funcs_t funcs = *ioport->funcs;
    uint8_t gpio_input;

    PT_BEGIN(pt);

    data->type_id = PBIO_IODEV_TYPE_ID_NONE;
    data->dev_id1_group = DEV_ID1_GROUP_OPEN;

    // set ID1 high
    funcs.gpio_out_high(&pins.uart_tx);
    funcs.gpio_out_low(&pins.uart_buf);

    // set ID2 as input
    funcs.gpio_input(&pins.id2);

    PT_YIELD(pt);

    // save current ID2 value
    data->prev_gpio_value = funcs.gpio_input(&pins.id2);

    // set ID1 low
    funcs.gpio_out_low(&pins.uart_tx);

    PT_YIELD(pt);

    // read ID2
    gpio_input = funcs.gpio_input(&pins.id2);

    // if ID2 changed from high to low
    if (data->prev_gpio_value == 1 && gpio_input == 0) {
        // we have touch sensor
        data->type_id = PBIO_IODEV_TYPE_ID_LPF2_TOUCH;

        // set ID1 as input
        funcs.gpio_out_high(&pins.uart_buf);
        funcs.gpio_input(&pins.uart_tx);

        PT_YIELD(pt);

        // ID1 is inverse of touch sensor value
        // TODO: save this value to sensor data
        //sensor_data = !funcs.gpio_input(&pins.id1);
    }
    // if ID2 changed from low to high
    else if (data->prev_gpio_value == 0 && gpio_input == 1) {
        data->type_id = PBIO_IODEV_TYPE_ID_LPF2_TPOINT;
    }
    else {
        // read ID1
        data->prev_gpio_value = funcs.gpio_input(&pins.id1);

        // set ID1 high
        funcs.gpio_out_high(&pins.uart_tx);

        PT_YIELD(pt);

        // read ID1
        gpio_input = funcs.gpio_input(&pins.id1);

        // if ID1 did not change and is high
        if (data->prev_gpio_value == 1 && gpio_input == 1) {
            // we have ID1 == VCC
            data->dev_id1_group = DEV_ID1_GROUP_VCC;
        }
        // if ID1 did not change and is low
        else if (data->prev_gpio_value == 0 && gpio_input == 0) {
            // we have ID1 == GND
            data->dev_id1_group = DEV_ID1_GROUP_GND;
        }
        else {
            // set ID1 as input
            funcs.gpio_out_high(&pins.uart_buf);
            funcs.gpio_input(&pins.uart_tx);

            PT_YIELD(pt);

            // read ID1
            if (funcs.gpio_input(&pins.id1) == 1) {
                // we have ID1 == open
                data->dev_id1_group = DEV_ID1_GROUP_OPEN;
            }
            else {
                // we have ID1 == pull down
                data->dev_id1_group = DEV_ID1_GROUP_PULL_DOWN;
            }
        }

        PT_YIELD(pt);

        // set ID1 as input
        funcs.gpio_out_high(&pins.uart_buf);
        funcs.gpio_input(&pins.uart_tx);

        // set ID2 high
        funcs.gpio_out_high(&pins.id2);

        PT_YIELD(pt);

        // read ID1
        data->prev_gpio_value = funcs.gpio_input(&pins.id1);

        // set ID2 low
        funcs.gpio_out_low(&pins.id2);

        PT_YIELD(pt);

        // read ID1
        gpio_input = funcs.gpio_input(&pins.id1);

        // if ID1 changed from high to low
        if (data->prev_gpio_value == 1 && gpio_input == 0) {
            // if we have ID1 = open
            if (data->dev_id1_group == DEV_ID1_GROUP_OPEN) {
                // then we have this
                data->type_id = PBIO_IODEV_TYPE_ID_LPF2_3_PART;
            }
        }
        // if ID1 changed from low to high
        else if (data->prev_gpio_value == 0 && gpio_input == 1) {
            // something might explode
            data->type_id = PBIO_IODEV_TYPE_ID_LPF2_EXPLOD;
        }
        else {
            // set ID1 high
            funcs.gpio_out_high(&pins.uart_tx);
            funcs.gpio_out_low(&pins.uart_buf);

            // set ID2 high
            funcs.gpio_out_high(&pins.id2);

            PT_YIELD(pt);

            // if ID2 is high
            if (funcs.gpio_input(&pins.uart_rx) == 1) {
                // set ID2 low
                funcs.gpio_out_low(&pins.id2);

                PT_YIELD(pt);

                // if ID2 is low
                if (funcs.gpio_input(&pins.uart_rx) == 0) {
                    if (data->dev_id1_group < 3) {
                        data->type_id = ioport_type_id_lookup[data->dev_id1_group][2];
                    }
                }
                else {
                    if (data->dev_id1_group < 3) {
                        data->type_id = ioport_type_id_lookup[data->dev_id1_group][1];
                    }
                }
            }
            else {
                // we know the device now
                if (data->dev_id1_group < 3) {
                    data->type_id = ioport_type_id_lookup[data->dev_id1_group][0];
                }
                else {
                    data->type_id = PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART;
                }
            }
        }
    }

    PT_YIELD(pt);

    // set ID2 as input
    funcs.gpio_input(&pins.id2);

    // set ID1 low
    funcs.gpio_out_low(&pins.uart_tx);
    funcs.gpio_out_low(&pins.uart_buf);

    if (data->type_id == data->prev_type_id) {
        if (++data->dev_id_match_count >= 20) {

            if (data->type_id != ioport->connected_type_id) {
                ioport->connected_type_id = data->type_id;
            }

            // don't want to wrap around and re-trigger
            data->dev_id_match_count--;
        }
    }

    data->prev_type_id = data->type_id;

    PT_END(pt);
}

PROCESS_THREAD(pbio_ioport_lpf2_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    etimer_set(&timer, clock_from_msec(2));

    for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
        init_one(i);
    }

    while (true) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        etimer_reset(&timer);

        for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
            ioport_dev_t *ioport = &ioport_devs[i];

            if (ioport->connected_type_id != PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                poll_dcm(ioport);
            }

            if (ioport->connected_type_id != ioport->prev_type_id) {
                ioport->prev_type_id = ioport->connected_type_id;
                if (ioport->connected_type_id == PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                    ioport_enable_uart(ioport);
                }
            }
        }
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_IOPORT_LPF2
