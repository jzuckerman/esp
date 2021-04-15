// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _SVHN_AUTOENC_HLS4ML_H_
#define _SVHN_AUTOENC_HLS4ML_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include <esp.h>
#include <esp_accelerator.h>

struct svhn_autoenc_hls4ml_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned nbursts;
	unsigned src_offset;
	unsigned dst_offset;
};

#define SVHN_AUTOENC_HLS4ML_IOC_ACCESS	_IOW ('S', 0, struct svhn_autoenc_hls4ml_access)

#endif /* _SVHN_AUTOENC_HLS4ML_H_ */
