/* Driver for the NXT's I2S sound controller.
 *
 * The sound controller is connected to the main board via the SSC
 * (Synchronous Serial Controller).
 */

#include "base/at91sam7s256.h"

#include "base/mytypes.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"
#include "base/drivers/systick.h"
#include "base/drivers/sound.h"

/* Statically defined digitized sine wave, used for tone
 * generation.
 */
static const U32 tone_pattern[16] =
  {
    0xF0F0F0F0,0xF0F0F0F0,
    0xFCFCFCFC,0xFCFCFCFC,
    0xFFFFFFFF,0xFFFFFFFF,
    0xFCFCFCFC,0xFCFCFCFC,
    0xF0F0F0F0,0xF0F0F0F0,
    0xC0C0C0C0,0xC0C08080,
    0x00000000,0x00000000,
    0x8080C0C0,0xC0C0C0C0
  };

/* When a tone is playing, this value contains the number of times the
 * previous digitized sine wave is to be played.
 */
static volatile U32 tone_cycles;


static void
sound_isr()
{
  if (tone_cycles--) {
    /* Tell the DMA controller to stream the static sine wave, 16
     * words of data.
     */
    *AT91C_SSC_TNPR = (U32) tone_pattern;
    *AT91C_SSC_TNCR = 16;
  } else {
    /* Transmit complete, disable sound again. */
    *AT91C_SSC_IDR = AT91C_SSC_ENDTX;
  }
}


/* Initialise the Synchronous Serial Controller. */
void
nx_sound_init()
{
  nx_interrupts_disable();

  /* Start by inhibiting all sound output. Then enable power to the
   * SSC peripheral and do a software reset. The combination of these
   * three actions will get the controller reinitialized whether we
   * are warm- or cold-booting the NXT.
   */
  *AT91C_PMC_PCER = (1 << AT91C_ID_SSC);
  *AT91C_SSC_IDR = ~0;
  *AT91C_SSC_CR = AT91C_SSC_SWRST;

  /* Configure the transmit clock to be based on the board master
   * clock, to clock continuously (don't stop sending a clock signal
   * when there is no data), and set transmissions to start as soon as
   * there is data available to send.
   */
  *AT91C_SSC_TCMR = (AT91C_SSC_CKS_DIV +
                     AT91C_SSC_CKO_CONTINOUS +
                     AT91C_SSC_START_CONTINOUS);

  /* Configure the framing mode for transmission: 32-bit data words, 8
   * words per frame, most significant bit first. Also set the default
   * driven value (when there is no data being streamed) to 1.
   */
  *AT91C_SSC_TFMR =
    31 + AT91C_SSC_DATDEF + AT91C_SSC_MSBF + (7 << 8);

  /* Idle the output data pin of the SSC. The value on the pin will
   * now be whatever the SSC pumps out.
   */
  *AT91C_PIOA_PDR = AT91C_PA17_TD;

  /* Start transmitting. */
  *AT91C_SSC_CR = AT91C_SSC_TXEN;

  /* Install the interrupt routine that will feed data to the DMA
   * controller when we are outputting data.
   */
  nx_aic_install_isr(AT91C_ID_SSC, AIC_PRIO_DRIVER,
		     AIC_TRIG_LEVEL, sound_isr); // Level or edge?

  nx_interrupts_enable();
}


/* Emit a tone at a given frequency (in Hz) for a given duration.
 *
 * This function schedules the tone to be output and returns
 * immediately.
 */
void nx_sound_freq_async(U32 freq, U32 ms)
{
  /* Set the master clock divider to output the correct frequency.
   *
   * The values are currently magic borrowed from Lejos.
   * TODO: Figure this out and document it.
   */
  *AT91C_SSC_CMR = ((96109714 / 1024) / freq) + 1;
  tone_cycles = (freq * ms) / 2000 - 1;

  /* Enable handling of the transmit end interrupt. */
  *AT91C_SSC_IER = AT91C_SSC_ENDTX;

  /* Tell the DMA controller to start transmitting. This will cause an
   * interrupt, and the interrupt handler will point the DMA
   * controller at the data.
   */
  *AT91C_SSC_PTCR = AT91C_PDC_TXTEN;
}


/* Emit a tone at a given frequency (in Hz) for a given duration.
 *
 * This function blocks for the duration of the tone. If you do not
 * want this, use sound_freq_async instead.
 */
void nx_sound_freq(U32 freq, U32 ms)
{
  nx_sound_freq_async(freq, ms);
  nx_systick_wait_ms(ms);
}
