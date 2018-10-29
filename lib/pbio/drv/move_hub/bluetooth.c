
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sys/autostart.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "sys/pt.h"

#include "bluenrg_aci.h"
#include "bluenrg_gap.h"
#include "hci_le.h"
#include "hci_tl.h"
#include "stm32f070xb.h"

// name used for standard GAP device name characteristic
#define DEV_NAME "PyBricks Hub"

// bluetooth address is set at factory at this address
#define FLASH_BD_ADDR ((const uint8_t *)0x08004ffa)

// size of the BlueNRG SPI header
#define BLUENRG_HEADER_SIZE 5
// value returned in READY byte of BlueNRG SPI header when interface is ready
#define BLUENRG_READY 2

// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20


typedef void(*gatt_attr_modified_handler_t)(void *data, uint8_t size);

// array to hold GATT attribute write message callbacks. The index of the array
// is the attribute handle, so we need to make sure this is big enough if we
// add more attributes
static gatt_attr_modified_handler_t gatt_attr_modified_handler[32];

// BlueNRG header data for SPI write xfer
static const uint8_t write_header_tx[BLUENRG_HEADER_SIZE] = { 0x0a };
// BlueNRG header data for SPI read xfer
static const uint8_t read_header_tx[BLUENRG_HEADER_SIZE] = { 0x0b };
// Rx buffer for BlueNRG header
static uint8_t header_rx[BLUENRG_HEADER_SIZE];

// Tx buffer for SPI writes
static uint8_t write_buf[HCI_MAX_PAYLOAD_SIZE];
// Rx buffer for SPI reads
static uint8_t read_buf[HCI_MAX_PAYLOAD_SIZE];
// Dummy Tx data for read xfers
static const uint8_t dummy_write_buf[1] = { 0xff };
// Dummy Rx data for write xfers
static uint8_t dummy_read_buf[1];

// size of current SPI xfer Tx data (not including BlueNRG read/write header)
// value is set to 0 when Tx is complete
static uint8_t write_xfer_size;

// reflects state of SPI_IRQ pin
volatile bool spi_irq = false;
// set to false when xfer is started and true when xfer is complete
volatile bool spi_xfer_complete = false;

// set to false when hci command is started and true when command is completed
static bool hci_command_complete = false;
// handle to connected Bluetooth device
static uint16_t conn_handle;

// nRF UART service handles
static uint16_t uart_service_handle, uart_rx_char_handle, uart_tx_char_handle;


PROCESS(pbdrv_bluetooth_hci_process, "Bluetooth HCI");
PROCESS(pbdrv_bluetooth_spi_process, "Bluetooth SPI");

