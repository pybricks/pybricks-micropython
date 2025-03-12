/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
const unsigned short me_encode_tab[256] = {
    0x5555, 0x5556, 0x5559, 0x555a, 0x5565, 0x5566, 0x5569, 0x556a, 0x5595,
    0x5596, 0x5599, 0x559a, 0x55a5, 0x55a6, 0x55a9, 0x55aa, 0x5655, 0x5656,
    0x5659, 0x565a, 0x5665, 0x5666, 0x5669, 0x566a, 0x5695, 0x5696, 0x5699,
    0x569a, 0x56a5, 0x56a6, 0x56a9, 0x56aa, 0x5955, 0x5956, 0x5959, 0x595a,
    0x5965, 0x5966, 0x5969, 0x596a, 0x5995, 0x5996, 0x5999, 0x599a, 0x59a5,
    0x59a6, 0x59a9, 0x59aa, 0x5a55, 0x5a56, 0x5a59, 0x5a5a, 0x5a65, 0x5a66,
    0x5a69, 0x5a6a, 0x5a95, 0x5a96, 0x5a99, 0x5a9a, 0x5aa5, 0x5aa6, 0x5aa9,
    0x5aaa, 0x6555, 0x6556, 0x6559, 0x655a, 0x6565, 0x6566, 0x6569, 0x656a,
    0x6595, 0x6596, 0x6599, 0x659a, 0x65a5, 0x65a6, 0x65a9, 0x65aa, 0x6655,
    0x6656, 0x6659, 0x665a, 0x6665, 0x6666, 0x6669, 0x666a, 0x6695, 0x6696,
    0x6699, 0x669a, 0x66a5, 0x66a6, 0x66a9, 0x66aa, 0x6955, 0x6956, 0x6959,
    0x695a, 0x6965, 0x6966, 0x6969, 0x696a, 0x6995, 0x6996, 0x6999, 0x699a,
    0x69a5, 0x69a6, 0x69a9, 0x69aa, 0x6a55, 0x6a56, 0x6a59, 0x6a5a, 0x6a65,
    0x6a66, 0x6a69, 0x6a6a, 0x6a95, 0x6a96, 0x6a99, 0x6a9a, 0x6aa5, 0x6aa6,
    0x6aa9, 0x6aaa, 0x9555, 0x9556, 0x9559, 0x955a, 0x9565, 0x9566, 0x9569,
    0x956a, 0x9595, 0x9596, 0x9599, 0x959a, 0x95a5, 0x95a6, 0x95a9, 0x95aa,
    0x9655, 0x9656, 0x9659, 0x965a, 0x9665, 0x9666, 0x9669, 0x966a, 0x9695,
    0x9696, 0x9699, 0x969a, 0x96a5, 0x96a6, 0x96a9, 0x96aa, 0x9955, 0x9956,
    0x9959, 0x995a, 0x9965, 0x9966, 0x9969, 0x996a, 0x9995, 0x9996, 0x9999,
    0x999a, 0x99a5, 0x99a6, 0x99a9, 0x99aa, 0x9a55, 0x9a56, 0x9a59, 0x9a5a,
    0x9a65, 0x9a66, 0x9a69, 0x9a6a, 0x9a95, 0x9a96, 0x9a99, 0x9a9a, 0x9aa5,
    0x9aa6, 0x9aa9, 0x9aaa, 0xa555, 0xa556, 0xa559, 0xa55a, 0xa565, 0xa566,
    0xa569, 0xa56a, 0xa595, 0xa596, 0xa599, 0xa59a, 0xa5a5, 0xa5a6, 0xa5a9,
    0xa5aa, 0xa655, 0xa656, 0xa659, 0xa65a, 0xa665, 0xa666, 0xa669, 0xa66a,
    0xa695, 0xa696, 0xa699, 0xa69a, 0xa6a5, 0xa6a6, 0xa6a9, 0xa6aa, 0xa955,
    0xa956, 0xa959, 0xa95a, 0xa965, 0xa966, 0xa969, 0xa96a, 0xa995, 0xa996,
    0xa999, 0xa99a, 0xa9a5, 0xa9a6, 0xa9a9, 0xa9aa, 0xaa55, 0xaa56, 0xaa59,
    0xaa5a, 0xaa65, 0xaa66, 0xaa69, 0xaa6a, 0xaa95, 0xaa96, 0xaa99, 0xaa9a,
    0xaaa5, 0xaaa6, 0xaaa9, 0xaaaa,
};
const unsigned char me_decode_tab[256] = {
    0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2,
    0x2, 0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x0, 0x0,
    0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2, 0x3,
    0x3, 0x2, 0x2, 0x3, 0x3, 0x4, 0x4, 0x5, 0x5,
    0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6,
    0x6, 0x7, 0x7, 0x4, 0x4, 0x5, 0x5, 0x4, 0x4,
    0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6, 0x6, 0x7,
    0x7, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1,
    0x2, 0x2, 0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x0,
    0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2,
    0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x4, 0x4, 0x5,
    0x5, 0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7,
    0x6, 0x6, 0x7, 0x7, 0x4, 0x4, 0x5, 0x5, 0x4,
    0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6, 0x6,
    0x7, 0x7, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9,
    0x9, 0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb,
    0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9, 0x9, 0xa,
    0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb, 0xc, 0xc,
    0xd, 0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf,
    0xf, 0xe, 0xe, 0xf, 0xf, 0xc, 0xc, 0xd, 0xd,
    0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf, 0xf, 0xe,
    0xe, 0xf, 0xf, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8,
    0x9, 0x9, 0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb,
    0xb, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9, 0x9,
    0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb, 0xc,
    0xc, 0xd, 0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe,
    0xf, 0xf, 0xe, 0xe, 0xf, 0xf, 0xc, 0xc, 0xd,
    0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf, 0xf,
    0xe, 0xe, 0xf, 0xf,
};
const unsigned char me_valid_tab[256] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1,
    0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0,
    0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
};
