#include "vc_mipi_subdev.h"
#include "vc_mipi_driver.h"
#include "vc_mipi_sensor.h"
#include <linux/delay.h>

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_subdev.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE


// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_mipi_s_power(struct v4l2_subdev *sd, int on)
{
	struct vc_mipi_driver *driver = to_vc_mipi_driver(sd);
	struct i2c_client *client_mod = driver->module.client_mod;
	struct device *dev_mod = &client_mod->dev;
	int ret;

	// dev_info(dev_mod, "[vc-mipi subdev] %s: On=%d\n", __FUNCTION__, on); 

	if (on) {
		/* restore controls */
		ret = v4l2_ctrl_handler_setup(sd->ctrl_handler);
		if(ret) {
			dev_err(dev_mod, "[vc-mipi subdev] %s: Failed to restore control handler\n",  __func__);
		}
	}
	
	return ret;
}

int vc_mipi_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	return v4l2_queryctrl(sd->ctrl_handler, qc);
}

int vc_mipi_try_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	return 0;
}

int vc_mipi_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct vc_mipi_driver *driver = to_vc_mipi_driver(sd);
	struct i2c_client* client_sen = driver->module.client_sen;
	struct device* dev = sd->dev;
	struct v4l2_ctrl *ctrl;

	// TRACE

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL)
		return -EINVAL;

	if(control->value < ctrl->minimum || control->value > ctrl->maximum) {
		dev_err(dev, "[vc-mipi subdev] %s: Control value '%s' exceeds allowed range (%lld - %lld)\n", 
			__FUNCTION__, ctrl->name, ctrl->minimum, ctrl->maximum);
		return -EINVAL;
	}

	// Store value in ctrl to get and restore ctrls later and after power off.
	v4l2_ctrl_s_ctrl(ctrl, control->value);

     	switch (ctrl->id)  {
 	case V4L2_CID_EXPOSURE: 	
        	return vc_mipi_sen_set_exposure(client_sen, control->value);

	case V4L2_CID_GAIN:
		return vc_mipi_sen_set_gain(client_sen, control->value);
    	}

       	dev_err(dev, "[vc-mipi subdev] %s: ctrl(id:0x%x,val:0x%x) is not handled\n", __func__, ctrl->id, ctrl->val);
      	return -EINVAL;
}

int vc_mipi_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	return v4l2_g_ctrl(sd->ctrl_handler, control);
}


// --- v4l2_subdev_video_ops ---------------------------------------------------

int vc_mipi_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	TRACE

	return 0;
}

int vc_mipi_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
 	TRACE
	
	return 0;
}

int vc_mipi_s_stream(struct v4l2_subdev *sd, int enable)
{
 	TRACE
	
	return 0;
}


// --- v4l2_subdev_pad_ops ---------------------------------------------------

int vc_mipi_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	TRACE
	
	return 0;
}

int vc_mipi_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *format)
{
	TRACE
	
	return 0;
}

int vc_mipi_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *format)
{
 	TRACE

	return 0;
}

int vc_mipi_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	TRACE

	return 0;
}

int vc_mipi_enum_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_interval_enum *fie)
{
	TRACE
	
	return 0;
}



// *** Initialisation *********************************************************

const struct v4l2_subdev_core_ops vc_mipi_core_ops = {
	.s_power = vc_mipi_s_power,
	.queryctrl = vc_mipi_queryctrl,
	.try_ext_ctrls = vc_mipi_try_ext_ctrls,
	.s_ctrl = vc_mipi_s_ctrl,
	.g_ctrl = vc_mipi_g_ctrl,
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

// imx8-mipi-csi2.c don't uses the ctrl ops. We have to handle all ops thru the subdev.
const struct v4l2_ctrl_ops vc_mipi_ctrl_ops = {
	// .queryctrl = vc_mipi_queryctrl,
	// .try_ext_ctrls = vc_mipi_try_ext_ctrls,
	// .s_ctrl = vc_mipi_s_ctrl,
	// .g_ctrl = vc_mipi_g_ctrl,
};

struct v4l2_ctrl_config ctrl_config_list[] =
{
{
     	// .ops = &vc_mipi_ctrl_ops,
	.id = V4L2_CID_GAIN,
	.name = "Gain",			// Do not change the name field for the controls!
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.min = 0,
	.max = 0xfff,
	.def = 0,
	.step = 1,
},
{
	// .ops = &vc_mipi_ctrl_ops,
	.id = V4L2_CID_EXPOSURE,
	.name = "Exposure",		// Do not change the name field for the controls!
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.min = 0,
	.max = 16000000,
	.def = 0,
	.step = 1,
},
};

int vc_mipi_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct v4l2_ctrl_handler* ctrl_hdl;
	struct v4l2_ctrl *ctrl;
	int num_ctrls = ARRAY_SIZE(ctrl_config_list);
    	int ret, i;

	// TRACE

	// Initializes the subdevice
	v4l2_i2c_subdev_init(sd, client, &vc_mipi_subdev_ops);

	// Initializes the ctrls
	ctrl_hdl = devm_kzalloc(dev, sizeof(*ctrl_hdl), GFP_KERNEL);
    	ret = v4l2_ctrl_handler_init(ctrl_hdl, 2);
    	if (ret) {
		dev_err(dev, "[vc-mipi subdev] %s: Failed to init control handler\n",  __func__);
		return ret;
	}

	for (i = 0; i < num_ctrls; i++) {
		// ctrl_config_list[i].ops = &vc_mipi_ctrl_ops;
        	ctrl = v4l2_ctrl_new_custom(ctrl_hdl, &ctrl_config_list[i], NULL);
        	if (ctrl == NULL) {
            		dev_err(dev, "[vc-mipi subdev] %s: Failed to init %s ctrl\n",  __func__, ctrl_config_list[i].name);
            		continue;
        	}
	}

	if (ctrl_hdl->error) {
		ret = ctrl_hdl->error;
		dev_err(dev, "[vc-mipi subdev] %s: control init failed (%d)\n", __func__, ret);
		goto error;
	}

	sd->ctrl_handler = ctrl_hdl;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdl);
	return ret;
}