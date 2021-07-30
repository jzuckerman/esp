// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "fft_stratus.h"
#include "cholesky_stratus.h"
#include "mriq_stratus.h"
#include "nightvision_stratus.h"
#include "sort_stratus.h"
#include "spmv_stratus.h"
#include "vitdodec_stratus.h"
#include "conv2d_stratus.h"
#include "gemm_stratus.h"
#include "svhn_mlp_hls4ml.h"
typedef uint32_t token_t;

//---------------FFT------------

#define FX_IL 12

/* <<--params-def-->> */
#define LOG_LEN 11
#define LEN (1 << LOG_LEN)
#define DO_BITREV 1
#define NUM_BATCHES 1
/* <<--params-->> */
const int32_t do_bitrev = DO_BITREV;
int32_t len = LEN;
int32_t log_len = LOG_LEN;
int32_t num_batches = 1;


struct fft_stratus_access fft_cfg_000[] = {
	{
		.batch_size = NUM_BATCHES,
        .do_bitrev = DO_BITREV,
		.log_len = LOG_LEN,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_NONE,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
        .esp.devid = 0, 
        .esp.learn = 0,
    }
};

//-----------CHOLEKSY------------

#define INPUT_ROWS 64
#define OUTPUT_ROWS 64

/* <<--params-->> */
int32_t input_rows = INPUT_ROWS;
int32_t output_rows = OUTPUT_ROWS;

struct cholesky_stratus_access cholesky_cfg_000[] = {
         /* <<--descriptor-->> */
    {
        .input_rows = INPUT_ROWS,
        .output_rows = OUTPUT_ROWS,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_FULL,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
        .esp.devid = 0,
        .esp.learn = 0,
    }
};


//----------MRI-Q------------

#define NUM_BATCH_K 8
#define BATCH_SIZE_K 128
#define NUM_BATCH_X 8
#define BATCH_SIZE_X 128

/* <<--params-->> */
int32_t num_batch_k = NUM_BATCH_K;
int32_t batch_size_k = BATCH_SIZE_K;
int32_t num_batch_x = NUM_BATCH_X;
int32_t batch_size_x = BATCH_SIZE_X;

struct mriq_stratus_access mriq_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .num_batch_k = NUM_BATCH_K,
        .batch_size_k = BATCH_SIZE_K,
        .num_batch_x = NUM_BATCH_X,
        .batch_size_x = BATCH_SIZE_X,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};

//----------Nightvision--------

#define DO_DWT 0
#define NV_NROWS 32
#define NV_NCOLS 32
#define NIMAGES 2

const int32_t do_dwt = DO_DWT;
const int32_t nv_nrows = NV_NROWS;
const int32_t nv_ncols = NV_NCOLS;
int32_t nimages;

struct nightvision_stratus_access nightvision_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .rows = NV_NROWS,
        .cols = NV_NCOLS,
        .do_dwt = DO_DWT,
        .nimages = NIMAGES,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};

//----------Sort---------------

#define SORT_SIZE 64
#define SORT_BATCH 64

int32_t sort_size = SORT_SIZE;
int32_t sort_batch = SORT_BATCH;

struct sort_stratus_access sort_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .batch = SORT_BATCH,
        .size = SORT_SIZE,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};


//----------SPMV---------------

#define SPMV_NROWS 256
#define SPMV_NCOLS 512
#define SPMV_MAX_NZ 8
#define MTX_LEN 1184
#define VALS_PLM_SIZE 1024
#define VECT_FITS_PLM 0

int32_t spmv_nrows = SPMV_NROWS;
int32_t spmv_ncols = SPMV_NCOLS;
int32_t max_nonzero = SPMV_MAX_NZ;
int32_t mtx_len = MTX_LEN;
const int32_t vals_plm_size = VALS_PLM_SIZE;
const int32_t vect_fits_plm = VECT_FITS_PLM;

struct spmv_stratus_access spmv_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .nrows = SPMV_NROWS,
        .ncols = SPMV_NCOLS,
        .max_nonzero = SPMV_MAX_NZ,
        .mtx_len = MTX_LEN,
        .vals_plm_size = VALS_PLM_SIZE, 
        .vect_fits_plm = VECT_FITS_PLM,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
        .esp.devid = 1,
        .esp.learn = 0,
    }
};


//----------Vitdodec-----------

/* <<--params-def-->> */
#define NBATCHES 1
#define CBPS 48
#define NTRACEBACK 5
#define DATA_BITS 288

/* <<--params-->> */
int32_t nbatches = NBATCHES;
const int32_t cbps = CBPS;
const int32_t ntraceback = NTRACEBACK;
const int32_t data_bits = DATA_BITS;

struct vitdodec_stratus_access vitdodec_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .nbatches = NBATCHES,
        .cbps = CBPS,
        .ntraceback = NTRACEBACK,
        .data_bits = DATA_BITS,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
        .esp.devid = 1,
        .esp.learn = 0,
    }
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "fft_stratus.0",
		.ioctl_req = FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(fft_cfg_000[0].esp),
	}
};

//--------CONV2D-------

#define N_CHANNELS 2
#define FEATURE_MAP_HEIGHT 6
#define FEATURE_MAP_WIDTH 6
#define N_FILTERS 2
#define FILTER_DIM 3
#define IS_PADDED 1
#define STRIDE 1
#define DO_RELU 0
#define POOL_TYPE 0
#define BATCH_SIZE 1

struct conv2d_stratus_access conv2d_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .n_channels = N_CHANNELS,
        .feature_map_height = FEATURE_MAP_HEIGHT,
        .feature_map_width = FEATURE_MAP_WIDTH,
        .n_filters = N_FILTERS,
        .filter_dim = FILTER_DIM,
        .is_padded = IS_PADDED,
        .stride = STRIDE,
        .do_relu = DO_RELU,
        .pool_type = POOL_TYPE,
        .batch_size = BATCH_SIZE,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};

//---------GEMM---------------

#define DO_RELU 0
#define TRANSPOSE 1
#define NINPUTS 2
#define D3 8
#define D2 8
#define D1 8
#define ST_OFFSET (NINPUTS * (D1 * D2 + D2 * D3))
#define LD_OFFSET1 0
#define LD_OFFSET2 (NINPUTS * (D1 * D2))

struct gemm_stratus_access gemm_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .do_relu = DO_RELU,
        .transpose = TRANSPOSE,
        .ninputs = NINPUTS,
        .d3 = D3,
        .d2 = D2,
        .d1 = D1,
        .st_offset = ST_OFFSET,
        .ld_offset1 = LD_OFFSET1,
        .ld_offset2 = LD_OFFSET2,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};

//----------MLP--------------

#define NBURSTS 9

struct svhn_mlp_hls4ml_access svhn_mlp_cfg_000[] = {
    {
        /* <<--descriptor-->> */
        .nbursts = NBURSTS,
        .src_offset = 0,
        .dst_offset = 0,
        .esp.coherence = ACC_COH_NONE,
        .esp.p2p_store = 0,
        .esp.p2p_nsrcs = 0,
        .esp.p2p_srcs = {"", "", "", ""},
    }
};

#endif /* __ESP_CFG_000_H__ */
