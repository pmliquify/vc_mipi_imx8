// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#ifndef _VC_MIPI_DEVICE_H
#define _VC_MIPI_DEVICE_H

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h> 
#include <linux/types.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "vc_mipi_module.h"
#include "vc_mipi_ctrls.h"


struct vc_mipi_driver {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	/* the parsed DT endpoint info */
	struct mutex lock;			/* lock to protect all members below */
	
	struct vc_mipi_module module;
	struct vc_mipi_ctrls ctrls;

// 	struct sensor_params sen_pars;
// 	struct v4l2_pix_format pix;

// 	bool streaming;
// 	u32 digital_gain;
// 	u32 exposure_time;
// 	int color;              // color flag: 0=no, 1=yes
// 	int model;              // sensor model
// 	char model_name[32];    // sensor model name, e.g. "IMX327 color"

// 	int sensor_mode;        // sensor mode
// 	int num_lanes;          // # of data lanes: 1, 2, 4
// 	int sensor_ext_trig;    // ext. trigger flag: 0=no, 1=yes
// 	int flash_output;       // flash output enable
// 	int sen_clk;            // sen_clk: default=54Mhz imx183=72Mhz
// 	int fpga_addr;          // FPGA i2c address (def = 0x10)
};

static inline struct vc_mipi_driver *to_vc_mipi_driver(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_mipi_driver, sd);
}

#endif // _VC_MIPI_DEVICE_H