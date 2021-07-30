// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "fft_stratus.h"
#include "vitdodec_stratus.h"
#include "conv2d_stratus.h"
#include "gemm_stratus.h"
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

struct fft_stratus_access fft_cfg_001[] = {
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

struct vitdodec_stratus_access vitdodec_cfg_001[] = {
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

struct conv2d_stratus_access conv2d_cfg_001[] = {
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

struct gemm_stratus_access gemm_cfg_001[] = {
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


#endif /* __ESP_CFG_000_H__ */
