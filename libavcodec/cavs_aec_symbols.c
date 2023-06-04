/*
 * Advanced Entropy Code (AEC) symbol decoder.
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
#include "cavs_aec_symbols.h"

int cavs_aec_read_mb_reference_index(Aec *aec, GetBitContext *gb, int refA, int refB) {
    int ctx;
    int symbol = 0;
    if(refA > 0)
        refA = 1;
    else
        refA = 0;
    if(refB > 0)
        refB = 1;
    else   
        refB = 0;
    ctx = refA + 2 * refB;
    while(!aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[MB_REFERENCE_INDEX + ctx], NULL, false)) {
        symbol++;
        if(symbol == 1)
            ctx = 4;
        else
            ctx = 5;
        if(symbol == 3)
            break;
    }
    aec_log(&aec->aecdec, "ref_frame", symbol);
    //aec_log(&aec->aecdec, "ref_frameA", refA);
    //aec_log(&aec->aecdec, "ref_frameB", refB);
    return symbol;
}

int cavs_aec_read_mb_reference_index_b(Aec *aec, GetBitContext *gb, int refA, int refB) {
    int ctx;
    int symbol = 0;
    // TODO: Remove
    if(refA == -1)
        refA = 0;
    if(refB == -1)
        refB = 0;
    aec_log(&aec->aecdec, "refA", refA);
    aec_log(&aec->aecdec, "refB", refB);
    if(refA > 0)
        refA = 1;
    else
        refA = 0;
    if(refB > 0)
        refB = 1;
    else   
        refB = 0;
    ctx = refA + 2 * refB;
    symbol = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[MB_REFERENCE_INDEX + ctx], NULL, false);
    if(symbol)
        symbol = 0;
    else
        symbol = 1;
    aec_log(&aec->aecdec, "ref_frame", symbol);
    //aec_log(&aec->aecdec, "ref_frameA", refA);
    //aec_log(&aec->aecdec, "ref_frameB", refB);
    return symbol;
}

int cavs_aec_read_mb_skip_run(Aec *aec, GetBitContext *gb) {
    int symbol = 0;
    int ctx = 0;
    while(!aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[MB_SKIP_RUN+ctx], NULL, false)) {
        symbol++;
        ctx++;
        if(ctx > 3)
            ctx = 3;
    }
    aec_log(&aec->aecdec, "mb_skip_run", symbol);
    return symbol;
}

int cavs_aec_read_mv_diff(Aec *aec, GetBitContext *gb, int base_ctx, int mvda) {
    int ctx = 0;
    int value = 0;
    int sign = 0;
    if (mvda > 1)
        ctx = 1;
    if (mvda > 15)
        ctx = 2;
    //aec_log(&aec->aecdec, "mvda", mvda);
    //aec_log(&aec->aecdec, "ctx", ctx);
    if (!aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[base_ctx + ctx], NULL, false))
        value = 0;
    else if (!aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[base_ctx + 3], NULL, false))
        value = 1;
    else if (!aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[base_ctx + 4], NULL, false))
        value = 2;
    else {
        int exgolomb = 0;
        int pre = 0;
        int post = 0;
        value = 3;
        value += aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[base_ctx + 5], NULL, false);
        while(!aec_decode_bypass(&aec->aecdec, gb)) {
            pre += (1<<exgolomb);
            exgolomb++;
        }
        
        while(exgolomb--) {
            post |= aec_decode_bypass(&aec->aecdec, gb)<<exgolomb;
        }
        value = value + (pre + post)*2;
        //aec_log(&aec->aecdec, "FMVD loong", value);
    }
    if (value) {
        sign = aec_decode_bypass(&aec->aecdec, gb);
        value = sign ? -value : value;
    }
    aec_log(&aec->aecdec, "FMVD", value);
    return value;
}

int cavs_aec_read_intra_luma_pred_mode(Aec *aec, GetBitContext *gb, int predpred) {
    int ctxIdxInc = 0;
    int intra_luma_pred_mode = 0;
    while(aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[INTRA_LUMA_PRED_MODE + ctxIdxInc], NULL, false) == 0) {
        intra_luma_pred_mode++;
        ctxIdxInc++;
        if (intra_luma_pred_mode == 4)
            break;
    }

    aec_log(&aec->aecdec, "intra_luma_pred_mode", intra_luma_pred_mode);
    if( intra_luma_pred_mode == 0) {
        // Keep predpred
    } else {
        if (intra_luma_pred_mode == 4) {
            intra_luma_pred_mode = 0;
        }
        if (intra_luma_pred_mode < predpred) {
            predpred = intra_luma_pred_mode;
        } else {
            predpred = intra_luma_pred_mode + 1;
        }
    }
    return predpred;
}

int cavs_aec_read_intra_chroma_pred_mode(Aec *aec, GetBitContext *gb, int a, int b) {
    int ctxIdxInc;
    int symbol = 0;
    if(a == -1 || a == 0)
        a = 0;
    else
        a = 1;
    if(b == -1 || b == 0)
        b = 0;
    else
        b = 1;
    ctxIdxInc = a + b;
    aec_log(&aec->aecdec, "intra_chroma_pred_mode_ctx", ctxIdxInc);
    while(aec_decode_bin(&aec->aecdec, gb, 0, &aec->aecctx[INTRA_CHROMA_PRED_MODE + ctxIdxInc], NULL) != 0) {
        symbol++;
        ctxIdxInc = 3;
        if (symbol == 3)
            break;
    }
    aec_log(&aec->aecdec, "intra_chroma_pred_mode", symbol);
    return symbol;
}

int cavs_aec_read_cbp(Aec *aec, GetBitContext *gb, int a_cbp, int b_cbp, bool a_avail, bool b_avail) {
    int a = 0;
    int b = 0;
    int ctxIdxInc = 0;
    int bitpos = 0;
    int bit;
    int leftmask;
    int topmask;
    int cbp_code;

    cbp_code = 0;
    /** Read the first 4 bits of the CBP, checking if block A or B exists and has the CBP bit set*/
    /** bit 0*/
    leftmask = 1<<1;
    topmask = 1<<2;
    if(a_avail && !(a_cbp & leftmask)){
        a = 1;
    }
    if(b_avail && !(b_cbp & topmask)){
        b = 1;
    }
    ctxIdxInc = a + 2*b;
    bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
    cbp_code |= (bit<<bitpos);
    bitpos++;

    /** bit 1*/
    a = 0;
    b = 0;
    topmask = 1<<3;
    if(!cbp_code) {
        a = 1;
    }
    if(b_avail && !(b_cbp & topmask)){
        b = 1;
    }
    ctxIdxInc = a + 2*b;
    bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
    cbp_code |= (bit<<bitpos);
    bitpos++;

    /** bit 2*/
    a = 0;
    b = 0;
    leftmask = 1<<3;
    topmask = 1<<0;
    if(a_avail && !(a_cbp & leftmask)){
        a = 1;
    }
    if(!(cbp_code & topmask)){
        b = 1;
    }
    ctxIdxInc = a + 2*b;
    bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
    cbp_code |= (bit<<bitpos);
    bitpos++;

    /** bit 3*/
    a = 0;
    b = 0;
    leftmask = 1<<2;
    topmask = 1<<1;
    if(!(cbp_code & leftmask)){
        a = 1;
    }
    if(!(cbp_code & topmask)){
        b = 1;
    }
    ctxIdxInc = a + 2*b;
    bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
    cbp_code |= (bit<<bitpos);

    /** Read 1st Chroma bit */
    ctxIdxInc = 4;
    bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
    if(bit) {
        /** Read 2nd Chroma bit*/
        ctxIdxInc = 5;
        bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
        if(bit) {
            cbp_code |=(1<<4);
            cbp_code |=(1<<5);
        } else {
            /** Read 3rd Chroma bit*/
            bit = aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[CBP + ctxIdxInc], NULL, false);
            if(bit)
                cbp_code |= (1<<5);
            else
                cbp_code |= (1<<4);
        }
    }
    
    aec_log(&aec->aecdec, "CBP", cbp_code);
    return cbp_code;
}

int cavs_aec_read_qp_delta(Aec *aec, GetBitContext *gb, int qp_delta_last) {
    int ctxIdxInc = 0;
    int symbol = 0;
    if (qp_delta_last)
        ctxIdxInc = 1;
    while(aec_decode_bin_debug(&aec->aecdec, gb, 0, &aec->aecctx[MB_QP_DELTA + ctxIdxInc], NULL, false) == 0) {
        symbol++;
        if (symbol == 1)
            ctxIdxInc = 2;
        else
            ctxIdxInc = 3;
    }
    if(symbol%2 == 0)
        symbol = -(symbol + 1)/2;
    else
        symbol = (symbol + 1)/2;
    aec_log(&aec->aecdec, "QP Delta", symbol);
    return symbol;
}