void _pbdrv_bluetooth_init(void) {
    // put Bluetooth chip into reset

    // nRESET
    // set PB6 output low
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
    GPIOB->BRR = GPIO_BRR_BR_6;

    // SPI2 pin mux

    // SPI_CS
    // set PB12 as gpio output high
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER12_Msk) | (1 << GPIO_MODER_MODER12_Pos);
    GPIOB->BSRR = GPIO_BSRR_BS_12;

    // SPI_IRQ
    // set PD2 as gpio input with pull-down
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER2_Msk) | (0 << GPIO_MODER_MODER2_Pos);
    GPIOD->PUPDR = (GPIOD->PUPDR & ~GPIO_PUPDR_PUPDR2_Msk) | (2 << GPIO_PUPDR_PUPDR2_Pos);

    // SPI_MOSI
    // set PC3 as SPI2->MOSI
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER3_Msk) | (2 << GPIO_MODER_MODER3_Pos);
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL3_Msk) | (1 << GPIO_AFRL_AFSEL3_Pos);

    // SPI_MISO
    // set PC2 as SPI2->MISO
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER2_Msk) | (2 << GPIO_MODER_MODER2_Pos);
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL2_Msk) | (1 << GPIO_AFRL_AFSEL2_Pos);

    // SPI_SCK
    // set PB13 as SPI2->CLK
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER13_Msk) | (2 << GPIO_MODER_MODER13_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL13_Msk) | (0 << GPIO_AFRH_AFSEL13_Pos);

    // DMA

    // enable DMA power domain
    RCC->AHBENR |= RCC_AHBENR_DMAEN;

    // TODO: most of this can be deleted except CPAR
    DMA1_Channel4->CPAR = (uint32_t)&SPI2->DR;
    DMA1_Channel5->CPAR = (uint32_t)&SPI2->DR;

    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 4);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

    // SPI2

    // enable SPI2 power domain
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    // set as master and clock /8
    #define SPI_CR1_BR_DIV8 SPI_CR1_BR_1
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV8 | SPI_CR1_SSM;
    // enable DMA rx/tx, chip select (even though it is unused?), rx not empty irq, 8-bit word size, trigger rx irq on 8-bit
    #define SPI_CR2_DS_8BIT (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
    SPI2->CR2 = SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN | SPI_CR2_SSOE | SPI_CR2_RXNEIE | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;

    // LEGO Firmware doesn't do this, but we actually use IRQ for SPI_IRQ pin
    // this is needed since we use __WFI() sometimes
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PD;
    EXTI->IMR |= EXTI_EMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_RT2;
    EXTI->FTSR |= EXTI_FTSR_FT2;
    NVIC_SetPriority(EXTI2_3_IRQn, 128);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
}

// overrides weak function in start_*.S
void DMA1_Channel4_5_IRQHandler(void) {
    // if CH4 transfer complete
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        // clear interrupt
        DMA1->IFCR |= DMA_ISR_TCIF4;
        // disable CH4
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;

        // notify that SPI xfer is complete
        spi_xfer_complete = true;
        process_poll(&pbdrv_bluetooth_spi_process);
    }
}

// overrides weak function in startup_*.S
void EXTI2_3_IRQHandler(void) {
    spi_irq = !!(GPIOD->IDR & GPIO_IDR_2);

    // clear the interrupt
    EXTI->PR = EXTI_PR_PR2;

    process_poll(&pbdrv_bluetooth_spi_process);
}

static inline void spi_enable_cs() {
    GPIOB->BRR = GPIO_BRR_BR_12;
}

static inline void spi_disable_cs() {
    GPIOB->BSRR = GPIO_BSRR_BS_12;
}

// configures and starts an SPI xfer
static void spi_start_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint8_t xfer_size) {
    // reset the xfer complete flag
    spi_xfer_complete = false;

    // hopefully this shouldn't actually block, but we can't disable SPI while
    // it is busy, so just in case...
    while (SPI2->SR & SPI_SR_BSY) { }

    // disable the SPI so we can configure it
    SPI2->CR1 &= ~SPI_CR1_SPE;

    // configure DMA
    DMA1_Channel5->CCR = 0;
    DMA1_Channel4->CCR = 0;
    DMA1_Channel5->CMAR = (uint32_t)tx_buf;
    DMA1_Channel5->CNDTR = xfer_size;
    DMA1_Channel4->CMAR = (uint32_t)rx_buf;
    DMA1_Channel4->CNDTR = xfer_size;
    // dummy bufs are only one byte, so don't enable memory increment for them
    DMA1_Channel4->CCR = (rx_buf == dummy_read_buf ? 0 : DMA_CCR_MINC) | DMA_CCR_TCIE | DMA_CCR_EN;
    DMA1_Channel5->CCR = (tx_buf == dummy_write_buf ? 0 : DMA_CCR_MINC) | DMA_CCR_DIR | DMA_CCR_EN;

    // enable SPI to start the xfer
    SPI2->CR1 |= SPI_CR1_SPE;
}

// gets the available tx and rx buffer sizes returned in the BlueNRG SPI header
// returns true if the header is valid, otherwise false
static bool get_bluenrg_buf_size(uint8_t *wbuf, uint8_t *rbuf) {
    if (header_rx[0] != BLUENRG_READY) {
        return false;
    }

    *wbuf = header_rx[1];
    *rbuf = header_rx[3];

    return true;
}

