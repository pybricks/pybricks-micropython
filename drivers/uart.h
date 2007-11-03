#ifndef __NXOS_UART_H__
#define __NXOS_UART_H__

#include "base/mytypes.h"

typedef enum usart_char_length
  {
    USART_5BITS = 0x0,
    USART_6BITS = 0x1,
    USART_7BITS = 0x2,
    USART_8BITS = 0x3
  } usart_char_length_t;


typedef enum usart_parity
  {
    USART_EVEN_PARITY        = 0x0,
    USART_ODD_PARITY         = 0x1,
    USART_FORCED_TO_0_PARITY = 0x2,
    USART_FORCED_TO_1_PARITY = 0x3,
    USART_NO_PARITY          = 0x4,
    USART_MULTIDROP_PARITY   = 0x6
  } usart_parity_t;

typedef enum usart_stop_bits
  {
    USART_ONE_STOP_BIT            = 0x0,
    USART_ONE_AND_A_HALF_STOP_BIT = 0x1,
    USART_TWO_STOP_BIT            = 0x2
  } usart_stop_bits_t;



/*
 * called back each time a buffer has been filled in. Their are filled in
 * in the given order
 */
typedef void (*usart_read_callback_t)();


/*
 * the driver using an uart port
 * must configure itself the PIO
 * uart_number : 0 or 1
 */
bool nx_uart_init(U8 uart_number, U32 baud_rate,
	       usart_char_length_t char_length,
	       usart_parity_t parity,
	       usart_stop_bits_t stop_bits,
	       usart_read_callback_t callback);

/*
 * Will send the data asap.
 * If the two slot in the pdc are used, this function will be blocking.
 */
bool nx_uart_write(U8 uart_number,
		void *data,
		U16 lng);

bool nx_uart_can_write(U8 uart_number);

/*
 * Will start filling in the specified buffer with the data coming from the uart.
 * Since the uart pdc can manage two buffers, you can call this function
 * two time with two different buffers when you start.
 * Return false if you try to use more than 2 buffers
 */
bool nx_uart_read_start(U8 uart_number,
		     void *buffer,
		     U16 lng);

/* Indicates if the buffer can be read by the user application:
 */
bool nx_uart_read_is_filled(U8 uart_number, void *buffer, U16 lng);

/* The user application tell the driver that the buffer has been handled */
/* Note: in the current implementation, this function does nothing */
void nx_uart_read_ack(U8 uart_number, void *buffer);

#endif
