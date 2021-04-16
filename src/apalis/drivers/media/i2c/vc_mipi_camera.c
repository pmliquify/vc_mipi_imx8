// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#include "vc_mipi_camera.h"
#include "vc_mipi_subdev.h"


/*............. Sensor models */
enum sen_model {
    SEN_MODEL_UNKNOWN = 0,
    OV7251_MODEL_MONOCHROME,
    OV7251_MODEL_COLOR,
//    OV9281_MODEL,
    OV9281_MODEL_MONOCHROME,
    OV9281_MODEL_COLOR,

    IMX183_MODEL_MONOCHROME,
    IMX183_MODEL_COLOR,
    IMX226_MODEL_MONOCHROME,
    IMX226_MODEL_COLOR,
    IMX250_MODEL_MONOCHROME,
    IMX250_MODEL_COLOR,
    IMX252_MODEL_MONOCHROME,
    IMX252_MODEL_COLOR,
    IMX273_MODEL_MONOCHROME,
    IMX273_MODEL_COLOR,

    IMX290_MODEL_MONOCHROME,
    IMX290_MODEL_COLOR,
    IMX296_MODEL_MONOCHROME,
    IMX296_MODEL_COLOR,
    IMX297_MODEL_MONOCHROME,
    IMX297_MODEL_COLOR,

    IMX327C_MODEL,
    IMX327_MODEL_MONOCHROME,
    IMX327_MODEL_COLOR,
    IMX412_MODEL_MONOCHROME,
    IMX412_MODEL_COLOR,
    IMX415_MODEL_MONOCHROME,
    IMX415_MODEL_COLOR,
};

/*----------------------------------------------------------------------*/
/*   IMX226                                                             */
/*----------------------------------------------------------------------*/
#define SENSOR_TABLE_END        0xffff

/* In dB*10 */
#define IMX226_DIGITAL_GAIN_MIN        0x00
#define IMX226_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX226_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX226_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX226_DIGITAL_EXPOSURE_MAX     10000000
#define IMX226_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
  #define IMX226_DX 3840
  #define IMX226_DY 3040
#else    // test mode
  #define IMX226_DX 1440    // IMX273
  #define IMX226_DY 1080
//#define IMX226_DX 1920    // IMX327
//#define IMX226_DY 1080
//#define IMX226_DX 1280    // OV9281
//#define IMX226_DY 800
#endif

