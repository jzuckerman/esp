// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "svhn_autoenc_hls4ml.h"

#define DRV_NAME	"svhn_autoenc_hls4ml"

/* <<--regs-->> */
#define SVHN_AUTOENC_NBURSTS_REG 0x40

struct svhn_autoenc_hls4ml_device {
	struct esp_device esp;
};

static struct esp_driver svhn_autoenc_driver;

static struct of_device_id svhn_autoenc_device_ids[] = {
	{
		.name = "SLD_SVHN_AUTOENC_HLS4ML",
	},
	{
		.name = "eb_090",
	},
	{
		.compatible = "sld,svhn_autoenc_hls4ml",
	},
	{ },
};

static int svhn_autoenc_devs;

static inline struct svhn_autoenc_hls4ml_device *to_svhn_autoenc(struct esp_device *esp)
{
	return container_of(esp, struct svhn_autoenc_hls4ml_device, esp);
}

static void svhn_autoenc_prep_xfer(struct esp_device *esp, void *arg)
{
	struct svhn_autoenc_hls4ml_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->nbursts, esp->iomem + SVHN_AUTOENC_NBURSTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool svhn_autoenc_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct svhn_autoenc_hls4ml_device *svhn_autoenc = to_svhn_autoenc(esp); */
	/* struct svhn_autoenc_hls4ml_access *a = arg; */

	return true;
}

static int svhn_autoenc_probe(struct platform_device *pdev)
{
	struct svhn_autoenc_hls4ml_device *svhn_autoenc;
	struct esp_device *esp;
	int rc;

	svhn_autoenc = kzalloc(sizeof(*svhn_autoenc), GFP_KERNEL);
	if (svhn_autoenc == NULL)
		return -ENOMEM;
	esp = &svhn_autoenc->esp;
	esp->module = THIS_MODULE;
	esp->number = svhn_autoenc_devs;
	esp->driver = &svhn_autoenc_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	svhn_autoenc_devs++;
	return 0;
 err:
	kfree(svhn_autoenc);
	return rc;
}

static int __exit svhn_autoenc_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct svhn_autoenc_hls4ml_device *svhn_autoenc = to_svhn_autoenc(esp);

	esp_device_unregister(esp);
	kfree(svhn_autoenc);
	return 0;
}

static struct esp_driver svhn_autoenc_driver = {
	.plat = {
		.probe		= svhn_autoenc_probe,
		.remove		= svhn_autoenc_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = svhn_autoenc_device_ids,
		},
	},
	.xfer_input_ok	= svhn_autoenc_xfer_input_ok,
	.prep_xfer	= svhn_autoenc_prep_xfer,
	.ioctl_cm	= SVHN_AUTOENC_HLS4ML_IOC_ACCESS,
	.arg_size	= sizeof(struct svhn_autoenc_hls4ml_access),
};

static int __init svhn_autoenc_init(void)
{
	return esp_driver_register(&svhn_autoenc_driver);
}

static void __exit svhn_autoenc_exit(void)
{
	esp_driver_unregister(&svhn_autoenc_driver);
}

module_init(svhn_autoenc_init)
module_exit(svhn_autoenc_exit)

MODULE_DEVICE_TABLE(of, svhn_autoenc_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("svhn_autoenc_hls4ml driver");
