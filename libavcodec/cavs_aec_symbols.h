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

#ifndef AVCODEC_CAVS_AEC_SYMBOLS_H
#define AVCODEC_CAVS_AEC_SYMBOLS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // TODO REMOVE?
#include "aec.h"
#include "get_bits.h"


#define AEC_CONTEXTS                  323

enum aec_ctx_offset {
  MB_SKIP_RUN = 0,
  MB_TYPE = 4,
  MB_PART_TYPE = 19,
  INTRA_LUMA_PRED_MODE = 22,
  INTRA_CHROMA_PRED_MODE = 26,
  MB_REFERENCE_INDEX = 30,
  MV_DIFF_X = 36,
  MV_DIFF_Y = 42,
  CBP = 48,
  MB_QP_DELTA = 54,
  TRANS_COEFFICIENT_FRAME_LUMA = 58,
  TRANS_COEFFICIENT_FRAME_CHROMA = 124,
  TRANS_COEFFICIENT_FIELD_LUMA = 190,
  TRANS_COEFFICIENT_FIELD_CHROMA = 256,
  WEIGHTING_PREDICTION = 322
};

typedef struct Aec {
  AecDec aecdec;
  AecCtx aecctx[AEC_CONTEXTS];
} Aec;


int cavs_aec_read_mb_reference_index(Aec *aec, GetBitContext *gb, int refA, int refB);
int cavs_aec_read_mv_diff(Aec *aec, GetBitContext *gb, int base_ctx, int mvda);
//int cavs_aec_read_skip_run(Aec *aec, GetBitContext *gb, int base_ctx);
int cavs_aec_read_intra_luma_pred_mode(Aec *aec, GetBitContext *gb, int predpred);
int cavs_aec_read_intra_chroma_pred_mode(Aec *aec, GetBitContext *gb, int a, int b);
int cavs_aec_read_cbp(Aec *aec, GetBitContext *gb, int a_cbp, int b_cbp, bool a_avail, bool b_avail);
int cavs_aec_read_qp_delta(Aec *aec, GetBitContext *gb, int qp_delta_last);

#endif /* AVCODEC_CAVS_AEC_SYMBOLS_H */
