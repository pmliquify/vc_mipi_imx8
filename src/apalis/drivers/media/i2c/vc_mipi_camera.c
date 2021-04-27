// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#include "vc_mipi_camera.h"
#include "vc_mipi_subdev.h"
#include "vc_mipi_modules.h"


int vc_param_setup(struct vc_camera *camera)
{
	camera->sen_pars.o_width     = IMX226_DX;
	camera->sen_pars.o_height    = IMX226_DY;
	camera->sen_pars.start_table = (struct sensor_reg *)imx226_start;
	camera->sen_pars.stop_table  = (struct sensor_reg *)imx226_stop;
	camera->sen_pars.mode_table  = (struct sensor_reg *)imx226_mode_3840_3040;

	// camera->model = IMX226_MODEL_COLOR;
	// if(camera->model == IMX226_MODEL_MONOCHROME) {
	// 	camera->vc_data_fmts      = imx226_mono_fmts;
	// 	camera->vc_data_fmts_size = imx226_mono_fmts_size;
	
	// } else {
	// 	camera->vc_data_fmts      = imx226_color_fmts;
	// 	camera->vc_data_fmts_size = imx226_color_fmts_size;
	// }

	// if(camera->model == IMX183_MODEL_MONOCHROME || camera->model == IMX183_MODEL_COLOR) {
	// 	camera->sen_clk = 72000000;     // clock-frequency: default=54Mhz imx183=72Mhz
	// } else {
	// 	camera->sen_clk = 54000000;     // clock-frequency: default=54Mhz imx183=72Mhz
	// }

	camera->state.code = 0;
	camera->state.width = camera->sen_pars.o_width;
	camera->state.height = camera->sen_pars.o_height;
	camera->state.flash_output = 0;
	camera->state.ext_trig = 0;
	camera->state.mode = 0;
	camera->state.streaming = 0;

	return 0;
}

static int vc_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations vc_sd_media_ops = {
	.link_setup = vc_link_setup,
};

static int vc_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *endpoint;
	struct vc_camera *camera;
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

	ret = vc_sd_init(&camera->sd, client);
	if (ret)
		goto free_ctrls;

	camera->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	camera->pad.flags = MEDIA_PAD_FL_SOURCE;
	camera->sd.entity.ops = &vc_sd_media_ops;
	camera->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&camera->sd.entity, 1, &camera->pad);
	if (ret)
		return ret;

	mutex_init(&camera->lock);

	camera->client_sen = client;
	// TODO: Load module address from DT.
	camera->client_mod = vc_mod_setup(camera->client_sen, 0x10, &camera->mod_desc);
	if (camera->client_mod == 0) {
		goto free_ctrls;
	}

	ret = vc_param_setup(camera);
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

static int vc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_camera *camera = to_vc_camera(sd);
	
	v4l2_async_unregister_subdev(&camera->sd);
	media_entity_cleanup(&camera->sd.entity);
	v4l2_ctrl_handler_free(camera->sd.ctrl_handler);
	mutex_destroy(&camera->lock);

	return 0;
}

static const struct i2c_device_id vc_id[] = {
	{"vc-mipi-cam", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, vc_id);

static const struct of_device_id vc_dt_ids[] = {
	{ .compatible = "vc,vc_mipi" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vc_dt_ids);

static struct i2c_driver vc_i2c_driver = {
	.driver = {
		.name  = "vc-mipi-cam",
		.of_match_table	= vc_dt_ids,
	},
	.id_table = vc_id,
	.probe_new = vc_probe,
	.remove   = vc_remove,
};

module_i2c_driver(vc_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");