// assigned to one of RESET_* from bluenrg_hal_aci.h
static uint8_t reset_reason;

// processes an event received from the Bluetooth chip
static void handle_event(hci_event_pckt *event) {
    switch (event->evt) {
    case EVT_DISCONN_COMPLETE:
        {
            evt_disconn_complete *evt = (evt_disconn_complete *)event->data;
            if (conn_handle == evt->handle) {
                conn_handle = 0;
            }
        }
        break;
    case EVT_CMD_COMPLETE:
        hci_command_complete = true;
        break;
    case EVT_LE_META_EVENT:
        {
            evt_le_meta_event *evt = (evt_le_meta_event *)event->data;
            switch (evt->subevent) {
            case EVT_LE_CONN_COMPLETE:
                {
                    evt_le_connection_complete *subevt = (evt_le_connection_complete *)evt->data;
                    conn_handle = subevt->handle;
                }
                break;
            }
        }
        break;
    case EVT_VENDOR:
        {
            evt_blue_aci *evt = (evt_blue_aci *)event->data;
            switch (evt->ecode) {
            case EVT_BLUE_HAL_INITIALIZED:
                {
                    evt_hal_initialized *subevt = (evt_hal_initialized *)evt->data;
                    reset_reason = subevt->reason_code;
                }
                break;
            case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
                {
                    evt_gatt_attr_modified *subevt = (evt_gatt_attr_modified *)evt->data;
                    if (gatt_attr_modified_handler[subevt->attr_handle]) {
                        gatt_attr_modified_handler[subevt->attr_handle](subevt->att_data, subevt->data_length);
                    }
                }
                break;
            }
        }
        break;
    }

    process_post_synch(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_NONE, NULL);
}

// read message from BlueNRG chip
static PT_THREAD(spi_read(struct pt *pt)) {
    hci_uart_pckt *pckt = (hci_uart_pckt *)read_buf;
    uint8_t wbuf, rbuf;

    PT_BEGIN(pt);

retry:
    if (!spi_irq) {
        // if SPI_IRQ went away, reading will fail, so don't try
        goto end;
    }

    spi_enable_cs();

    // send the read header
    spi_start_xfer(read_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    // keep retrying until the chip is ready to read
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || rbuf == 0) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // read the message
    spi_start_xfer(dummy_write_buf, read_buf, rbuf);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    spi_disable_cs();

    // handle the received data (note: can't use switch statement here because
    // we are using protothreads)
    if (pckt->type == HCI_EVENT_PKT) {
        handle_event((hci_event_pckt *)pckt->data);
    }
    // TODO: do we need to handle ACL packets (HCI_ACLDATA_PKT)?

end: ;
    PT_END(pt);
}

// write message to BlueNRG chip
static PT_THREAD(spi_write(struct pt *pt)) {
    uint8_t wbuf, rbuf;

    PT_BEGIN(pt);

retry:
    spi_enable_cs();

    // send the write header
    spi_start_xfer(write_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    // keep retrying until the chip is ready to write
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || wbuf < write_xfer_size) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // write the message
    spi_start_xfer(write_buf, dummy_read_buf, write_xfer_size);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    spi_disable_cs();

    // set to 0 to indicate that xfer is complete
    write_xfer_size = 0;
    process_post_synch(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_NONE, NULL);

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_bluetooth_spi_process, ev, data) {
    static struct pt child_pt;

    PROCESS_BEGIN();

    while (1) {
        PROCESS_WAIT_UNTIL(spi_irq || write_xfer_size);
        // if there is a pending read message
        if (spi_irq) {
            PROCESS_PT_SPAWN(&child_pt, spi_read(&child_pt));
        }
        // if there is a pending write message
        else if (write_xfer_size) {
            PROCESS_PT_SPAWN(&child_pt, spi_write(&child_pt));
        }
    }

    PROCESS_END();
}

