// tary, 0:22 2012/12/22
#include "am18x_uart.h"
#include "am18x_dclk.h"

uint32_t uart_input_freq(UART_con_t* ucon) {
	if (ucon == UART0) {
		return dev_get_freq(DCLK_ID_UART0);
	}
	if (ucon == UART1) {
		return dev_get_freq(DCLK_ID_UART1);
	}
	if (ucon == UART2) {
		return dev_get_freq(DCLK_ID_UART2);
	}
	return 0UL;
}

static inline uint32_t uart_get_divisor(UART_con_t* ucon, uint32_t baud_rate) {
	uint32_t samp_clk;

	samp_clk = 16;
	if (FIELD_GET(ucon->MDR, MDR_OSM_SEL_MASK) == MDR_OSM_SEL_13x) {
		samp_clk = 13;
	}

	return uart_input_freq(ucon) / (samp_clk * baud_rate);
}

am18x_rt uart_init_conf(uart_conf_t* conf) {
	assert(conf);

	conf->baudrate = 115200UL;
	conf->length = UART_LENGTH_8BIT;
	conf->stop = UART_STOP_1BIT;
	conf->parity = UART_PARITY_NONE;
	conf->flow = UART_FLOW_NONE;
	conf->mode = UART_MODE_INTR;

	return AM18X_OK;
}

am18x_rt uart_set_conf(UART_con_t* ucon, const uart_conf_t* conf) {
	uint32_t reg, msk, v;

	assert(conf);
	assert(conf->baudrate);

	// 2. Set the desired baud rate by writing the appropriate
	// clock divisor values to the divisor latch registers
	ucon->MDR = FIELD_SET(ucon->MDR, MDR_OSM_SEL_MASK, MDR_OSM_SEL_13x);

	v = uart_get_divisor(ucon, conf->baudrate);
	ucon->DLL = __field_xget(v, 0x00FF);
	ucon->DLH = __field_xget(v, 0xFF00);

	// 4. Choose the desired protocol settings by writing the
	// appropriate values to the line control register
	reg = FIELD_SET(ucon->LCR, LCR_DLAB_MASK, LCR_DLAB_rbr_thr_ier);
	reg = FIELD_SET(reg, LCR_BC_MASK, LCR_BC_disabled);

	msk = LCR_PEN_MASK | LCR_EPS_MASK | LCR_SP_MASK;
	switch (conf->parity) {
	case UART_PARITY_ODD:
		v = LCR_PEN_Yes | LCR_EPS_Odd | LCR_SP_disabled;
		break;
	case UART_PARITY_EVEN:
		v = LCR_PEN_Yes | LCR_EPS_Even | LCR_SP_disabled;
		break;
	case UART_PARITY_1S:
		v = LCR_PEN_Yes | LCR_EPS_Odd | LCR_SP_enabled;;
		break;
	case UART_PARITY_0S:
		v = LCR_PEN_Yes | LCR_EPS_Even | LCR_SP_enabled;
		break;
	case UART_PARITY_NONE:
	default:
		v = LCR_PEN_No;
		break;
	}
	reg = FIELD_SET(reg, msk, v);

	switch (conf->stop) {
	case UART_STOP_2BIT:
		v = LCR_STB_15_2STOP;
		break;
	case UART_STOP_1BIT:
	default:
		v = LCR_STB_1STOP;
		break;
	}
	reg = FIELD_SET(reg, LCR_STB_MASK, v);

	switch(conf->length) {
	case UART_LENGTH_5BIT:
		v = LCR_WLS_5bits;
		break;
	case UART_LENGTH_6BIT:
		v = LCR_WLS_6bits;
		break;
	case UART_LENGTH_7BIT:
		v = LCR_WLS_7bits;
		break;
	case UART_LENGTH_8BIT:
	default:
		v = LCR_WLS_8bits;
		break;
	}
	reg = FIELD_SET(reg, LCR_WLS_MASK, v);
	ucon->LCR = reg;

	// 6. Choose the desired response to emulation suspend events
	// by configuring the FREE bit and enable the UART
	// by setting the UTRST and URRST bits in the power and emulation
	// management register
	msk = PWREMU_UTRST_MASK | PWREMU_URRST_MASK;
	v = PWREMU_UTRST_enabled | PWREMU_URRST_enabled;// | PWREMU_FREE_run;
	ucon->PWREMU_MGMT = FIELD_SET(ucon->PWREMU_MGMT, msk, v);

	return AM18X_OK;
}

am18x_bool uart_state(const UART_con_t* ucon, uart_state_type_t type) {
	am18x_bool r;

	r = AM18X_FALSE;
	switch (type) {
	case STATE_TX_EMPTY:
		if (FIELD_GET(ucon->LSR, LSR_TEMT_MASK) == LSR_TEMT_yes) {
			r = AM18X_TRUE;
		}
		break;
	case STATE_TX_BUF_EMPTY:
		if (FIELD_GET(ucon->LSR, LSR_THRE_MASK) == LSR_THRE_yes) {
			r = AM18X_TRUE;
		}
		break;
	case STATE_RX_READY:
		if (FIELD_GET(ucon->LSR, LSR_DR_MASK) == LSR_DR_yes) {
			r = AM18X_TRUE;
		}
		break;
	case STATE_ERROR_ANY:
	case STATE_ERROR_BREAK:
	case STATE_ERROR_FRAME:
	case STATE_ERROR_PARITY:
	case STATE_ERROR_OVERRUN:

	case STATE_CTS_ACTIVE:
	case STATE_CTS_CHANGED:

	case STATE_FIFO_XXFULL:
	case STATE_FIFO_TXFULL:
	case STATE_FIFO_RXFULL:
		break;
	}
	return r;
}

am18x_rt uart_write_byte(UART_con_t* ucon, uint8_t c) {
	ucon->THRw = c;
	return AM18X_OK;
}

uint8_t uart_read_byte(const UART_con_t* ucon) {
	return ucon->RBRr;
}
