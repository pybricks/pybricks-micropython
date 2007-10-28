/* Driver for the NXT's LCD display.
 *
 * This driver contains a basic SPI driver to talk to the UltraChip
 * 1601 LCD controller, as well as a higher level API implementing the
 * UC1601's commandset.
 *
 * Note that the SPI driver is not suitable as a general-purpose SPI
 * driver: the MISO pin (Master-In Slave-Out) is instead wired to the
 * UC1601's CD input (used to select whether the transferred data is
 * control commands or display data). Thus, the SPI driver here takes
 * manual control of the MISO pin, and drives it depending on the type
 * of data being transferred.
 *
 * This also means that you can only write to the UC1601, not read
 * back from it. This is not too much of a problem, as we can just
 * introduce a little delay in the places where we really need it.
 */

#include "base/at91sam7s256.h"

#include "base/mytypes.h"
#include "base/lock.h"
#include "base/interrupts.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/drivers/lcd.h"

/* Internal command bytes implementing part of the basic commandset of
 * the UC1601.
 */
#define SET_COLUMN_ADDR0(addr) (0x00 | (addr & 0xF))
#define SET_COLUMN_ADDR1(addr) (0x10 | ((addr >> 4) & 0xF))
#define SET_MULTIPLEX_RATE(rate) (0x20 | (rate & 3))
#define SET_SCROLL_LINE(sl) (0x40 | (sl & 0x3F))
#define SET_PAGE_ADDR(page) (0xB0 | (page & 0xF))
#define SET_BIAS_POT0() (0x81)
#define SET_BIAS_POT1(pot) (pot & 0xFF)
#define SET_RAM_ADDR_CONTROL(auto_wrap, page_first, neg_inc, write_only_inc) \
           (0x88 | (auto_wrap << 0) | (page_first << 1) |                    \
            (neg_inc << 2) | (write_only_inc << 3))
#define SET_ALL_PIXELS_ON(on) (0xA4 | (on & 1))
#define SET_INVERSE_DISPLAY(on) (0xA6 | (on & 1))
#define ENABLE(on) (0xAE | (on & 1))
#define SET_MAP_CONTROL(mx, my) (0xC0 | (mx << 1) | (my << 2))
#define RESET() (0xE2)
#define SET_BIAS_RATIO(bias) (0xE8 | (bias & 3))


/*
 * SPI controller driver.
 */
typedef enum spi_mode {
  COMMAND,
  DATA
} spi_mode;

/*
 * The SPI device state. Contains some of the actual state of the bus,
 * and transitory state for interrupt-driven DMA transfers.
 */
static volatile struct {
  /* TRUE if the SPI driver is configured for sending commands, FALSE
   * if it's configured for sending video data. */
  spi_mode mode;

  /* A pointer to the in-memory screen framebuffer to mirror to
   * screen, and a flag stating whether the in-memory buffer is dirty
   * (new content needs mirroring to the LCD device.
   */
  U8 *screen;
  bool screen_dirty;

  /* State used by the display update code to manage the DMA
   * transfer. */
  U8 *data;
  U8 page;
  bool send_padding;
} spi_state = {
  COMMAND, /* We're initialized in command tx mode */
  NULL,    /* No screen buffer */
  FALSE,   /* ... So obviously not dirty */
  NULL,    /* No current refresh data pointer */
  0,       /* Current state: 1st data page... */
  FALSE    /* And about to send display data */
};


/*
 * Set the data transmission mode.
 */
static void spi_set_tx_mode(spi_mode mode) {
  if (spi_state.mode == mode)
    /* Mode hasn't changed, no-op. */
    return;
  else
    /* If there is a mode switch, we need to let the SPI controller
     * drain all data first, to avoid spurious writes of the wrong
     * type.
     */
    while(!(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY));

  spi_state.mode = mode;

  if (mode == COMMAND) {
    *AT91C_PIOA_CODR = AT91C_PA12_MISO;
  } else {
    *AT91C_PIOA_SODR = AT91C_PA12_MISO;
  }
}


/*
 * Send a command byte to the LCD controller.
 */
