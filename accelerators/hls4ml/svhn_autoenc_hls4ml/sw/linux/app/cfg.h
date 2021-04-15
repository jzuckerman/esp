// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "svhn_autoenc_hls4ml.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define NBURSTS 

/* <<--params-->> */
const int32_t nbursts = NBURSTS;

#define NACC 1

#define INT_BITS 22
#define fl2fx(A) float_to_fixed32(A, INT_BITS)

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

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "svhn_autoenc_hls4ml.0",
		.ioctl_req = SVHN_AUTOENC_HLS4ML_IOC_ACCESS,
		.esp_desc = &(svhn_autoenc_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
