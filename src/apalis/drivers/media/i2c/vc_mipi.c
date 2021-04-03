// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#include "vc_mipi.h"
#include "vc_mipi_sd.h"
#include "vc_mipi_mod.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

const struct v4l2_subdev_core_ops vc_mipi_core_ops = {
	.s_power = vc_mipi_s_power,
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

const struct v4l2_subdev_video_ops vc_mipi_video_ops = {
	.g_frame_interval = vc_mipi_g_frame_interval,
	.s_frame_interval = vc_mipi_s_frame_interval,
	.s_stream = vc_mipi_s_stream,
};

const struct v4l2_subdev_pad_ops vc_mipi_pad_ops = {
	.enum_mbus_code = vc_mipi_enum_mbus_code,
	.get_fmt = vc_mipi_get_fmt,
	.set_fmt = vc_mipi_set_fmt,
	.enum_frame_size = vc_mipi_enum_frame_size,
	.enum_frame_interval = vc_mipi_enum_frame_interval,
};

const struct v4l2_subdev_ops vc_mipi_subdev_ops = {
	.core = &vc_mipi_core_ops,
	.video = &vc_mipi_video_ops,
	.pad = &vc_mipi_pad_ops,
};

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
	struct vc_mipi_dev *sensor;
	// struct v4l2_mbus_framefmt *fmt;
	// u32 rotation;
	int ret;

	TRACE

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->i2c_client = client;

	// /*
	//  * default init sequence initialize sensor to
	//  * YUV422 UYVY VGA@30fps
	//  */
	// fmt = &sensor->fmt;
	// fmt->code = MEDIA_BUS_FMT_UYVY8_2X8;
	// fmt->colorspace = V4L2_COLORSPACE_SRGB;
	// fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	// fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	// fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
	// fmt->width = 640;
	// fmt->height = 480;
	// fmt->field = V4L2_FIELD_NONE;
	// sensor->frame_interval.numerator = 1;
	// sensor->frame_interval.denominator = vc_mipi_framerates[vc_mipi_30_FPS];
	// sensor->current_fr = vc_mipi_30_FPS;
	// sensor->current_mode =
	// 	&vc_mipi_mode_data[IMX226_MODE_VGA_640_480];
	// sensor->last_mode = sensor->current_mode;

	// sensor->ae_target = 52;

	// /* optional indication of physical rotation of sensor */
	// ret = fwnode_property_read_u32(dev_fwnode(&client->dev), "rotation",
	// 			       &rotation);
	// if (!ret) {
	// 	switch (rotation) {
	// 	case 180:
	// 		sensor->upside_down = true;
	// 		/* fall through */
	// 	case 0:
	// 		break;
	// 	default:
	// 		dev_warn(dev, "[vc-mipi vc_mipi] %u degrees rotation is not supported, ignoring...\n",
	// 			 rotation);
	// 	}
	// }

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(&client->dev),
						  NULL);
	if (!endpoint) {
		dev_err(dev, "[vc-mipi vc_mipi] endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &sensor->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "[vc-mipi vc_mipi] Could not parse endpoint\n");
		return ret;
	}

	// /* get system clock (xclk) */
	// sensor->xclk = devm_clk_get(dev, "xclk");
	// if (IS_ERR(sensor->xclk)) {
	// 	dev_err(dev, "[vc-mipi vc_mipi] failed to get xclk\n");
	// 	return PTR_ERR(sensor->xclk);
	// }

	// sensor->xclk_freq = clk_get_rate(sensor->xclk);
	// if (sensor->xclk_freq < IMX226_XCLK_MIN ||
	//     sensor->xclk_freq > IMX226_XCLK_MAX) {
	// 	dev_err(dev, "[vc-mipi vc_mipi] xclk frequency out of range: %d Hz\n",
	// 		sensor->xclk_freq);
	// 	return -EINVAL;
	// }

	// /* request optional power down pin */
	// sensor->pwdn_gpio = devm_gpiod_get_optional(dev, "powerdown",
	// 					    GPIOD_OUT_HIGH);
	// if (IS_ERR(sensor->pwdn_gpio))
	// 	return PTR_ERR(sensor->pwdn_gpio);

	// /* request optional reset pin */
	// sensor->reset_gpio = devm_gpiod_get_optional(dev, "reset",
	// 					     GPIOD_OUT_HIGH);
	// if (IS_ERR(sensor->reset_gpio))
	// 	return PTR_ERR(sensor->reset_gpio);

	v4l2_i2c_subdev_init(&sensor->sd, client, &vc_mipi_subdev_ops);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.ops = &vc_mipi_sd_media_ops;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		return ret;

	// ret = vc_mipi_get_regulators(sensor);
	// if (ret)
	// 	return ret;

	mutex_init(&sensor->lock);

	//---------------------------------------------------------------------

	ret = vc_mipi_mod_setup(sensor);
	if(ret) {
		goto entity_cleanup;
	}

	// ret = imx_param_setup(priv);
    	// if(ret)
    	// {
        // 	dev_err(dev, "[vc-mipi driver] %s: imx_param_setup() ret=%d\n", __func__, ret);
        // 	return -EIO;
    	// }

	//---------------------------------------------------------------------

	// ret = vc_mipi_check_chip_id(sensor);
	// if (ret)
	// 	goto entity_cleanup;

	ret = vc_mipi_init_controls(sensor);
	if (ret)
		goto entity_cleanup;

	ret = v4l2_async_register_subdev_sensor_common(&sensor->sd);
	// if (ret)
	// 	goto free_ctrls;

	return 0;

// free_ctrls:
// 	v4l2_ctrl_handler_free(&sensor->ctrls.handler);
entity_cleanup:
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	return ret;
}

static int vc_mipi_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);

	TRACE
	
	v4l2_async_unregister_subdev(&sensor->sd);
	media_entity_cleanup(&sensor->sd.entity);
	// v4l2_ctrl_handler_free(&sensor->ctrls.handler);
	mutex_destroy(&sensor->lock);

	return 0;
}

static const struct i2c_device_id vc_mipi_id[] = {
	{"vc_mipi", 0},
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
		.name  = "vc-mipi",
		.of_match_table	= vc_mipi_dt_ids,
	},
	.id_table = vc_mipi_id,
	.probe_new = vc_mipi_probe,
	.remove   = vc_mipi_remove,
};

module_i2c_driver(vc_mipi_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");