// implements function for BlueNRG library
void hci_send_req(struct hci_request *r) {
    hci_uart_pckt *pckt = (hci_uart_pckt *)write_buf;
    hci_command_hdr *hdr = (hci_command_hdr *)pckt->data;
    void *cmd = &write_buf[HCI_HDR_SIZE + HCI_COMMAND_HDR_SIZE];

    pckt->type = HCI_COMMAND_PKT;
    hdr->opcode = r->opcode;
    hdr->plen = r->clen;
    memcpy(cmd, r->cparam, r->clen);
    write_xfer_size = HCI_HDR_SIZE + HCI_COMMAND_HDR_SIZE + r->clen;

    hci_command_complete = false;
    process_post_synch(&pbdrv_bluetooth_spi_process, PROCESS_EVENT_NONE, NULL);
}

// implements function for BlueNRG library
void hci_recv_resp(struct hci_response *r) {
    // TODO: might be a good idea to make sure opcodes match
    memcpy(r->rparam, &read_buf[HCI_HDR_SIZE + HCI_EVENT_HDR_SIZE + EVT_CMD_COMPLETE_SIZE], r->rlen);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static PT_THREAD(hci_init(struct pt *pt)) {
    static const uint8_t mode = 2; // Slave and master; Only one connection; 12 KB of RAM retention
    static uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
    uint8_t bd_addr[6];

    PT_BEGIN(pt);

    // set the mode

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_hal_write_config_data_begin(CONFIG_DATA_MODE_OFFSET, CONFIG_DATA_MODE_LEN, &mode);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_write_config_data_end();

    // set the Bluetooth address

    PT_WAIT_WHILE(pt, write_xfer_size);
    // has to be in little-endian format, but stored in big-endian in flash
    bd_addr[0] = FLASH_BD_ADDR[5];
    bd_addr[1] = FLASH_BD_ADDR[4];
    bd_addr[2] = FLASH_BD_ADDR[3];
    bd_addr[3] = FLASH_BD_ADDR[2];
    bd_addr[4] = FLASH_BD_ADDR[1];
    bd_addr[5] = FLASH_BD_ADDR[0];
    aci_hal_write_config_data_begin(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_addr);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_write_config_data_end();

    // set Tx power level

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_hal_set_tx_power_level_begin(1, 5); // 1.4 dBm - same as LEGO firmware
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_set_tx_power_level_end();

    // init GATT layer

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_init_begin();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_init_end();

    // init GATT layer

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gap_init_begin(GAP_PERIPHERAL_ROLE, PRIVACY_DISABLED, 16); // 16 comes from LEGO bootloader
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gap_init_end(&gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);

    // set the device name

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_update_char_value_begin(gap_service_handle, gap_dev_name_char_handle,
        0, strlen(DEV_NAME), DEV_NAME);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_update_char_value_end();

    PT_END(pt);
}

static void uart_rx_char_modified(void *data, uint8_t size) {
    uint8_t *rx_data = data;

    printf("UART Rx:");
    for (int i = 0; i < size; i++) {
        printf(" %02x", rx_data[i]);
    }
    printf("\n");
}

static PT_THREAD(init_uart_service(struct pt *pt)) {
    // using well-known (but not standard) nRF UART UUIDs

    // 6e400001-b5a3-f393-e0a-9e50e24dcca9e
    static const uint8_t nrf_uart_service_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e
    };
    // 6e400002-b5a3-f393-e0a9-e50e24dcca9e
    static const uint8_t nrf_uart_rx_char_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e
    };
    // 6e400002-b5a3-f393-e0a9-e50e24dcca9e
    static const uint8_t nrf_uart_tx_char_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e
    };

    PT_BEGIN(pt);

    // add the nRF UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_128, nrf_uart_service_uuid, PRIMARY_SERVICE, 7);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&uart_service_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_rx_char_uuid,
        NRF_CHAR_SIZE, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE,
        GATT_NOTIFY_ATTRIBUTE_WRITE, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&uart_rx_char_handle);

    gatt_attr_modified_handler[uart_rx_char_handle + 1] = uart_rx_char_modified;

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_tx_char_uuid,
        NRF_CHAR_SIZE, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&uart_tx_char_handle);

    PT_END(pt);
}

