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

#include "aec.h"
#include "get_bits.h"

int dbg_get_bits(GetBitContext *gb, int n, const char *c, bool dbg);

int dbg_get_bits(GetBitContext *gb, int n, const char *c, bool dbg){
    int b = get_bits(gb,n);
    if (dbg)
        printf("Read: %d    %s\n", b, c);
    return(b);
}

void aec_init_aecdec(AecDec *aecdec, GetBitContext *gb) {
    printf("Initializing AEC decoder!\n");
    aecdec->rS1 = 0;
    aecdec->rT1 = 0xFF;
    aecdec->boundS = 0xFE;
    aecdec->valueS = 0;
    aecdec->valueT = dbg_get_bits(gb, 9, "init", true);
    while(!((aecdec->valueT>>8) & 0x01) && aecdec->valueS < aecdec->boundS) {
        aecdec->valueT = (aecdec->valueT << 1) | dbg_get_bits(gb, 1, "init2", true);
        aecdec->valueS++;
    }
    if(aecdec->valueT < 0x100)
        aecdec->bFlag = 1;
    else
        aecdec->bFlag = 0;
    aecdec->valueT = aecdec->valueT & 0xFF;
    aecdec->initialized = true;
}

void aec_init_aecctx(AecCtx *aecctx) {
    aecctx->mps = 0;
    aecctx->cycno = 0;
    aecctx->lgPmps = 1023;
}

void update_ctx(int binVal, AecCtx *ctx)
{
    int cwr;
	if (ctx->cycno <= 1)
		cwr = 3;
	else if (ctx->cycno == 2)
		cwr = 4;
	else
		cwr = 5;
	if (binVal != ctx->mps) {
		if (ctx->cycno <= 2)
			ctx->cycno = ctx->cycno + 1;
		else
			ctx->cycno = 3;
	} else if (ctx->cycno == 0)
		ctx->cycno = 1;
	if (binVal == ctx->mps) {
		ctx->lgPmps = ctx->lgPmps - (ctx->lgPmps >> cwr) - (ctx->lgPmps >> (cwr+2));
	} else {
		switch (cwr) {
			case 3:
				ctx->lgPmps = ctx->lgPmps + 197;
				break;
			case 4:
				ctx->lgPmps = ctx->lgPmps + 95;
				break;
			default:
				ctx->lgPmps = ctx->lgPmps + 46;
		}
		if (ctx->lgPmps > 1023) {
			ctx->lgPmps = 2047 - ctx->lgPmps;
			ctx->mps = ! (ctx->mps);
		}
	}
	return;
}

int aec_decode_bin(AecDec *aecdec, GetBitContext *gb, int contextWeighting, AecCtx *ctx, AecCtx *ctx2) {
    return aec_decode_bin_debug(aecdec, gb, contextWeighting, ctx, ctx2, false);
}

int aec_decode_bin_debug(AecDec *aecdec, GetBitContext *gb, int contextWeighting, AecCtx *ctx, AecCtx *ctx2, bool dbg) {
    int predMps;
    int lgPmps;
    int rS2;
    int rT2;
    int sFlag;
    int binVal;
    int tRlps;

    if (!aecdec->initialized) {
        aec_init_aecdec(aecdec, gb);
    }
    if (dbg) {
        printf("ctx: %d %d %d \taec: %d %d %d %d %d %d\n",ctx->cycno, ctx->lgPmps, ctx->mps, aecdec->bFlag, aecdec->boundS, aecdec->rS1, aecdec->rT1, aecdec->valueS, aecdec->valueT);
    }

	if ( contextWeighting == 1 ) {
		if ( ctx->mps == ctx2->mps ) {
			predMps = ctx->mps;
			lgPmps = (ctx->lgPmps + ctx2->lgPmps) / 2;
		} else {
			if ( ctx->lgPmps < ctx2->lgPmps ) {
				predMps = ctx->mps;
				lgPmps = 1023 - ((ctx2->lgPmps - ctx->lgPmps) >> 1);
			} else {
				predMps = ctx2->mps;
				lgPmps = 1023 - ((ctx->lgPmps - ctx2->lgPmps) >> 1);
			}
		}
	} else {
		predMps = ctx->mps;
		lgPmps = ctx->lgPmps;
	}
	if ( aecdec->rT1 >= (lgPmps >> 2) ) {
		rS2 = aecdec->rS1;
		rT2 = aecdec->rT1 - (lgPmps >> 2 );
		sFlag = 0;
	} else {
		rS2 = aecdec->rS1 + 1;
		rT2 = 256 + aecdec->rT1 - (lgPmps >> 2);
		sFlag = 1;
	}
	if ( rS2 > aecdec->valueS || (rS2 == aecdec->valueS && aecdec->valueT >= rT2) && aecdec->bFlag == 0 ) {
		binVal = ! predMps;
		if ( sFlag == 0 )
			tRlps = lgPmps >> 2;
		else
			tRlps = aecdec->rT1 + (lgPmps >> 2);
		if ( rS2 == aecdec->valueS )
			aecdec->valueT = aecdec->valueT - rT2;
		else
			aecdec->valueT = 256 + ((aecdec->valueT << 1 ) | dbg_get_bits(gb,1, "b1", dbg)) - rT2;
		while ( tRlps < 0x100 ) {
			tRlps = tRlps << 1;
			aecdec->valueT = (aecdec->valueT << 1 ) | dbg_get_bits(gb,1,"b2", dbg);
		}
		aecdec->rT1 = tRlps & 0xFF;
	} else {
		binVal = predMps;
		aecdec->rS1 = rS2;
		aecdec->rT1 = rT2;
	}
	if ( binVal != predMps || ( binVal == predMps && aecdec->bFlag == 1 && rS2 == aecdec->boundS ) ) {
		aecdec->rS1 = 0;
		aecdec->valueS = 0;
		while ( aecdec->valueT < 0x100 ) {
			aecdec->valueS++;
			aecdec->valueT = (aecdec->valueT << 1 ) | dbg_get_bits(gb,1,"b3", dbg);
		}
		if ( aecdec->valueT < 0x100 )
			aecdec->bFlag = 1;
		else
			aecdec->bFlag = 0;
		aecdec->valueT = aecdec->valueT & 0xFF;
	}
	if ( contextWeighting == 1 ) {
		update_ctx(binVal, ctx);
		update_ctx(binVal, ctx2);
	} else {
		//ctx = update_ctx(binVal, ctx);
        update_ctx(binVal, ctx);
	}
    if (dbg) {
        printf("aec_decode_bin:\t%d\n", binVal);
    }

	return (binVal);
}

