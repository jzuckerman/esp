// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "svhn_mlp_hls4ml.h"
#include "svhn_autoenc_hls4ml.h"
#include "nightvision_stratus.h"
typedef uint32_t token_t;


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

struct svhn_mlp_hls4ml_access svhn_mlp_cfg_001[] = {
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

struct svhn_mlp_hls4ml_access svhn_mlp_cfg_002[] = {
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

struct svhn_mlp_hls4ml_access svhn_mlp_cfg_003[] = {
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

#define DO_DWT 0
#define NV_NROWS 32
#define NV_NCOLS 32
#define NIMAGES 2

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
struct nightvision_stratus_access nightvision_cfg_001[] = {
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
struct nightvision_stratus_access nightvision_cfg_002[] = {
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

struct nightvision_stratus_access nightvision_cfg_003[] = {
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

struct svhn_autoenc_hls4ml_access svhn_autoenc_cfg_000[] = {
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
