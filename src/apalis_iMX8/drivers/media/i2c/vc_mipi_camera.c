#include "vc_mipi_core.h"

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
#include <linux/delay.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

struct vc_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	// the parsed DT endpoint info

	struct vc_cam cam;
};

static inline struct vc_device *to_vc_device(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_device, sd);
}

static inline struct vc_cam *to_vc_cam(struct v4l2_subdev *sd)
{
	struct vc_device *device = to_vc_device(sd);
	return &device->cam;
}


// --- v4l2_subdev_core_ops ---------------------------------------------------

static int vc_sd_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int vc_sd_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL)
		return -EINVAL;

	vc_dbg(dev, "%s(): Get control '%s' value %d\n", __FUNCTION__, ctrl->name, ctrl->val);
	return v4l2_g_ctrl(sd->ctrl_handler, control);
}

static __s32 vc_sd_get_ctrl_value(struct v4l2_subdev *sd, __u32 id)
{
	struct v4l2_control control;
	control.id = id;
	vc_sd_g_ctrl(sd, &control);
	// TODO: Errorhandling
	return control.value;
}

static int vc_sd_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL) {
		vc_err(dev, "%s(): Control with id: 0x%08x not defined!\n", __FUNCTION__, control->id);
		return -EINVAL;
	}

	if (control->value < ctrl->minimum || control->value > ctrl->maximum) {
		vc_err(dev, "%s(): Control '%s' with value: %d exceeds allowed range (%lld - %lld)\n", __FUNCTION__,
			ctrl->name, control->value, ctrl->minimum, ctrl->maximum);
		return -EINVAL;
	}

	vc_dbg(dev, "%s(): Set control '%s' to value %d\n", __FUNCTION__, ctrl->name, control->value);

	// Store value in ctrl to get and restore ctrls later and after power off.
	v4l2_ctrl_s_ctrl(ctrl, control->value);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return vc_sen_set_exposure(cam, control->value);

	case V4L2_CID_GAIN:
		return vc_sen_set_gain(cam, control->value);
	}

	vc_warn(dev, "%s(): Control with id: 0x%08x is not handled\n", __FUNCTION__, ctrl->id);
	return -EINVAL;
}


// --- v4l2_subdev_video_ops ---------------------------------------------------

static int vc_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vc_cam *cam = to_vc_cam(sd);
	// struct vc_ctrl *ctrl = &cam->ctrl;
	struct vc_state *state = &cam->state;
	struct device *dev = sd->dev;
	struct vc_frame *frame = vc_core_get_frame(cam);
	int reset = 0;
	int ret = 0;

	vc_notice(dev, "%s(): Set streaming: %s\n", __FUNCTION__, enable ? "on" : "off");

	if (enable) {
		if (state->streaming == 1) {
			vc_warn(dev, "%s(): Sensor is already streaming!\n", __FUNCTION__);
			ret = vc_sen_stop_stream(cam);
		}

		ret  = vc_mod_set_mode(cam, &reset);
		if (!ret && reset) {
			ret |= vc_sen_set_roi(cam, frame->x, frame->y, frame->width, frame->height);
			ret |= vc_sen_set_exposure(cam, vc_sd_get_ctrl_value(sd, V4L2_CID_EXPOSURE));
			ret |= vc_sen_set_gain(cam, vc_sd_get_ctrl_value(sd, V4L2_CID_GAIN));
			ret |= vc_sen_set_blacklevel(cam, cam->state.blacklevel);
		}
		ret |= vc_sen_start_stream(cam);
		if (ret == 0)
			state->streaming = 1;

	} else {
		ret = vc_sen_stop_stream(cam);
		if (ret == 0)
			state->streaming = 0;
	}

	return ret;
}

// --- v4l2_subdev_pad_ops ---------------------------------------------------

static int vc_sd_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct vc_frame* frame = vc_core_get_frame(cam);

	mf->code = vc_core_get_format(cam);
	mf->width = frame->width;
	mf->height = frame->height;
	// mf->reserved[1] = 30;

	return 0;
}

static int vc_sd_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct v4l2_mbus_framefmt *mf = &format->format;

	vc_core_set_format(cam, mf->code);
	vc_core_set_frame(cam, 0, 0, mf->width, mf->height);
	
	return 0;
}



// *** Initialisation *********************************************************

static const struct v4l2_subdev_core_ops vc_core_ops = {
	.s_power = vc_sd_s_power,
	.g_ctrl = vc_sd_g_ctrl,
	.s_ctrl = vc_sd_s_ctrl,
};

static const struct v4l2_subdev_video_ops vc_video_ops = {
	.s_stream = vc_sd_s_stream,
};

static const struct v4l2_subdev_pad_ops vc_pad_ops = {
	.get_fmt = vc_sd_get_fmt,
	.set_fmt = vc_sd_set_fmt,
};

static const struct v4l2_subdev_ops vc_subdev_ops = {
	.core = &vc_core_ops,
	.video = &vc_video_ops,
	.pad = &vc_pad_ops,
};

static struct v4l2_ctrl_config ctrl_config_gain = {
	.id = V4L2_CID_GAIN,
	.name = "Gain",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.step = 1,
};

static struct v4l2_ctrl_config ctrl_config_exposure = {
	.id = V4L2_CID_EXPOSURE,
	.name = "Exposure", 
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.step = 1,
};