static PT_THREAD(set_discoverable(struct pt *pt)) {
    char local_name[16];

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    hci_le_set_scan_resp_data_begin(0, NULL);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    hci_le_set_scan_resp_data_end();

    // TODO probably want to call hci_le_set_advertising_data() here to advertise UART service

    PT_WAIT_WHILE(pt, write_xfer_size);
    local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
    strcpy(&local_name[1], DEV_NAME);
    aci_gap_set_discoverable_begin(ADV_IND, 0, 0, PUBLIC_ADDR, NO_WHITE_LIST_USE,
        1 + strlen(DEV_NAME), local_name, 0, NULL, 0, 0);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gap_set_discoverable_end();

    PT_END(pt);
}

static PT_THREAD(uart_service_send_data(struct pt *pt, const char *data, uint8_t size))
{
    static uint8_t i, n;
    tBleStatus ret;

    PT_BEGIN(pt);

    // send data in NRF_CHAR_SIZE sized chunks
    for (i = 0; i < size; i += NRF_CHAR_SIZE) {
        n = size - i;
        if (n > NRF_CHAR_SIZE) {
            n = NRF_CHAR_SIZE;
        }

retry:
        PT_WAIT_WHILE(pt, write_xfer_size);
        aci_gatt_update_char_value_begin(uart_service_handle, uart_tx_char_handle, 0, n, data + i);
        PT_WAIT_UNTIL(pt, hci_command_complete);
        ret = aci_gatt_update_char_value_end();

        if (ret == BLE_STATUS_INSUFFICIENT_RESOURCES) {
            // this will happen if notifications are enabled and the previous
            // changes haven't been sent over the air yet
            goto retry;
        }
    }

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_bluetooth_hci_process, ev, data) {
    static struct etimer timer;
    static struct pt child_pt;

    PROCESS_BEGIN();

    while (true) {
        // make sure the Bluetooth chip is in reset long enough to actually reset
        etimer_set(&timer, clock_from_msec(10));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // take Bluetooth chip out of reset
        GPIOB->BSRR = GPIO_BSRR_BS_6;

        // wait for the Bluetooth chip to send the reset reason event so we know it is ready
        PROCESS_WAIT_UNTIL(reset_reason);

        PROCESS_PT_SPAWN(&child_pt, hci_init(&child_pt));
        PROCESS_PT_SPAWN(&child_pt, init_uart_service(&child_pt));
        PROCESS_PT_SPAWN(&child_pt, set_discoverable(&child_pt));

        // TODO: we should have a timeout and stop scanning eventually
        PROCESS_WAIT_UNTIL(conn_handle);

        etimer_set(&timer, CLOCK_SECOND);

        // conn_handle is set to 0 upon disconnection
        while (conn_handle) {
            // TODO: send out UART Tx data here
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
            etimer_reset(&timer);
            #define HELLO_STR "Hello!"
            PROCESS_PT_SPAWN(&child_pt, uart_service_send_data(&child_pt, HELLO_STR, strlen(HELLO_STR)));
        }

        // reset Bluetooth chip
        GPIOB->BRR = GPIO_BRR_BR_6;
    }

    PROCESS_END();
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_bluetooth_deinit(void) {
    NVIC_DisableIRQ(EXTI2_3_IRQn);

    // TODO: need to make sure SPI2 is not busy
    NVIC_DisableIRQ(DMA1_Channel4_5_IRQn);

    // nRESET
    // set PB6 output low
    GPIOB->BRR = GPIO_BRR_BR_6;
}
#endif
