// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "svhn_mlp_hls4ml.h"

#define DRV_NAME	"svhn_mlp_hls4ml"

/* <<--regs-->> */
#define SVHN_MLP_NBURSTS_REG 0x40

struct svhn_mlp_hls4ml_device {
	struct esp_device esp;
};

static struct esp_driver svhn_mlp_driver;

static struct of_device_id svhn_mlp_device_ids[] = {
	{
		.name = "SLD_SVHN_MLP_HLS4ML",
	},
	{
		.name = "eb_089",
	},
	{
		.compatible = "sld,svhn_mlp_hls4ml",
	},
	{ },
};

static int svhn_mlp_devs;

static inline struct svhn_mlp_hls4ml_device *to_svhn_mlp(struct esp_device *esp)
{
	return container_of(esp, struct svhn_mlp_hls4ml_device, esp);
}

static void svhn_mlp_prep_xfer(struct esp_device *esp, void *arg)
{
	struct svhn_mlp_hls4ml_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->nbursts, esp->iomem + SVHN_MLP_NBURSTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool svhn_mlp_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct svhn_mlp_hls4ml_device *svhn_mlp = to_svhn_mlp(esp); */
	/* struct svhn_mlp_hls4ml_access *a = arg; */

	return true;
}

static int svhn_mlp_probe(struct platform_device *pdev)
{
	struct svhn_mlp_hls4ml_device *svhn_mlp;
	struct esp_device *esp;
	int rc;

	svhn_mlp = kzalloc(sizeof(*svhn_mlp), GFP_KERNEL);
	if (svhn_mlp == NULL)
		return -ENOMEM;
	esp = &svhn_mlp->esp;
	esp->module = THIS_MODULE;
	esp->number = svhn_mlp_devs;
	esp->driver = &svhn_mlp_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	svhn_mlp_devs++;
	return 0;
 err:
	kfree(svhn_mlp);
	return rc;
}

static int __exit svhn_mlp_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct svhn_mlp_hls4ml_device *svhn_mlp = to_svhn_mlp(esp);

	esp_device_unregister(esp);
	kfree(svhn_mlp);
	return 0;
}

static struct esp_driver svhn_mlp_driver = {
	.plat = {
		.probe		= svhn_mlp_probe,
		.remove		= svhn_mlp_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = svhn_mlp_device_ids,
		},
	},
	.xfer_input_ok	= svhn_mlp_xfer_input_ok,
	.prep_xfer	= svhn_mlp_prep_xfer,
	.ioctl_cm	= SVHN_MLP_HLS4ML_IOC_ACCESS,
	.arg_size	= sizeof(struct svhn_mlp_hls4ml_access),
};

static int __init svhn_mlp_init(void)
{
	return esp_driver_register(&svhn_mlp_driver);
}

static void __exit svhn_mlp_exit(void)
{
	esp_driver_unregister(&svhn_mlp_driver);
}

module_init(svhn_mlp_init)
module_exit(svhn_mlp_exit)

MODULE_DEVICE_TABLE(of, svhn_mlp_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("svhn_mlp_hls4ml driver");
