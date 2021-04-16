// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "svhn_denoise_hls4ml.h"

#define DRV_NAME	"svhn_denoise_hls4ml"

/* <<--regs-->> */
#define SVHN_DENOISE_NBURSTS_REG 0x40

struct svhn_denoise_hls4ml_device {
	struct esp_device esp;
};

static struct esp_driver svhn_denoise_driver;

static struct of_device_id svhn_denoise_device_ids[] = {
	{
		.name = "SLD_SVHN_DENOISE_HLS4ML",
	},
	{
		.name = "eb_091",
	},
	{
		.compatible = "sld,svhn_denoise_hls4ml",
	},
	{ },
};

static int svhn_denoise_devs;

static inline struct svhn_denoise_hls4ml_device *to_svhn_denoise(struct esp_device *esp)
{
	return container_of(esp, struct svhn_denoise_hls4ml_device, esp);
}

static void svhn_denoise_prep_xfer(struct esp_device *esp, void *arg)
{
	struct svhn_denoise_hls4ml_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->nbursts, esp->iomem + SVHN_DENOISE_NBURSTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool svhn_denoise_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct svhn_denoise_hls4ml_device *svhn_denoise = to_svhn_denoise(esp); */
	/* struct svhn_denoise_hls4ml_access *a = arg; */

	return true;
}

static int svhn_denoise_probe(struct platform_device *pdev)
{
	struct svhn_denoise_hls4ml_device *svhn_denoise;
	struct esp_device *esp;
	int rc;

	svhn_denoise = kzalloc(sizeof(*svhn_denoise), GFP_KERNEL);
	if (svhn_denoise == NULL)
		return -ENOMEM;
	esp = &svhn_denoise->esp;
	esp->module = THIS_MODULE;
	esp->number = svhn_denoise_devs;
	esp->driver = &svhn_denoise_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	svhn_denoise_devs++;
	return 0;
 err:
	kfree(svhn_denoise);
	return rc;
}

static int __exit svhn_denoise_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct svhn_denoise_hls4ml_device *svhn_denoise = to_svhn_denoise(esp);

	esp_device_unregister(esp);
	kfree(svhn_denoise);
	return 0;
}

static struct esp_driver svhn_denoise_driver = {
	.plat = {
		.probe		= svhn_denoise_probe,
		.remove		= svhn_denoise_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = svhn_denoise_device_ids,
		},
	},
	.xfer_input_ok	= svhn_denoise_xfer_input_ok,
	.prep_xfer	= svhn_denoise_prep_xfer,
	.ioctl_cm	= SVHN_DENOISE_HLS4ML_IOC_ACCESS,
	.arg_size	= sizeof(struct svhn_denoise_hls4ml_access),
};

static int __init svhn_denoise_init(void)
{
	return esp_driver_register(&svhn_denoise_driver);
}

static void __exit svhn_denoise_exit(void)
{
	esp_driver_unregister(&svhn_denoise_driver);
}

module_init(svhn_denoise_init)
module_exit(svhn_denoise_exit)

MODULE_DEVICE_TABLE(of, svhn_denoise_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("svhn_denoise_hls4ml driver");