static void spi_write_command_byte(U8 command) {
  spi_set_tx_mode(COMMAND);

  /* Wait for the transmit register to empty. */
  while (!(*AT91C_SPI_SR & AT91C_SPI_TDRE));

  /* Send the command byte and wait for a reply. */
  *AT91C_SPI_TDR = command;
}


/* Interrupt routine for handling DMA screen refreshing. */
void spi_isr() {
  /* If we are in the initial state, determine whether we need to do a
   * refresh cycle.
   */
  if (spi_state.page == 0 && !spi_state.send_padding) {
    /* Atomically retrieve the dirty flag and set it to FALSE. This is
     * to avoid race conditions where a set of the dirty flag could
     * get squashed by the interrupt handler resetting it.
     */
    bool dirty = atomic_cas8((U8*)&(spi_state.screen_dirty), FALSE);
    spi_state.data = dirty ? spi_state.screen: NULL;

    /* If the screen is not dirty, or if there is no screen pointer to
     * source data from, then shut down the DMA refresh interrupt
     * routine. It'll get reenabled by the screen dirtying function or
     * the 1kHz interrupt update if the screen becomes dirty.
     */
    if (!spi_state.data) {
      *AT91C_SPI_IDR = AT91C_SPI_ENDTX;
      return;
    }
  }

  /* Make sure that we are in data TX mode. This is a no-op if we
   * already are, so it costs next to nothing to make sure of it at
   * every interrupt.
   */
  spi_set_tx_mode(DATA);


  if (!spi_state.send_padding) {
    /* We are at the start of a page, so we need to send 100 bytes of
     * pixel data to display. We also set the state for the next
     * interrupt, which is to send end-of-page padding.
     */
    spi_state.send_padding = TRUE;
    *AT91C_SPI_TNPR = (U32)spi_state.data;
    *AT91C_SPI_TNCR = 100;
  } else {
    /* 100 bytes of displayable data have been transferred. We now
     * have to send 32 more bytes to get to the end of the page and
     * wrap around. We also set up the state for the next interrupt,
     * which is to send the visible part of the next page of data.
     *
     * Given that this data is off-screen, we just resend the last 32
     * bytes of the 100 we just transferred.
     */
    spi_state.page = (spi_state.page + 1) % 8;
    spi_state.data += 100;
    spi_state.send_padding = FALSE;
    *AT91C_SPI_TNPR = (U32)(spi_state.data - 32);
    *AT91C_SPI_TNCR = 32;
  }
}


static void spi_init() {
  interrupts_disable();

  /* Enable power to the SPI and PIO controllers. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_SPI) | (1 << AT91C_ID_PIOA);

  /* Configure the PIO controller: Hand the MOSI (Master Out, Slave
   * In) and SPI clock pins over to the SPI controller, but keep MISO
   * (Master In, Slave Out) and PA10 (Chip Select in this case) and
   * configure them for manually driven output.
   *
   * The initial configuration is command mode (sending LCD commands)
   * and the LCD controller chip not selected.
   */
  *AT91C_PIOA_PDR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;
  *AT91C_PIOA_ASR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;

  *AT91C_PIOA_PER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
  *AT91C_PIOA_OER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
  *AT91C_PIOA_CODR = AT91C_PA12_MISO;
  *AT91C_PIOA_SODR = AT91C_PA10_NPCS2;

  /* Disable all SPI interrupts, then configure the SPI controller in
   * master mode, with the chip select locked to chip 0 (UC1601 LCD
   * controller), communication at 2MHz, 8 bits per transfer and an
   * inactive-high clock signal.
   */
  *AT91C_SPI_CR = AT91C_SPI_SWRST;
  *AT91C_SPI_CR = AT91C_SPI_SPIEN;
  *AT91C_SPI_IDR = ~0;
  *AT91C_SPI_MR = (6 << 24) | AT91C_SPI_MSTR;
  AT91C_SPI_CSR[0] = ((0x18 << 24) | (0x18 << 16) | (0x18 << 8) |
                      AT91C_SPI_BITS_8 | AT91C_SPI_CPOL);

  /* Now that the SPI bus is initialized, pull the Chip Select line
   * low, to select the uc1601. For some reason, letting the SPI
   * controller do this fails. Therefore, we force it once now.
   */
  *AT91C_PIOA_CODR = AT91C_PA10_NPCS2;

  /* Install an interrupt handler for the SPI controller, and enable
   * DMA transfers for SPI data. All SPI-related interrupt sources are
   * inhibited, so it won't bother us until we're ready.
   */
  aic_install_isr(AT91C_ID_SPI, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL, spi_isr);
  *AT91C_SPI_PTCR = AT91C_PDC_TXTEN;

  interrupts_enable();
}


