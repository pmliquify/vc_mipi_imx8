#ifndef _VC_MIPI_CAMERA_H
#define _VC_MIPI_CAMERA_H

// #define DEBUG

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
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

#include "vc_mipi_core.h"


// // Valid sensor resolutions
// struct vc_res {
//     int width;
//     int height;
// };

// /* Valid sensor resolutions */
// static struct vc_res vc_valid_res[] = {
//     [0] = {640, 480},
// //    [1] = {320, 240},
// //    [2] = {720, 480},
// //    [3] = {1280, 720},
// //    [4] = {1920, 1080},
// //    [5] = {2592, 1944},
// };

// struct vc_datafmt {
//     __u32 code;
//     enum v4l2_colorspace colorspace;
// };

struct vc_sen_pars {
	__u32 o_width;
	__u32 o_height;
	struct sensor_reg *start_table;
	struct sensor_reg *stop_table;
	struct sensor_reg *mode_table;
};

struct vc_state {
	// Frame and Format
	__u32 code;
	__u32 width;
	__u32 height;
	// Configuration flags
	int flash_output;       		// flash output enable
	int ext_trig;    			// ext. trigger flag: 0=no, 1=yes
	// Status flags
	int mode;	        		// sensor mode
 	int streaming;
};

struct vc_camera {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	/* the parsed DT endpoint info */
	struct mutex lock;			/* lock to protect all members below */

	struct i2c_client* client_sen;
	struct i2c_client* client_mod;

	// Sensor specific configuration
	struct vc_mod_desc mod_desc;
	struct vc_sen_pars sen_pars;
	struct vc_state state;

	// struct vc_datafmt *vc_data_fmts;
	// int vc_data_fmts_size;
	// int model;              		// sensor model
	// int sen_clk;            		// sen_clk: default=54Mhz imx183=72Mhz	
	// const struct vc_datafmt *fmt;
	// struct v4l2_captureparm streamcap;
 	// struct v4l2_pix_format pix;
	// int num_lanes;          		// # of data lanes: 1, 2, 4

// 	int color;              // color flag: 0=no, 1=yes
// 	char model_name[32];    // sensor model name, e.g. "IMX327 color"
// 	int fpga_addr;          // FPGA i2c address (def = 0x10)
};

static inline struct vc_camera *to_vc_camera(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_camera, sd);
}

#endif // _VC_MIPI_CAMERA_H