int aec_decode_bypass(AecDec *aecdec, GetBitContext *gb) {
    return aec_decode_bypass_debug(aecdec, gb, false);
}

int aec_decode_bypass_debug(AecDec *aecdec, GetBitContext *gb, bool dbg) {
    int predMps = 0;
    int lgPmps = 1023;
    int rS2;
    int rT2;
    int sFlag;
    int binVal;
    int tRlps;
    if ( aecdec->rT1 >= (lgPmps >> 2) ) {
        rS2 = aecdec->rS1;
        rT2 = aecdec->rT1 - (lgPmps >> 2 );
        sFlag = 0;
    } else {
        rS2 = aecdec->rS1 + 1;
        rT2 = 256 + aecdec->rT1 - (lgPmps >> 2);
        sFlag = 1;
    }
    if( rS2 > aecdec->valueS || (rS2 == aecdec->valueS && aecdec->valueT >= rT2) ) {
        binVal = ! predMps;
        if ( sFlag == 0 )
            tRlps = lgPmps >> 2;
        else
            tRlps = aecdec->rT1 + (lgPmps >> 2);
        if ( rS2 == aecdec->valueS )
            aecdec->valueT = aecdec->valueT - rT2;
        else
            aecdec->valueT = ((aecdec->valueT << 1 ) | dbg_get_bits(gb,1,"by1", dbg)) - rT2 + 256;
        while ( tRlps < 0x100 ) {
            tRlps = tRlps << 1;
            aecdec->valueT = (aecdec->valueT << 1 ) | dbg_get_bits(gb,1,"by2", dbg);
        }
        aecdec->rS1 = 0;
        aecdec->rT1 = tRlps & 0xFF;
        aecdec->valueS = 0;
        while ( aecdec->valueT < 0x100 ) {
            aecdec->valueS++;
            aecdec->valueT = (aecdec->valueT << 1 ) | dbg_get_bits(gb,1,"by3", dbg);
        }
        aecdec->valueT = aecdec->valueT & 0xFF;
    } else {
        binVal = predMps;
        aecdec->rS1 = rS2;
        aecdec->rT1 = rT2;
    }
    if (dbg)
        printf("aec_decode_bypass:\t%d\n", binVal);
    return (binVal);
}

int aec_decode_stuffing_bit(AecDec *aecdec, GetBitContext *gb, bool dbg) {
    int predMps = 0;
    int lgPmps = 4;
    int rS2;
    int rT2;
    int sFlag;
    int binVal;
    int tRlps;

    if ( aecdec->rT1 >= (lgPmps >> 2) ) {
        rS2 = aecdec->rS1;
        rT2 = aecdec->rT1 - (lgPmps >> 2 );
        sFlag = 0;
    } else {
        rS2 = aecdec->rS1 + 1;
        rT2 = 256 + aecdec->rT1 - (lgPmps >> 2);
        sFlag = 1;
    }
    if( rS2 > aecdec->valueS || (rS2 == aecdec->valueS && aecdec->valueT >= rT2) ) {
        binVal = ! predMps;
        if ( sFlag == 0 )
            tRlps = lgPmps >> 2;
        else
            tRlps = aecdec->rT1 + (lgPmps >> 2);
        if ( rS2 == aecdec->valueS )
            aecdec->valueT = aecdec->valueT - rT2;
        else
            aecdec->valueT = 256 + ((aecdec->valueT << 1) | dbg_get_bits(gb,1,"sb1", dbg)) - rT2;
        while ( tRlps < 0x100 ) {
            tRlps = tRlps << 1;
            aecdec->valueT = (aecdec->valueT << 1) | dbg_get_bits(gb,1,"sb2", dbg);
        }
        aecdec->rS1 = 0;
        aecdec->rT1 = tRlps & 0xFF;
        aecdec->valueS = 0;
        while ( aecdec->valueT < 0x100 ) {
            aecdec->valueS++;
            aecdec->valueT = (aecdec->valueT << 1) | dbg_get_bits(gb,1,"sb3", dbg);
        }
        aecdec->valueT = aecdec->valueT & 0xFF;
    } else {
        binVal = predMps;
        aecdec->rS1 = rS2;
        aecdec->rT1 = rT2;
    }
    return (binVal);
}

void aec_debug(AecDec *aecdec, AecCtx *ctx1, AecCtx *ctx2) {
    printf("AEC: %d %d %d %d %d %d\n", -1, -1, aecdec->rS1, aecdec->rT1, aecdec->valueS, aecdec->valueT);
    if (ctx1 != NULL)
        printf("ctx: %d %d %d\n",ctx1->cycno, ctx1->lgPmps, ctx1->mps);
    if (ctx2 != NULL)
        printf("ctx2: %d %d %d\n",ctx2->cycno, ctx2->lgPmps, ctx2->mps);
}