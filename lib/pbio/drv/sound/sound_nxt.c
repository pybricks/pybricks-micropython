// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2023 The Pybricks Authors
// original source: https://raw.githubusercontent.com/cmorty/lejos/079530f422098faeb89fee4cb52065ea66d2a836/nxtvm/platform/nxt/sound.c

/* leJOS Sound generation
 * The module provides support for audio output. Two modes are supported, tone
 * generation and the playback of PCM based audio samples. Both use pulse
 * density modulation to actually produce the output.
 * To produce a tone a single pdm encoded cycle is created (having the
 * requested amplitude), this single cycle is then played repeatedly to
 * generate the tone. The bit rate used to output the sample defines the
 * frequency of the tone and the number of repeats represents then length.
 * To play an encoded sample (only 8 bit PCM is currently supported), each PCM
 * sample is turned into a 256 bit pdm block, which is then output (at the
 * sample rate), to create the output. Again the amplitude of the samples may
 * be controlled.
 * The actual output of the bits is performed using the built in Synchronous
 * Serial Controller (SSC). This is capable of outputting a series of bits to
 * port at fixed intervals and is used to output the pdm audio.
 *
 * Pybricks: tone mode is removed and PCM uses 16-bit data.
 */

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_NXT

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>
#include <nxos/drivers/_aic.h>
#include <nxos/interrupts.h>
#include <nxos/nxt.h>

// We have two possible types of PDM encoding for use when playing PCM
// data. The first is based on the LEGO firmware and encodes each 8 bit
// value to a 256-bit PDM value by using a lookup table. The second uses
// a second order Sigma-Delta encoder to create a 32-bit encoding. The
// Sigma-Delta encoder uses less memory and produces a slightly better
// sounding result, but requires approximately 15% of the cpu when
// playing 8kHz samples. The lookup method requires more memory but only
// uses approx 4% of the cpu when playing the same clip.
// The sigma-delta method generates fewer interrupts (due to the smaller
// sample size), and because of this has more accurate timing (because the
// clock divisor is larger).
#define PDM_LOOKUP 1
#define PDM_SIGMA_DELTA 2
#define PDM_ENCODE PDM_SIGMA_DELTA

/* Buffer length must be a multiple of 8 and at most 64 (preferably as long as possible) */
#define PDM_BUFFER_LENGTH 64

/* Main clock frequency */
#define OSC NXT_CLOCK_FREQ

/* Size of a sample block */
#if (PDM_ENCODE == PDM_LOOKUP)
#define SAMPLE_BITS 256
#else
#define SAMPLE_BITS 32
#endif

#define SAMPLE_PER_BUF ((PDM_BUFFER_LENGTH * 32) / SAMPLE_BITS)
#define MAX_RATE 48000  // Pybricks: was 22050 in lejos - may want to change back to limit CPU usage
#define MIN_RATE 2000

enum {
    SOUND_MODE_NONE,
    SOUND_MODE_TONE,
    SOUND_MODE_PCM
};

static struct {
    // pointer to the sample data
    const uint16_t *ptr;
    // The number of hardware samples ahead
    int32_t count;
    // Double buffer
    uint32_t buf[2][PDM_BUFFER_LENGTH];
    // Current sample index
    uint32_t out_index;
    // Place to add new samples
    uint32_t in_index;
    // 0 or 1, identifies the current buffer
    uint8_t buf_id;
    // Size of the sample in 32 bit words
    uint8_t len;
} sample;

#if (PDM_ENCODE == PDM_LOOKUP)
// Lookup table for PDM encoding. Contains 0-32 evenly spaced set bits.
static const uint32_t sample_pattern[] =
{
    0x00000000, 0x80000000, 0x80008000, 0x80200400,
    0x80808080, 0x82081040, 0x84208420, 0x88442210,
    0x88888888, 0x91224488, 0x92489248, 0xa4924924,
    0xa4a4a4a4, 0xa94a5294, 0xaa54aa54, 0xaaaa5554,
    0xaaaaaaaa, 0xd555aaaa, 0xd5aad5aa, 0xd6b5ad6a,
    0xdadadada, 0xdb6db6da, 0xedb6edb6, 0xeeddbb76,
    0xeeeeeeee, 0xf7bbddee, 0xfbdefbde, 0xfdf7efbe,
    0xfefefefe, 0xffdffbfe, 0xfffefffe, 0xfffffffe,
    0xffffffff
};
#endif

