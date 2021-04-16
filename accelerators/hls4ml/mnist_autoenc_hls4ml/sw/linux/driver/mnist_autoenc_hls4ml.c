// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "mnist_autoenc_hls4ml.h"

#define DRV_NAME	"mnist_autoenc_hls4ml"

/* <<--regs-->> */
#define MNIST_AUTOENC_NBURSTS_REG 0x40

struct mnist_autoenc_hls4ml_device {
	struct esp_device esp;
};

static struct esp_driver mnist_autoenc_driver;

static struct of_device_id mnist_autoenc_device_ids[] = {
	{
		.name = "SLD_MNIST_AUTOENC_HLS4ML",
	},
	{
		.name = "eb_092",
	},
	{
		.compatible = "sld,mnist_autoenc_hls4ml",
	},
	{ },
};

static int mnist_autoenc_devs;

static inline struct mnist_autoenc_hls4ml_device *to_mnist_autoenc(struct esp_device *esp)
{
	return container_of(esp, struct mnist_autoenc_hls4ml_device, esp);
}

static void mnist_autoenc_prep_xfer(struct esp_device *esp, void *arg)
{
	struct mnist_autoenc_hls4ml_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->nbursts, esp->iomem + MNIST_AUTOENC_NBURSTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool mnist_autoenc_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct mnist_autoenc_hls4ml_device *mnist_autoenc = to_mnist_autoenc(esp); */
	/* struct mnist_autoenc_hls4ml_access *a = arg; */

	return true;
}

static int mnist_autoenc_probe(struct platform_device *pdev)
{
	struct mnist_autoenc_hls4ml_device *mnist_autoenc;
	struct esp_device *esp;
	int rc;

	mnist_autoenc = kzalloc(sizeof(*mnist_autoenc), GFP_KERNEL);
	if (mnist_autoenc == NULL)
		return -ENOMEM;
	esp = &mnist_autoenc->esp;
	esp->module = THIS_MODULE;
	esp->number = mnist_autoenc_devs;
	esp->driver = &mnist_autoenc_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	mnist_autoenc_devs++;
	return 0;
 err:
	kfree(mnist_autoenc);
	return rc;
}

static int __exit mnist_autoenc_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct mnist_autoenc_hls4ml_device *mnist_autoenc = to_mnist_autoenc(esp);

	esp_device_unregister(esp);
	kfree(mnist_autoenc);
	return 0;
}

static struct esp_driver mnist_autoenc_driver = {
	.plat = {
		.probe		= mnist_autoenc_probe,
		.remove		= mnist_autoenc_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = mnist_autoenc_device_ids,
		},
	},
	.xfer_input_ok	= mnist_autoenc_xfer_input_ok,
	.prep_xfer	= mnist_autoenc_prep_xfer,
	.ioctl_cm	= MNIST_AUTOENC_HLS4ML_IOC_ACCESS,
	.arg_size	= sizeof(struct mnist_autoenc_hls4ml_access),
};

static int __init mnist_autoenc_init(void)
{
	return esp_driver_register(&mnist_autoenc_driver);
}

static void __exit mnist_autoenc_exit(void)
{
	esp_driver_unregister(&mnist_autoenc_driver);
}

module_init(mnist_autoenc_init)
module_exit(mnist_autoenc_exit)

MODULE_DEVICE_TABLE(of, mnist_autoenc_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mnist_autoenc_hls4ml driver");
