// tary, 0:36 2012/12/23

#ifndef __AM18X_UART_H__
#define __AM18X_UART_H__

#include "am18x_map.h"

typedef enum {
	UART_PARITY_NONE,
	UART_PARITY_ODD,
	UART_PARITY_EVEN,
	UART_PARITY_1S,
	UART_PARITY_0S,
} uart_parity_t;

typedef enum {
	UART_STOP_1BIT,
	UART_STOP_2BIT,
} uart_stop_t;

typedef enum {
	UART_LENGTH_5BIT,
	UART_LENGTH_6BIT,
	UART_LENGTH_7BIT,
	UART_LENGTH_8BIT,
} uart_length_t;

typedef enum {
	UART_FLOW_NONE,
	UART_FLOW_SOFT,
	UART_FLOW_AFC,
} uart_flow_t;

typedef enum {
	UART_MODE_DISABLE,
	UART_MODE_INTR,
	UART_MODE_DMA,
} uart_xmode_t;

typedef struct {
	uint32_t	baudrate;
	uart_length_t	length;
	uart_stop_t	stop;
	uart_parity_t	parity;
	uart_flow_t	flow;
	uart_xmode_t	mode;
} uart_conf_t;

typedef enum {
	UART_TTRIG_EMPTY,
	UART_TTRIG_16BYTE,
	UART_TTRIG_32BYTE,
	UART_TTRIG_48BYTE,
} uart_ttrig_t;

typedef enum {
	UART_RTRIG_1BYTE,
	UART_RTRIG_8BYTE,
	UART_RTRIG_16BYTE,
	UART_RTRIG_32BYTE,
} uart_rtrig_t;

typedef enum {
	UFIFO_CMD_TX_RESET,
	UFIFO_CMD_RX_RESET,
	UFIFO_CMD_RESET,
	UFIFO_CMD_DISABLE,
	UFIFO_CMD_ENABLE,
} ufifo_cmd_t;

typedef struct {
	ufifo_cmd_t	cmd;
	uart_ttrig_t	ttrig;
	uart_rtrig_t	rtrig;
} uart_fifo_t;

#define UFIFO_SIZE		(64)

typedef enum {
	FIFO_TYPE_TX,
	FIFO_TYPE_RX,
} fifo_type_t;

typedef enum {
	UART_MISC_BREAK,
	/* only valid AFC disabled */
	UART_MISC_REQSEND,
} uart_misc_t;

typedef enum {
	STATE_TX_EMPTY,
	STATE_TX_BUF_EMPTY,
	STATE_RX_READY,

	STATE_ERROR_ANY,
	STATE_ERROR_BREAK,
	STATE_ERROR_FRAME,
	STATE_ERROR_PARITY,
	STATE_ERROR_OVERRUN,

	STATE_CTS_ACTIVE,
	STATE_CTS_CHANGED,

	STATE_FIFO_XXFULL,
	STATE_FIFO_TXFULL,
	STATE_FIFO_RXFULL,
} uart_state_type_t;

am18x_rt uart_init_conf(uart_conf_t* conf);
am18x_rt uart_set_conf(UART_con_t* con, const uart_conf_t* conf);
am18x_rt uart_get_conf(const UART_con_t* con, uart_conf_t* conf);
am18x_rt uart_set_fifo(UART_con_t* con, const uart_fifo_t* fifo);
/* return number of data in specified fifo */
uint32_t uart_fifo_state(const UART_con_t* con, fifo_type_t type);
am18x_rt uart_cntl_misc(UART_con_t* con, uart_misc_t misc);
am18x_bool uart_state(const UART_con_t* con, uart_state_type_t type);
am18x_rt uart_write_byte(UART_con_t* con, uint8_t c);
uint8_t uart_read_byte(const UART_con_t* con);

#endif//__AM18X_UART_H__