static int vc_sd_init_ctrl(struct vc_device *device, struct v4l2_ctrl_handler *hdl, struct v4l2_ctrl_config* cfg, struct vc_control* control) 
{
	struct v4l2_subdev *sd = &device->sd;
	struct i2c_client *client = device->cam.ctrl.client_sen;
	struct device *dev = &client->dev;
	struct v4l2_ctrl *ctrl;

	cfg->min = control->min;
	cfg->max = control->max;
	cfg->def = control->def;
	ctrl = v4l2_ctrl_new_custom(hdl, cfg, NULL);
	if (ctrl == NULL) {
		vc_err(dev, "%s(): Failed to init %s ctrl\n", __FUNCTION__, cfg->name);
		return -EIO;
	}
	ctrl->priv = (void*)sd;
	
	return 0;
}

static int read_property_u32(struct device_node *node, const char *name, int radix, __u32 *value)
{
	const char *str;
	int ret = 0;

    	ret = of_property_read_string(node, name, &str);
    	if (ret)
        	return -ENODATA;

	ret = kstrtou32(str, radix, value);
	if (ret)
		return -EFAULT;

    	return 0;
}

static int vc_sd_parse_dt(struct vc_device *device)
{
	struct vc_cam *cam = &device->cam;
	struct device *dev = vc_core_get_sen_device(cam);
	struct device_node *node = dev->of_node;
	int value = 0;
	int ret = 0;

	if (node != NULL) {
		ret = read_property_u32(node, "num_lanes", 10, &value);
		if (ret) {
			vc_err(dev, "%s(): Unable to read num_lanes from device tree!\n", __FUNCTION__);
		} else {
			vc_core_set_num_lanes(cam, value);
		}

		ret = read_property_u32(node, "trigger_mode", 10, &value);
		if (ret) {
			vc_err(dev, "%s(): Unable to read trigger_mode from device tree!\n", __FUNCTION__);
		} else {
			vc_mod_set_trigger_mode(cam, value);
		}

		ret = read_property_u32(node, "flash_mode", 10, &value);
		if (ret) {
			vc_err(dev, "%s(): Unable to read flash_mode from device tree!\n", __FUNCTION__);
		} else {
			vc_mod_set_io_mode(cam, value);
		}

		ret = read_property_u32(node, "frame_rate", 10, &value);
		if (ret) {
			vc_err(dev, "%s(): Unable to read frame_rate from device tree!\n", __FUNCTION__);
		} else {
			vc_core_set_framerate(cam, value);
		}
	}

	return 0;
}

static int vc_sd_init(struct vc_device *device)
{
	struct v4l2_subdev *sd = &device->sd;
	struct i2c_client *client = device->cam.ctrl.client_sen;
	struct device *dev = &client->dev;
	struct v4l2_ctrl_handler *hdl;
	// struct v4l2_ctrl *ctrl;
	int ret;

	// Initializes the subdevice
	v4l2_i2c_subdev_init(sd, client, &vc_subdev_ops);

	// Initializes the ctrls
	hdl = devm_kzalloc(dev, sizeof(*hdl), GFP_KERNEL);
	ret = v4l2_ctrl_handler_init(hdl, 2);
	if (ret) {
		vc_err(dev, "%s(): Failed to init control handler\n", __FUNCTION__);
		return ret;
	}

	ret = vc_sd_init_ctrl(device, hdl, &ctrl_config_exposure, &device->cam.ctrl.exposure);
	if (ret) 
		return ret;

	ret = vc_sd_init_ctrl(device, hdl, &ctrl_config_gain, &device->cam.ctrl.gain);
	if (ret) 
		return ret;
	
	if (hdl->error) {
		ret = hdl->error;
		vc_err(dev, "%s(): Control init failed (%d)\n", __FUNCTION__, ret);
		goto error;
	}

	sd->ctrl_handler = hdl;

	return 0;

error:
	v4l2_ctrl_handler_free(hdl);
	return ret;
}

static int vc_link_setup(struct media_entity *entity, const struct media_pad *local, const struct media_pad *remote,
			 __u32 flags)
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
	struct vc_device *device;
	struct vc_cam *cam;
	int ret;

	device = devm_kzalloc(dev, sizeof(*device), GFP_KERNEL);
	if (!device)
		return -ENOMEM;
	cam = &device->cam;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		vc_err(dev, "%s(): Endpoint node not found\n", __FUNCTION__);
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &device->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		vc_err(dev, "%s(): Could not parse endpoint\n", __FUNCTION__);
		return ret;
	}

	ret  = vc_core_init(cam, client);
	if (ret)
		goto free_ctrls;

	ret = vc_sd_parse_dt(device);
	if (ret)
		goto free_ctrls;

	ret = vc_sd_init(device);
	if (ret)
		goto free_ctrls;

	device->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	device->pad.flags = MEDIA_PAD_FL_SOURCE;
	device->sd.entity.ops = &vc_sd_media_ops;
	device->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&device->sd.entity, 1, &device->pad);
	if (ret)
		return ret;

	ret = v4l2_async_register_subdev_sensor_common(&device->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(device->sd.ctrl_handler);
	media_entity_cleanup(&device->sd.entity);
	return ret;
}

static int vc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_device *device = to_vc_device(sd);

	v4l2_async_unregister_subdev(&device->sd);
	media_entity_cleanup(&device->sd.entity);
	v4l2_ctrl_handler_free(device->sd.ctrl_handler);

	return 0;
}

static const struct i2c_device_id vc_id[] = {
	{ "vc-mipi-cam", 0 },
	{ /* sentinel */ },
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

MODULE_VERSION("0.4.0");
MODULE_DESCRIPTION("Vision Components GmbH - VC MIPI NVIDIA driver");
MODULE_AUTHOR("Peter Martienssen, Liquify Consulting <peter.martienssen@liquify-consulting.de>");
MODULE_LICENSE("GPL v2");
