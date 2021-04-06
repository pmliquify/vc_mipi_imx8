#include "vc_mipi_subdev.h"
#include "vc_mipi_driver.h"
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_subdev.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE



// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_mipi_s_power(struct v4l2_subdev *sd, int on)
{
	struct vc_mipi_driver *driver = to_vc_mipi_driver(sd);
	struct i2c_client *client = driver->module.client_mod;
	struct device *dev = &client->dev;
	int ret;

 	TRACE

	dev_info(dev, "[vc-mipi driver] %s: On=%d\n", __FUNCTION__, on); 

	if (on) {
		/* restore controls */
		ret = v4l2_ctrl_handler_setup(&driver->ctrls.handler);
		if(ret) {
			dev_err(dev, "[vc-mipi driver] %s: Failed to restore control handler\n",  __func__);
		}
	}
	
	return ret;
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

const struct v4l2_subdev_core_ops vc_mipi_core_ops = {
	.s_power = vc_mipi_s_power,
	// .log_status = v4l2_ctrl_subdev_log_status,
	// .subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	// .unsubscribe_event = v4l2_event_subdev_unsubscribe,
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

void vc_mipi_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client)
{
	v4l2_i2c_subdev_init(sd, client, &vc_mipi_subdev_ops);
}