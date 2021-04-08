// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#include "vc_mipi_driver.h"
#include "vc_mipi_subdev.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_driver.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

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
	struct vc_mipi_driver *driver;
	int ret;

	// TRACE

	driver = devm_kzalloc(dev, sizeof(*driver), GFP_KERNEL);
	if (!driver)
		return -ENOMEM;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(&client->dev),
						  NULL);
	if (!endpoint) {
		dev_err(dev, "[vc-mipi vc_mipi] endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &driver->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "[vc-mipi vc_mipi] Could not parse endpoint\n");
		return ret;
	}

	ret = vc_mipi_subdev_init(&driver->sd, client);
	if (ret)
		goto entity_cleanup;

	driver->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	driver->pad.flags = MEDIA_PAD_FL_SOURCE;
	driver->sd.entity.ops = &vc_mipi_sd_media_ops;
	driver->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&driver->sd.entity, 1, &driver->pad);
	if (ret)
		return ret;

	mutex_init(&driver->lock);

	driver->module.client_sen = client;
	ret = vc_mipi_mod_setup(&driver->module);
	if (ret) {
		goto entity_cleanup;
	}

	// ret = imx_param_setup(priv);
    	// if(ret)
    	// {
        // 	dev_err(dev, "[vc-mipi driver] %s: imx_param_setup() ret=%d\n", __func__, ret);
        // 	return -EIO;
    	// }

	ret = v4l2_async_register_subdev_sensor_common(&driver->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(driver->sd.ctrl_handler);
entity_cleanup:
	media_entity_cleanup(&driver->sd.entity);
	mutex_destroy(&driver->lock);
	return ret;
}

static int vc_mipi_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_mipi_driver *driver = to_vc_mipi_driver(sd);

	TRACE
	
	v4l2_async_unregister_subdev(&driver->sd);
	media_entity_cleanup(&driver->sd.entity);
	v4l2_ctrl_handler_free(driver->sd.ctrl_handler);
	mutex_destroy(&driver->lock);

	return 0;
}

static const struct i2c_device_id vc_mipi_id[] = {
	{"vc_mipi-sen", 0},
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
		.name  = "vc-mipi-sen",
		.of_match_table	= vc_mipi_dt_ids,
	},
	.id_table = vc_mipi_id,
	.probe_new = vc_mipi_probe,
	.remove   = vc_mipi_remove,
};

module_i2c_driver(vc_mipi_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");