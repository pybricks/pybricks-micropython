#include "at91sam7s256.h"

#include "nxt.h"
#include "aic.h"

#include "uart.h"

/* see AT91C_US_MR register */
#define UART_MODE_NORMAL    0x0 /* << 0 ;  normal */
#define UART_MODE_CLOCK     0x0 /* << 4 ;  MCK */
#define UART_MODE_ASYNC     0x0 /* << 8 ;  ASYNC */
#define UART_MODE_CHMOD     0x0 /* << 14 ; normal mode */
#define UART_MODE_MSBF      0x0 /* << 16 ; Least Significant Bit is sent/received first */
#define UART_MODE_MODE9     0x0 /* << 17 ; CHRL defines character length. (1 == 9 bits length) */
#define UART_MODE_CLK0      0x0 /* << 18 ; The USART does not drive the SCK pin. */
#define UART_MODE_OVER      0x0 /* << 19 ; 16x Oversampling. */
#define UART_MODE_INACK     0x0 /* << 20 ; Won't inhibit : The NACK is generated. */
#define UART_MODE_DSNACK    0x0 /* << 21 ; NACK is sent on the ISO line as soon as a parity error occurs in the received character (unless INACK is set). */
#define UART_MODE_MAX_ITER  0x0 /* << 24 ; Only used in mode ISO << not used here */
#define UART_MODE_FILTER    0x0 /* << 28 ; The USART does not filter the receive line. */

#define UART_BRGR_FP        0x0 /* << 16 ; fractionnal part : disabled */

/* since sync == 0 (async) and oversampling == 16x,
 * we know that we will have a divisor of 16 */
#define UART_CLOCK_DIVISOR(baud_rate) \
( ((NXT_CLOCK_FREQ / baud_rate)+8) / 16 )


typedef struct uart_status
{
  U8                    pid;

  U32                   baud_rate;
  usart_char_length_t   char_length;
  usart_parity_t        parity;
  U8                    stop_bits;

  usart_read_callback_t callback;

  AT91S_USART          *regs;
} uart_status_t;


static uart_status_t uarts[2] = { { 0 } , { 0 } };


static void uart_isr(uart_status_t *uart)
{
  /* possible interruptions : */
  /* AT91C_US_ENDRX   : end of receive        */
  /* AT91C_US_RXBRK   : receiver break        */
  /* AT91C_US_ENDTX   : end of transmit       */
  /* AT91C_US_OVRE    : overrun error         */
  /* AT91C_US_FRAME   : framing error         */
  /* AT91C_US_PARE    : parity error          */
  /* AT91C_US_TIMEOUT : timeout error         */
  /* AT91C_US_TXBUFE  : transmit buffer empty */
  /* AT91C_US_RXBUFF  : receive buffer full   */
  /* AT91C_US_NACK    : non-ack               */

  if ((uart->regs->US_CSR & AT91C_US_ENDRX) > 0) { /* end of receive */

  }

  if ((uart->regs->US_CSR & AT91C_US_RXBRK) > 0) { /* receiver break */

  }

  if ((uart->regs->US_CSR & AT91C_US_ENDTX) > 0) { /* end of transmit */

  }

  if ((uart->regs->US_CSR & AT91C_US_OVRE) > 0) { /* overrun error */

  }

  if ((uart->regs->US_CSR & AT91C_US_FRAME) > 0) { /* framing error */

  }

  if ((uart->regs->US_CSR & AT91C_US_FRAME) > 0) { /* framing error */

  }
}


static void uart_isr0()
{
  uart_isr(&uarts[0]);
}


static void uart_isr1()
{
  uart_isr(&uarts[1]);
}