static void sound_interrupt_enable(uint32_t typ) {
    // Enable interrupt notification of either the end of the next buffer
    // or the end of all output. Having both enabled does not seem to work
    // the end notifcation seems to get lost.
    *AT91C_SSC_IDR = AT91C_SSC_TXBUFE | AT91C_SSC_ENDTX | AT91C_SSC_TXEMPTY;
    *AT91C_SSC_IER = typ;
}

static void sound_interrupt_disable(void) {
    *AT91C_SSC_IDR = AT91C_SSC_TXBUFE | AT91C_SSC_ENDTX | AT91C_SSC_TXEMPTY;
}

static void sound_enable(void) {
    *AT91C_PIOA_PDR = AT91C_PA17_TD;
}

static void sound_disable(void) {
    *AT91C_PIOA_PER = AT91C_PA17_TD;
}

#if (PDM_ENCODE == PDM_LOOKUP)

static void sound_fill_sample_buffer(void) {
    sample.buf_id ^= 1;
    uint32_t *sbuf = sample.buf[sample.buf_id];
    const uint16_t *samples = sample.ptr;
    uint32_t out = sample.out_index;
    uint32_t in = sample.in_index;
    uint8_t i;

    /* Each 8-bit sample is turned into 8 32-bit numbers, i.e. 256 bits altogether */
    for (i = 0; i < PDM_BUFFER_LENGTH >> 3; i++) {
        uint8_t smp;
        uint8_t msk;
        uint8_t s3;

        if (out != in) {
            smp = samples[out++] >> 8;
        } else {
            smp = 128;
        }

        msk = "\x00\x10\x22\x4a\x55\x6d\x77\x7f"[smp & 7];
        s3 = smp >> 3;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        msk >>= 1;
        *sbuf++ = sample_pattern[s3 + (msk & 1)];
        *sbuf++ = sample_pattern[s3];

        #if 0
        // An alternative that doesn't need a sample_pattern array:
        uint32_t msb = 0xffffffff << (32 - (smp >> 3));
        uint32_t lsb = msb | (msb >> 1);
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        msk >>= 1;
        *sbuf++ = ((msk & 1) ? lsb : msb);
        *sbuf++ = msb;
        #endif
    }

    sample.out_index = out;
}

#else // (PDM_ENCODE == PDM_LOOKUP)

static void sound_fill_sample_buffer(void) {
    // Fill the sample buffer converting the PCM samples in PDM samples. This
    // version uses a second order sigma-delta convertor. The actual conversion
    // used is unstable for values at the full range points. To avoid this
    // problem we limit the accumulated error. In this case we limit the error
    // by using a simple mod operation.
    sample.buf_id ^= 1;

    uint32_t *sbuf = sample.buf[sample.buf_id];
    const uint16_t *samples = sample.ptr;
    uint32_t out = sample.out_index;
    uint32_t in = sample.in_index;

    // Error accumulation terms
    static int e = 0;
    static int e2 = 0;

    for (int i = 0; i < SAMPLE_PER_BUF; i++) {
        int res;

        if (out != in) {
            res = samples[out++] - INT16_MAX;
        } else {
            res = 0;
        }

        // Perform sigma-delta conversion
        uint32_t bits = 0;

        for (uint32_t bit = 0x80000000; bit; bit >>= 1) {
            if (e2 >= 0) {
                e = res - INT16_MAX + e;
                e2 = e - INT16_MAX + (e2 & 0x1ffff);
                bits |= bit;
            } else {
                e = res + INT16_MAX + e;
                e2 = e + INT16_MAX + (e2 | ~0x1ffff);
            }
        }

        *sbuf++ = bits;
    }

    sample.out_index = out;
}