static const struct sensor_reg imx226_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx226_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
/* MCLK:25MHz  3840 x 3040   MIPI LANE2 */
static const struct sensor_reg imx226_mode_3840_3040[] = {
#if 1
    { 0x6015, IMX226_DX & 0xFF }, { 0x6016, (IMX226_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX226_DY & 0xFF }, { 0x6011, (IMX226_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX226 - mono formats
// static struct vc_mipi_datafmt imx226_mono_fmts[] = {
//     { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
//     { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
//     { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
//     { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// // use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
// };
// static int imx226_mono_fmts_size = ARRAY_SIZE(imx226_mono_fmts);

// IMX226 - color formats
// static struct vc_mipi_datafmt imx226_color_fmts[] = {
//     { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
//     { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
//     { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
// };
// static int imx226_color_fmts_size = ARRAY_SIZE(imx226_color_fmts);
/*----------------------------------------------------------------------*/


int vc_mipi_param_setup(struct vc_mipi_camera *camera)
{
	// camera->sen_pars.gain_min         = IMX226_DIGITAL_GAIN_MIN;
	// camera->sen_pars.gain_max         = IMX226_DIGITAL_GAIN_MAX;
	// camera->sen_pars.gain_default     = IMX226_DIGITAL_GAIN_DEFAULT;
	// camera->sen_pars.exposure_min     = IMX226_DIGITAL_EXPOSURE_MIN;
	// camera->sen_pars.exposure_max     = IMX226_DIGITAL_EXPOSURE_MAX;
	// camera->sen_pars.exposure_default = IMX226_DIGITAL_EXPOSURE_DEFAULT;
	// camera->sen_pars.frame_dx         = IMX226_DX;
	// camera->sen_pars.frame_dy         = IMX226_DY;

	camera->sen_pars.sensor_start_table = (struct sensor_reg *)imx226_start;
	camera->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx226_stop;
	camera->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx226_mode_3840_3040;

	// camera->model = IMX226_MODEL_COLOR;
	// if(camera->model == IMX226_MODEL_MONOCHROME) {
	// 	camera->vc_mipi_data_fmts      = imx226_mono_fmts;
	// 	camera->vc_mipi_data_fmts_size = imx226_mono_fmts_size;
	
	// } else {
	// 	camera->vc_mipi_data_fmts      = imx226_color_fmts;
	// 	camera->vc_mipi_data_fmts_size = imx226_color_fmts_size;
	// }

	// camera->pix.width  = camera->sen_pars.frame_dx; // 640;
	// camera->pix.height = camera->sen_pars.frame_dy; // 480;

	// vc_mipi_valid_res[0].width  = camera->sen_pars.frame_dx;
	// vc_mipi_valid_res[0].height = camera->sen_pars.frame_dy;

	// camera->sensor_ext_trig = 0;    // ext. trigger flag: 0=no, 1=yes

	// if(camera->model == IMX183_MODEL_MONOCHROME || camera->model == IMX183_MODEL_COLOR) {
	// 	camera->sen_clk = 72000000;     // clock-frequency: default=54Mhz imx183=72Mhz
	// } else {
	// 	camera->sen_clk = 54000000;     // clock-frequency: default=54Mhz imx183=72Mhz
	// }

	camera->flash_output = 0;
	camera->mode = 0;
	camera->streaming = 0;

	return 0;
}

static int vc_mipi_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations vc_mipi_sd_media_ops = {
	.link_setup = vc_mipi_link_setup,
};

static int vc_mipi_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *endpoint;
	struct vc_mipi_camera *camera;
	int ret;

	camera = devm_kzalloc(dev, sizeof(*camera), GFP_KERNEL);
	if (!camera)
		return -ENOMEM;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		dev_err(dev, "[vc-mipi vc_mipi] endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &camera->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "[vc-mipi vc_mipi] Could not parse endpoint\n");
		return ret;
	}

	ret = vc_mipi_sd_init(&camera->sd, client);
	if (ret)
		goto free_ctrls;

	camera->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	camera->pad.flags = MEDIA_PAD_FL_SOURCE;
	camera->sd.entity.ops = &vc_mipi_sd_media_ops;
	camera->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&camera->sd.entity, 1, &camera->pad);
	if (ret)
		return ret;

	mutex_init(&camera->lock);

	camera->client_sen = client;
	// TODO: Load module address from DT.
	camera->client_mod = vc_mipi_mod_setup(camera->client_sen, 0x10, &camera->mod_desc);
	if (camera->client_mod == 0) {
		goto free_ctrls;
	}

	ret = vc_mipi_param_setup(camera);
    	if(ret) {
        	goto free_ctrls;
    	}

	ret = v4l2_async_register_subdev_sensor_common(&camera->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(camera->sd.ctrl_handler);
// entity_cleanup:
	media_entity_cleanup(&camera->sd.entity);
	mutex_destroy(&camera->lock);
	return ret;
}

static int vc_mipi_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_mipi_camera *camera = to_vc_mipi_camera(sd);
	
	v4l2_async_unregister_subdev(&camera->sd);
	media_entity_cleanup(&camera->sd.entity);
	v4l2_ctrl_handler_free(camera->sd.ctrl_handler);
	mutex_destroy(&camera->lock);

	return 0;
}

static const struct i2c_device_id vc_mipi_id[] = {
	{"vc-mipi-cam", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, vc_mipi_id);

static const struct of_device_id vc_mipi_dt_ids[] = {
	{ .compatible = "vc,vc_mipi" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vc_mipi_dt_ids);

static struct i2c_driver vc_mipi_i2c_driver = {
	.driver = {
		.name  = "vc-mipi-cam",
		.of_match_table	= vc_mipi_dt_ids,
	},
	.id_table = vc_mipi_id,
	.probe_new = vc_mipi_probe,
	.remove   = vc_mipi_remove,
};

module_i2c_driver(vc_mipi_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");