bool uart_init(U8 uart_number, U32 baud_rate,
	       usart_char_length_t char_length,
	       usart_parity_t parity,
	       usart_stop_bits_t stop_bits,
	       usart_read_callback_t callback)
{
  uart_status_t *uart;

  if (uart_number > 1)
    return 0;

  uart = &uarts[uart_number];

  uart->pid         = (uart_number == 0 ? AT91C_ID_US0 : AT91C_ID_US1);
  uart->baud_rate   = baud_rate;
  uart->char_length = char_length;
  uart->parity      = parity;
  uart->stop_bits   = stop_bits;
  uart->callback    = callback;

  if (uart_number == 0)
    uart->regs = AT91C_BASE_US0;
  else
    uart->regs = AT91C_BASE_US1;

  /* we clock the usart and the PIOA */
  *AT91C_PMC_PCER |= uart->pid | AT91C_ID_PIOA;

  /* we disable the pdc */
  uart->regs->US_TCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
  /* we disable the receiver / transceiver */
  uart->regs->US_CR |= AT91C_US_RXDIS  | AT91C_US_TXDIS;
  /* we reset the usart */
  uart->regs->US_CR |= AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA;
  /* we disable all the interruption */
  uart->regs->US_IDR = ~0;

  /* we set the correct mode for the usart */
  uart->regs->US_MR =
    ((  UART_MODE_NORMAL            << 0)
     | (UART_MODE_CLOCK             << 4)
     | (char_length                 << 6)
     | (UART_MODE_ASYNC             << 8)
     | (parity                      << 9)
     | (stop_bits                   << 12)
     | (UART_MODE_CHMOD             << 14)
     | (UART_MODE_MSBF              << 16)
     | (UART_MODE_MODE9             << 17)
     | (UART_MODE_CLK0              << 18)
     | (UART_MODE_OVER              << 19)
     | (UART_MODE_INACK             << 20)
     | (UART_MODE_DSNACK            << 21)
     | (UART_MODE_MAX_ITER          << 24)
     | (UART_MODE_FILTER            << 28));

  /* we set the baud generator at the good rate */
  uart->regs->US_BRGR =
    ((UART_CLOCK_DIVISOR(baud_rate) << 0)
     | (UART_BRGR_FP                << 16));

  /* we disable the timeout and the timeguards :
   * we will use the PDC, so we shouldn't need them
   */
  uart->regs->US_RTOR = 0;
  uart->regs->US_TTGR = 0;


  /* we init the PDC */
  uart->regs->US_RPR  = 0;
  uart->regs->US_RCR  = 0;
  uart->regs->US_TPR  = 0;
  uart->regs->US_TCR  = 0;
  uart->regs->US_RNPR = 0;
  uart->regs->US_RNCR = 0;
  uart->regs->US_TNPR = 0;
  uart->regs->US_TNCR = 0;

  /* we install the interruption routine */
  aic_install_isr(uart_number == 0 ? AT91C_ID_US0 : AT91C_ID_US1,
		  AIC_PRIO_DRIVER,
		  AIC_TRIG_LEVEL,
		  uart_number == 0 ? uart_isr0 : uart_isr1);

  /* we re-enable the interruptions */
  uart->regs->US_IER =
      AT91C_US_ENDRX   /* end of receive        */
    | AT91C_US_RXBRK   /* receiver break        */
    | AT91C_US_ENDTX   /* end of transmit       */
    | AT91C_US_OVRE    /* overrun error         */
    | AT91C_US_FRAME   /* framing error         */
    | AT91C_US_PARE    /* parity error          */
    | AT91C_US_TIMEOUT /* timeout error         */
    | AT91C_US_TXBUFE  /* transmit buffer empty */
    | AT91C_US_RXBUFF  /* receive buffer full   */
    | AT91C_US_NACK;   /* non-ack               */



  /* we re-enable the receiver / transceiver */
  uart->regs->US_CR |= AT91C_US_RXEN  | AT91C_US_TXEN;

  /* we enable the pdc */
  uart->regs->US_TCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;

  return 1;
}


bool uart_write(U8 uart_number,
		void *data,
		U16 lng)
{
  uart_status_t *uart;

  if (uart_number > 1)
    return 0;

  uart = &uarts[uart_number];

  if (uart->regs->US_TCR == 0) {
    uart->regs->US_TPR = (U32)data;
    uart->regs->US_TCR = lng;
  } else {
    while (uart->regs->US_TNCR != 0);

    uart->regs->US_TNPR = (U32)data;
    uart->regs->US_TNCR = lng;
  }

  return 1;
}


bool uart_can_write(U8 uart_number)
{
  uart_status_t *uart;

  if (uart_number > 1)
    return 0;

  uart = &uarts[uart_number];

  return (uart->regs->US_TCR == 0 || uart->regs->US_TNCR == 0);
}


bool uart_start_reading(U8 uart_number,
			void *buffer,
			U16 lng)
{
  uart_status_t *uart;

  if (uart_number > 1)
    return 0;

  uart = &uarts[uart_number];

  if (uart->regs->US_RCR == 0) {

    uart->regs->US_RPR = (U32)buffer;
    uart->regs->US_RCR = lng;

  } else {

    if (uart->regs->US_RNCR != 0)
      return 0;

    uart->regs->US_RNPR = (U32)buffer;
    uart->regs->US_RNCR = lng;
  }

  return 1;
}


bool uart_read_is_filled(U8 uart_number, void *buffer, U16 lng)
{
  uart_status_t *uart;

  if (uart_number > 1)
    return 0;

  uart = &uarts[uart_number];

  if ( ( ((U32)uart->regs->US_RPR) + ((U32)uart->regs->US_RCR) )
       == ((U32)buffer) + lng
       && ((U32)uart->regs->US_RCR) != 0)
    return 0;

  if ( ( ((U32)uart->regs->US_RNPR) + ((U32)uart->regs->US_RNCR) )
       == ((U32)buffer) + lng
       && ((U32)uart->regs->US_RNCR) != 0)
    return 0;

  /* the buffer is no more used by the pdc, so we can
   * consider that the read was done
   */

  return 1;
}



void uart_read_ack(U8 uart_number, void *buffer)
{
  /* empty */
}
