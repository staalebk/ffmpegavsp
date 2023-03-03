/*
 * Advanced Entropy Code (AEC) decoder.
 * Copyright (c) 2023 Ståle Kristoffersen <staalebk@gmail.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_AEC_H
#define AVCODEC_AEC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "get_bits.h"

typedef struct AecCtx {
    uint8_t  mps;        //  1 bit  Most Probable Symbol
    uint8_t  cycno;      //  2 bits
    uint16_t lgPmps;     // 11 bits Most Probable Symbol (log)
} AecCtx;

typedef struct AecDec {
    uint32_t rS1;       // precision is smallest int equal to or greater than Log(boundS+2), init = 0, (if boundS == 254 this is 8bits)
    uint32_t rT1;       // precision 8 bits, init = 0xFF
    uint32_t valueS;    // precision is smallest int equal to or greater than Log(boundS+1) (if boundS == 254 this is 8bits)
    uint32_t valueT;    // 9 bits
    uint32_t bFlag;     // 1 or 2
    //uint8_t cFlag;      // 1 or 2
    uint32_t boundS;    // greater than 0, precision is smallest int equal to or greater than Log(boundS+1)
    //uint8_t valueD;     // 1 or 2
    bool initialized;
} AecDec;

void aec_init_aecdec(AecDec *aecdec, GetBitContext *gb);
void aec_init_aecctx(AecCtx *aecctx);
void update_ctx(int binVal, AecCtx *ctx);
int aec_decode_bin(AecDec *aecdec, GetBitContext *gb, int contextWeighting, AecCtx *ctx1, AecCtx *ctx2);
int aec_decode_bin_debug(AecDec *aecdec, GetBitContext *gb, int contextWeighting, AecCtx *ctx1, AecCtx *ctx2, bool dbg);
int aec_decode_bypass(AecDec *aecdec, GetBitContext *gb);
int aec_decode_bypass_debug(AecDec *aecdec, GetBitContext *gb, bool dbg);
void aec_debug(AecDec *aecdec, AecCtx *ctx1, AecCtx *ctx2);
int aec_decode_stuffing_bit(AecDec *aecdec, GetBitContext *gb, bool dbg);

#endif /* AVCODEC_AEC_H */