#endif // (PDM_ENCODE == PDM_LOOKUP)

static void sound_isr(void) {
    // Pybricks: for now, driver expects sound to always repeat
    if (sample.count <= 0) {
        sample.count = sample.out_index;
        sample.out_index = 0;
    }

    if (sample.count > 0) {
        // if (*AT91C_SSC_TCR == 0) {
        //     sound_fill_sample_buffer();
        //     *AT91C_SSC_TPR = (unsigned int)sample.buf[sample.buf_id];
        //     *AT91C_SSC_TCR = sample.len;
        //     sample.count--;
        // }

        sound_fill_sample_buffer();
        *AT91C_SSC_TNPR = (unsigned int)sample.buf[sample.buf_id];
        *AT91C_SSC_TNCR = sample.len;
        sample.count--;

        // If this is the last sample wait for it to complete, otherwise wait
        // to switch buffers
        sound_interrupt_enable(AT91C_SSC_ENDTX);
    } else {
        sound_disable();
        sound_interrupt_disable();
    }
}

void pbdrv_sound_init(void) {
    uint32_t state = nx_interrupts_disable();

    // Initialise the hardware. We make use of the SSC module.
    sound_interrupt_disable();
    sound_disable();

    *AT91C_PMC_PCER = (1 << AT91C_ID_SSC);

    *AT91C_PIOA_ODR = AT91C_PA17_TD;
    *AT91C_PIOA_OWDR = AT91C_PA17_TD;
    *AT91C_PIOA_MDDR = AT91C_PA17_TD;
    *AT91C_PIOA_PPUDR = AT91C_PA17_TD;
    *AT91C_PIOA_IFDR = AT91C_PA17_TD;
    *AT91C_PIOA_CODR = AT91C_PA17_TD;
    *AT91C_PIOA_IDR = AT91C_PA17_TD;

    *AT91C_SSC_CR = AT91C_SSC_SWRST;
    *AT91C_SSC_TCMR = AT91C_SSC_CKS_DIV + AT91C_SSC_CKO_CONTINOUS + AT91C_SSC_START_CONTINOUS;
    *AT91C_SSC_TFMR = 31 + (7 << 8) + AT91C_SSC_MSBF; // 8 32-bit words
    *AT91C_SSC_CR = AT91C_SSC_TXEN;

    nx_aic_install_isr(AT91C_ID_SSC, AIC_PRIO_DRIVER, AIC_TRIG_EDGE, sound_isr);

    sample.buf_id = 0;

    nx_interrupts_enable(state);
}

void pbdrv_sound_start(const uint16_t *data, uint32_t length, uint32_t sample_rate) {
    if (data == NULL || length == 0) {
        return;
    }

    if (sample_rate > MAX_RATE) {
        sample_rate = MAX_RATE;
    }

    if (sample_rate < MIN_RATE) {
        sample_rate = MIN_RATE;
    }

    // Turn off ints while we update shared values
    sound_interrupt_disable();
    sample.count = (length + SAMPLE_PER_BUF - 1) / SAMPLE_PER_BUF;
    sample.out_index = 0;
    sample.in_index = length;
    sample.ptr = data;
    sample.len = PDM_BUFFER_LENGTH;

    // Calculate the clock divisor based upon the recorded sample frequency
    *AT91C_SSC_CMR = (OSC / (2 * SAMPLE_BITS) + sample_rate / 2) / sample_rate;

    // re-enable and wait for the current sample to complete
    sound_enable();
    sound_interrupt_enable(AT91C_SSC_TXBUFE);
    *AT91C_SSC_PTCR = AT91C_PDC_TXTEN;
}

void pbdrv_sound_stop(void) {
    sound_disable();
    sound_interrupt_disable();
}

#endif // PBDRV_CONFIG_SOUND_NXT
