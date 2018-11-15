
#include <stdbool.h>
#include <stdio.h>

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "pbio/iodev.h"
#include "sys/etimer.h"
#include "sys/process.h"

#include "stm32f070xb.h"

typedef enum _ioport_t {
    IOPORT_C,
    IOPORT_D,
    NUM_IOPORT
} ioport_t;

// Device connection manager state
typedef enum _dcm_state_t {
    DCM_STATE_0,
    DCM_STATE_1,
    DCM_STATE_2,
    DCM_STATE_3,
    DCM_STATE_4,
    DCM_STATE_5,
    DCM_STATE_6,
    DCM_STATE_7,
    DCM_STATE_8,
    DCM_STATE_9,
    DCM_STATE_10,
    DCM_STATE_11,
} dcm_state_t;

typedef enum _dev_id1_group_t {
    DEV_ID1_GROUP_GND,
    DEV_ID1_GROUP_VCC,
    DEV_ID1_GROUP_PULL_DOWN,
    DEV_ID1_GROUP_OPEN,
} dev_id1_group_t;

typedef struct _ioport_gpio_t {
    GPIO_TypeDef *bank;
    const uint8_t bit;
} ioport_gpio_t;

// GPIOs associated with ID1 and ID2 pins
typedef struct _ioport_pins_t {
    const ioport_gpio_t id1;        // pin 5 in
    const ioport_gpio_t id2;        // pin 6 in/out
    const ioport_gpio_t uart_buf;
    const ioport_gpio_t uart_tx;    // pin 5 out
    const ioport_gpio_t uart_rx;    // pin 6 in
} ioport_pins_t;

// Device connection manager state for each port
typedef struct _dcm_data_t {
    dcm_state_t dcm_state;
    dev_id1_group_t dev_id1_group;
    pbio_iodev_type_id_t type_id;
    pbio_iodev_type_id_t prev_type_id;
    uint8_t prev_gpio_value;
    uint8_t dev_id_match_count;
} dcm_data_t;

static dcm_data_t dcm_data[NUM_IOPORT];

static const ioport_pins_t ioport_pins[NUM_IOPORT] = {
    [IOPORT_C] = {
        .id1        = { .bank = GPIOB, .bit = 7  },
        .id2        = { .bank = GPIOC, .bit = 15 },
        .uart_buf   = { .bank = GPIOB, .bit = 4  },
        .uart_tx    = { .bank = GPIOC, .bit = 10 },
        .uart_rx    = { .bank = GPIOC, .bit = 11 },
    },
    [IOPORT_D] = {
        .id1        = { .bank = GPIOB, .bit = 10 },
        .id2        = { .bank = GPIOA, .bit = 12 },
        .uart_buf   = { .bank = GPIOB, .bit = 0  },
        .uart_tx    = { .bank = GPIOC, .bit = 4  },
        .uart_rx    = { .bank = GPIOC, .bit = 5  },
    },
};

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

static pbio_iodev_type_id_t connected_type_id[NUM_IOPORT];
static pbio_iodev_type_id_t prev_type_id[NUM_IOPORT];

static struct {
    pbio_iodev_info_t info;
    pbio_iodev_mode_t modes[PBIO_IODEV_MAX_NUM_MODES];
} ioport_info[NUM_IOPORT];

pbio_iodev_t iodevs[NUM_IOPORT];

PROCESS(pbdrv_ioport_process, "I/O port");

static void ioport_gpio_out_low(const ioport_gpio_t *gpio) {
    gpio->bank->MODER = (gpio->bank->MODER & ~(3 << (gpio->bit * 2))) | (1 << (gpio->bit * 2));
    gpio->bank->BRR = 1 << gpio->bit;
}

static void ioport_gpio_out_high(const ioport_gpio_t *gpio) {
    gpio->bank->MODER = (gpio->bank->MODER & ~(3 << (gpio->bit * 2))) | (1 << (gpio->bit * 2));
    gpio->bank->BSRR = 1 << gpio->bit;
}

static uint8_t ioport_gpio_input(const ioport_gpio_t *gpio) {
    gpio->bank->MODER = (gpio->bank->MODER & ~(3 << (gpio->bit * 2))) | (0 << (gpio->bit * 2));
    return (gpio->bank->IDR >> gpio->bit) & 1;
}

static void ioport_gpio_alt(const ioport_gpio_t *gpio) {
    gpio->bank->MODER = (gpio->bank->MODER & ~(3 << (gpio->bit * 2))) | (2 << (gpio->bit * 2));
}

static void ioport_enable_uart(ioport_t ioport) {
    const ioport_pins_t *pins = &ioport_pins[ioport];

    ioport_gpio_alt(&pins->uart_rx);
    ioport_gpio_alt(&pins->uart_tx);
    ioport_gpio_out_low(&pins->uart_buf);
}