/* Initialize the LCD controller. */
void lcd_init() {
  int i;
  /* This is the command byte sequence that should be sent to the LCD
   * after a reset.
   */
  const U8 lcd_init_sequence[] = {
    /* LCD power configuration.
     *
     * The LEGO Hardware Developer Kit documentation specifies that the
     * display should be configured with a multiplex rate (MR) of 1/65,
     * and a bias ratio (BR) of 1/9, and a display voltage V(LCD) of 9V.
     *
     * The specified MR and BR both map to simple command writes. V(LCD)
     * however is determined by an equation that takes into account both
     * the BR and the values of the PM (Potentiometer) and TC
     * (Temperature Compensation) configuration parameters.
     *
     * The equation and calculations required are a little too complex
     * to inline here, but the net result is that we should set a PM
     * value of 92. This will result in a smooth voltage gradient, from
     * 9.01V at -20 degrees Celsius to 8.66V at 60 degrees Celsius
     * (close to the maximum operational range of the LCD display).
     */
    SET_MULTIPLEX_RATE(3),
    SET_BIAS_RATIO(3),
    SET_BIAS_POT0(),
    SET_BIAS_POT1(92),

    /* Set the RAM address control, which defines how the data we send
     * to the LCD controller are placed in its internal video RAM.
     *
     * We want the bytes we send to be written in row-major order (line
     * by line), with no automatic wrapping.
     */
    SET_RAM_ADDR_CONTROL(1, 0, 0, 0),

    /* Set the LCD mapping mode, which defines how the data in video
     * RAM is driven to the display. The display on the NXT is mounted
     * upside down, so we want just Y mirroring.
     */
    SET_MAP_CONTROL(0, 1),

    /* Set the initial position of the video memory cursor. We
     * initialize it to point to the start of the screen.
     */
    SET_COLUMN_ADDR0(0),
    SET_COLUMN_ADDR1(0),
    SET_PAGE_ADDR(0),

    /* Turn the display on. */
    ENABLE(1),
  };

  /* Initialize the SPI controller to enable communication, then wait
   * a little bit for the UC1601 to register the new SPI bus state.
   */
  spi_init();
  systick_wait_ms(20);

  /* Issue a reset command, and wait. Normally here we'd check the
   * UC1601 status register, but as noted at the start of the file, we
   * can't read from the LCD controller due to the board setup.
   */
  spi_write_command_byte(RESET());
  systick_wait_ms(20);

  for (i=0; i<sizeof(lcd_init_sequence); i++)
    spi_write_command_byte(lcd_init_sequence[i]);
}


/* Mirror the given display buffer to the LCD controller. The given
 * buffer must be exactly 100x64 bytes, one full screen of pixels.
 */
void lcd_set_display(U8 *display) {
  spi_state.screen = display;
  *AT91C_SPI_IER = AT91C_SPI_ENDTX;
}


inline void lcd_dirty_display() {
  spi_state.screen_dirty = TRUE;
}


void lcd_1kHz_update() {
  if (spi_state.screen_dirty) {
    *AT91C_SPI_IER = AT91C_SPI_ENDTX;
  }
}


/* Shutdown the LCD controller. */
void lcd_shutdown() {
  /* When power to the controller goes out, there is the risk that
   * some capacitors mounted around the controller might damage it
   * when discharging in an uncontrolled fashion. To avoid this, the
   * spec recommends putting the controller into reset mode before
   * shutdown, which activates a drain circuit to empty the board
   * capacitors gracefully.
   */
  *AT91C_SPI_IDR = ~0;
  *AT91C_SPI_PTCR = AT91C_PDC_TXTDIS;
  spi_write_command_byte(RESET());
  systick_wait_ms(20);
}
