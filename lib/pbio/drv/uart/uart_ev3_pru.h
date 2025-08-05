#ifndef _INTERNAL_PBDRV_UART_EV3_PRU_H_
#define _INTERNAL_PBDRV_UART_EV3_PRU_H_

#include <stdbool.h>
#include <stdint.h>

int pbdrv_uart_ev3_pru_load_firmware(uint8_t *firmware_data, uint32_t firmware_size);
int pbdrv_uart_ev3_pru_activate(uint8_t line);

void pbdrv_uart_ev3_pru_handle_irq_data(uint8_t line);
void pbdrv_uart_ev3_pru_set_baudrate(uint8_t line, unsigned int baud);

int pbdrv_uart_ev3_pru_read_bytes(uint8_t line, unsigned char *pdata, int size);
int pbdrv_uart_ev3_pru_write_bytes(uint8_t line, unsigned char *pdata, int size);
bool pbdrv_uart_ev3_pru_can_write(uint8_t line);

#endif // _INTERNAL_PBDRV_UART_EV3_PRU_H_