static void init_one(ioport_t ioport) {
    const ioport_pins_t pins = ioport_pins[ioport];

    iodevs[ioport].port = PBIO_PORT_C + ioport;
    iodevs[ioport].info = &ioport_info[ioport].info;

    // set up alternate function for UART pins
    if (ioport == IOPORT_C) {
        GPIOC->AFR[1] = (GPIOC->AFR[1] & ~(GPIO_AFRH_AFSEL10_Msk | GPIO_AFRH_AFSEL11_Msk)) | (0 << GPIO_AFRH_AFSEL10_Pos) | (0 << GPIO_AFRH_AFSEL11_Pos);
    }
    else if (ioport == IOPORT_D) {
        GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(GPIO_AFRL_AFSEL4_Msk | GPIO_AFRL_AFSEL5_Msk)) | (1 << GPIO_AFRL_AFSEL4_Pos) | (1 << GPIO_AFRL_AFSEL5_Pos);
    }

    ioport_gpio_input(&pins.id1);
    ioport_gpio_input(&pins.id2);
    ioport_gpio_input(&pins.uart_buf);
    ioport_gpio_input(&pins.uart_tx);
    ioport_gpio_input(&pins.uart_rx);
}

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    switch (port) {
    case PBIO_PORT_C:
        *iodev = &iodevs[IOPORT_C];
        break;
    case PBIO_PORT_D:
        *iodev = &iodevs[IOPORT_D];
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

// This is the device connection manager (dcm). It monitors the ID1 and ID2 pins
// on the port to see when devices are connected or disconnected.
// It is expected for there to be a 2ms delay between calls to this function.
static void poll_dcm(ioport_t ioport) {
    // copying the data struct reduces the code size by a few hundred bytes,
    // but we need to rember to copy it back if we change anything
    dcm_data_t data = dcm_data[ioport];
    const ioport_pins_t pins = ioport_pins[ioport];
    uint8_t gpio_input;

    switch (data.dcm_state) {
    case DCM_STATE_0:
        data.type_id = PBIO_IODEV_TYPE_ID_NONE;
        data.dev_id1_group = DEV_ID1_GROUP_OPEN;

        // set ID1 high
        ioport_gpio_out_high(&pins.uart_tx);
        ioport_gpio_out_low(&pins.uart_buf);

        // set ID2 as input
        ioport_gpio_input(&pins.id2);

        data.dcm_state = DCM_STATE_1;
        break;
    case DCM_STATE_1:
        // save current ID2 value
        data.prev_gpio_value = ioport_gpio_input(&pins.id2);

        // set ID1 low
        ioport_gpio_out_low(&pins.uart_tx);

        data.dcm_state = DCM_STATE_2;
        break;
    case DCM_STATE_2:
        // read ID2
        gpio_input = ioport_gpio_input(&pins.id2);

        // if ID2 changed from high to low
        if (data.prev_gpio_value == 1 && gpio_input == 0) {
            // we have touch sensor
            data.type_id = PBIO_IODEV_TYPE_ID_LPF2_TOUCH;

            // set ID1 as input
            ioport_gpio_out_high(&pins.uart_buf);
            ioport_gpio_input(&pins.uart_tx);

            data.dcm_state = DCM_STATE_3;
        }
        // if ID2 changed from low to high
        else if (data.prev_gpio_value == 0 && gpio_input == 1) {
            data.type_id = PBIO_IODEV_TYPE_ID_LPF2_TPOINT;

            data.dcm_state = DCM_STATE_11;
        }
        else {
            // read ID1
            data.prev_gpio_value = ioport_gpio_input(&pins.id1);

            // set ID1 high
            ioport_gpio_out_high(&pins.uart_tx);

            data.dcm_state = DCM_STATE_4;
        }
        break;
    case DCM_STATE_3:
        // ID1 is inverse of touch sensor value
        // TODO: save this value to sensor data
        //sensor_data = !ioport_gpio_input(&pins.id1);

        data.dcm_state = DCM_STATE_11;
        break;
    case DCM_STATE_4:
        // read ID1
        gpio_input = ioport_gpio_input(&pins.id1);

        // if ID1 did not change and is high
        if (data.prev_gpio_value == 1 && gpio_input == 1) {
            // we have ID1 == VCC
            data.dev_id1_group = DEV_ID1_GROUP_VCC;

            data.dcm_state = DCM_STATE_6;
        }
        // if ID1 did not change and is low
        else if (data.prev_gpio_value == 0 && gpio_input == 0) {
            // we have ID1 == GND
            data.dev_id1_group = DEV_ID1_GROUP_GND;

            data.dcm_state = DCM_STATE_6;
        }
        else {
            // set ID1 as input
            ioport_gpio_out_high(&pins.uart_buf);
            ioport_gpio_input(&pins.uart_tx);

            data.dcm_state = DCM_STATE_5;
        }
        break;
    case DCM_STATE_5:
        // read ID1
        if (ioport_gpio_input(&pins.id1) == 1) {
            // we have ID1 == open
            data.dev_id1_group = DEV_ID1_GROUP_OPEN;
        }
        else {
            // we have ID1 == pull down
            data.dev_id1_group = DEV_ID1_GROUP_PULL_DOWN;
        }

        data.dcm_state = DCM_STATE_6;
        break;
    case DCM_STATE_6:
        // set ID1 as input
        ioport_gpio_out_high(&pins.uart_buf);
        ioport_gpio_input(&pins.uart_tx);

        // set ID2 high
        ioport_gpio_out_high(&pins.id2);

        data.dcm_state = DCM_STATE_7;
        break;
    case DCM_STATE_7:
        // read ID1
        data.prev_gpio_value = ioport_gpio_input(&pins.id1);

        // set ID2 low
        ioport_gpio_out_low(&pins.id2);

        data.dcm_state = DCM_STATE_8;
        break;
    case DCM_STATE_8:
        // read ID1
        gpio_input = ioport_gpio_input(&pins.id1);

        // if ID1 changed from high to low
        if (data.prev_gpio_value == 1 && gpio_input == 0) {
            // if we have ID1 = open
            if (data.dev_id1_group == DEV_ID1_GROUP_OPEN) {
                // then we have this
                data.type_id = PBIO_IODEV_TYPE_ID_LPF2_3_PART;
            }

            data.dcm_state = DCM_STATE_11;
        }
        // if ID1 changed from low to high
        else if (data.prev_gpio_value == 0 && gpio_input == 1) {
            // something might explode
            data.type_id = PBIO_IODEV_TYPE_ID_LPF2_EXPLOD;

            data.dcm_state = DCM_STATE_11;
        }
        else {
            // set ID1 high
            ioport_gpio_out_high(&pins.uart_tx);
            ioport_gpio_out_low(&pins.uart_buf);

            // set ID2 high
            ioport_gpio_out_high(&pins.id2);

            data.dcm_state = DCM_STATE_9;
        }
        break;
    case DCM_STATE_9:
        // if ID2 is high
        if (ioport_gpio_input(&pins.uart_rx) == 1) {
            // set ID2 low
            ioport_gpio_out_low(&pins.id2);

            data.dcm_state = DCM_STATE_10;
        }
        else {
            // we know the device now
            if (data.dev_id1_group < 3) {
                data.type_id = ioport_type_id_lookup[data.dev_id1_group][0];
            }
            else {
                data.type_id = PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART;
            }

            data.dcm_state = DCM_STATE_11;
        }
        break;
    case DCM_STATE_10:
        // if ID2 is low
        if (ioport_gpio_input(&pins.uart_rx) == 0) {
            if (data.dev_id1_group < 3) {
                data.type_id = ioport_type_id_lookup[data.dev_id1_group][2];
            }
        }
        else {
            if (data.dev_id1_group < 3) {
                data.type_id = ioport_type_id_lookup[data.dev_id1_group][1];
            }
        }

        data.dcm_state = DCM_STATE_11;
        break;
    case DCM_STATE_11:
        // set ID2 as input
        ioport_gpio_input(&pins.id2);

        // set ID1 low
        ioport_gpio_out_low(&pins.uart_tx);
        ioport_gpio_out_low(&pins.uart_buf);

        if (data.type_id == data.prev_type_id) {
            if (++data.dev_id_match_count >= 20) {

                if (data.type_id != connected_type_id[ioport]) {
                    connected_type_id[ioport] = data.type_id;
                }

                // don't want to wrap around and re-trigger
                data.dev_id_match_count--;
            }
        }

        data.prev_type_id = data.type_id;

        data.dcm_state = DCM_STATE_0;
        break;
    }

    // copy local variable back to global
    dcm_data[ioport] = data;
}

PROCESS_THREAD(pbdrv_ioport_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    etimer_set(&timer, clock_from_msec(2));

    init_one(IOPORT_C);
    init_one(IOPORT_D);

    while (true) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        etimer_reset(&timer);

        if (connected_type_id[IOPORT_C] != PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
            poll_dcm(IOPORT_C);
        }

        if (connected_type_id[IOPORT_C] != prev_type_id[IOPORT_C]) {
            prev_type_id[IOPORT_C] = connected_type_id[IOPORT_C];
            if (connected_type_id[IOPORT_C] == PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                ioport_enable_uart(IOPORT_C);
            }
        }

        if (connected_type_id[IOPORT_D] != PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
            poll_dcm(IOPORT_D);
        }

        if (connected_type_id[IOPORT_D] != prev_type_id[IOPORT_D]) {
            prev_type_id[IOPORT_D] = connected_type_id[IOPORT_D];
            if (connected_type_id[IOPORT_D] == PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                ioport_enable_uart(IOPORT_D);
            }
        }
    }

    PROCESS_END();
}
