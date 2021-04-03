#include "vc_mipi_sd.h"
//#include "vc_mipi_sd_utils.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_sd.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE



// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_mipi_s_power(struct v4l2_subdev *sd, int on)
{
// 	struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
// 	int ret = 0;

 	TRACE
	
// 	mutex_lock(&sensor->lock);

// 	/*
// 	 * If the power count is modified from 0 to != 0 or from != 0 to 0,
// 	 * update the power state.
// 	 */
// 	if (sensor->power_count == !on) {
// 		ret = vc_mipi_set_power(sensor, !!on);
// 		if (ret)
// 			goto out;
// 	}

// 	/* Update the power count. */
// 	sensor->power_count += on ? 1 : -1;
// 	WARN_ON(sensor->power_count < 0);
// out:
// 	mutex_unlock(&sensor->lock);

// 	if (on && !ret && sensor->power_count == 1) {
// 		/* restore controls */
// 		ret = v4l2_ctrl_handler_setup(&sensor->ctrls.handler);
// 	}

// 	return ret;

	return 0;
}




// --- v4l2_subdev_video_ops ---------------------------------------------------

int vc_mipi_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	// struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);

	TRACE
	
	// mutex_lock(&sensor->lock);
	// fi->interval = sensor->frame_interval;
	// mutex_unlock(&sensor->lock);

	return 0;
}

int vc_mipi_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
// 	struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
// 	const struct vc_mipi_mode_info *mode;
// 	int frame_rate, ret = 0;

 	TRACE
	
// 	if (fi->pad != 0)
// 		return -EINVAL;

// 	mutex_lock(&sensor->lock);

// 	if (sensor->streaming) {
// 		ret = -EBUSY;
// 		goto out;
// 	}

// 	mode = sensor->current_mode;

// 	frame_rate = vc_mipi_try_frame_interval(sensor, &fi->interval,
// 					       mode->hact, mode->vact);
// 	if (frame_rate < 0) {
// 		/* Always return a valid frame interval value */
// 		fi->interval = sensor->frame_interval;
// 		goto out;
// 	}

// 	mode = vc_mipi_find_mode(sensor, frame_rate, mode->hact,
// 				mode->vact, true);
// 	if (!mode) {
// 		ret = -EINVAL;
// 		goto out;
// 	}

// 	if (mode != sensor->current_mode ||
// 	    frame_rate != sensor->current_fr) {
// 		sensor->current_fr = frame_rate;
// 		sensor->frame_interval = fi->interval;
// 		sensor->current_mode = mode;
// 		sensor->pending_mode_change = true;
// 	}
// out:
// 	mutex_unlock(&sensor->lock);
// 	return ret;

	return 0;
}

int vc_mipi_s_stream(struct v4l2_subdev *sd, int enable)
{
// 	struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
// 	struct i2c_client *client = sensor->i2c_client;
// 	int ret = 0;

 	TRACE
	
// 	mutex_lock(&sensor->lock);

// 	if (sensor->streaming == !enable) {
// 		ret = vc_mipi_check_valid_mode(sensor,
// 					      sensor->current_mode,
// 					      sensor->current_fr);
// 		if (ret) {
// 			dev_err(&client->dev, "[vc-mipi vc_mipi] Not support WxH@fps=%dx%d@%d\n",
// 				sensor->current_mode->hact,
// 				sensor->current_mode->vact,
// 				vc_mipi_framerates[sensor->current_fr]);
// 			goto out;
// 		}

// 		if (enable && sensor->pending_mode_change) {
// 			ret = vc_mipi_set_mode(sensor);
// 			if (ret)
// 				goto out;
// 		}

// 		if (enable && sensor->pending_fmt_change) {
// 			ret = vc_mipi_set_framefmt(sensor, &sensor->fmt);
// 			if (ret)
// 				goto out;
// 			sensor->pending_fmt_change = false;
// 		}

// 		if (sensor->ep.bus_type == V4L2_MBUS_CSI2_DPHY)
// 			ret = vc_mipi_set_stream_mipi(sensor, enable);
// 		else
// 			ret = vc_mipi_set_stream_dvp(sensor, enable);

// 		if (!ret)
// 			sensor->streaming = enable;
// 	}
// out:
// 	mutex_unlock(&sensor->lock);
// 	return ret;

	return 0;
}



// --- v4l2_subdev_pad_ops ---------------------------------------------------

int vc_mipi_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	TRACE
	
	// if (code->pad != 0)
	// 	return -EINVAL;
	// if (code->index >= ARRAY_SIZE(vc_mipi_formats))
	// 	return -EINVAL;

	// code->code = vc_mipi_formats[code->index].code;
	return 0;
}

int vc_mipi_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	// struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
	// struct v4l2_mbus_framefmt *fmt;

	TRACE
	
	// if (format->pad != 0)
	// 	return -EINVAL;

	// mutex_lock(&sensor->lock);

	// if (format->which == V4L2_SUBDEV_FORMAT_TRY)
	// 	fmt = v4l2_subdev_get_try_format(&sensor->sd, cfg,
	// 					 format->pad);
	// else
	// 	fmt = &sensor->fmt;

	// fmt->reserved[1] = (sensor->current_fr == vc_mipi_30_FPS) ? 30 : 15;
	// format->format = *fmt;

	// mutex_unlock(&sensor->lock);
	return 0;
}

int vc_mipi_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
// 	struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
// 	const struct vc_mipi_mode_info *new_mode;
// 	struct v4l2_mbus_framefmt *mbus_fmt = &format->format;
// 	struct v4l2_mbus_framefmt *fmt;
// 	int ret;

 	TRACE
	
// 	if (format->pad != 0)
// 		return -EINVAL;

// 	mutex_lock(&sensor->lock);

// 	if (sensor->streaming) {
// 		ret = -EBUSY;
// 		goto out;
// 	}

// 	ret = vc_mipi_try_fmt_internal(sd, mbus_fmt,
// 				      sensor->current_fr, &new_mode);
// 	if (ret)
// 		goto out;

// 	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
// 		fmt = v4l2_subdev_get_try_format(sd, cfg, 0);
// 	else
// 		fmt = &sensor->fmt;

// 	*fmt = *mbus_fmt;

// 	if (new_mode != sensor->current_mode) {
// 		sensor->current_mode = new_mode;
// 		sensor->pending_mode_change = true;
// 	}
// 	if (mbus_fmt->code != sensor->fmt.code)
// 		sensor->pending_fmt_change = true;

// 	if (sensor->pending_mode_change || sensor->pending_fmt_change)
// 		sensor->fmt = *mbus_fmt;
// out:
// 	mutex_unlock(&sensor->lock);
	// return ret;

	return 0;
}

int vc_mipi_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	TRACE
	
	// if (fse->pad != 0)
	// 	return -EINVAL;
	// if (fse->index >= IMX226_NUM_MODES)
	// 	return -EINVAL;

	// fse->min_width =
	// 	vc_mipi_mode_data[fse->index].hact;
	// fse->max_width = fse->min_width;
	// fse->min_height =
	// 	vc_mipi_mode_data[fse->index].vact;
	// fse->max_height = fse->min_height;

	return 0;
}

int vc_mipi_enum_frame_interval(
	struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_frame_interval_enum *fie)
{
	// struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
	// int i, j, count;

	TRACE
	
	// if (fie->pad != 0)
	// 	return -EINVAL;
	// if (fie->index >= IMX226_NUM_FRAMERATES)
	// 	return -EINVAL;

	// if (fie->width == 0 || fie->height == 0 || fie->code == 0) {
	// 	pr_warn("[vc-mipi vc_mipi] Please assign pixel format, width and height.\n");
	// 	return -EINVAL;
	// }

	// fie->interval.numerator = 1;

	// count = 0;
	// for (i = 0; i < IMX226_NUM_FRAMERATES; i++) {
	// 	for (j = 0; j < IMX226_NUM_MODES; j++) {
	// 		if (fie->width  == vc_mipi_mode_data[j].hact &&
	// 		    fie->height == vc_mipi_mode_data[j].vact &&
	// 		    !vc_mipi_check_valid_mode(sensor, &vc_mipi_mode_data[j], i))
	// 			count++;

	// 		if (fie->index == (count - 1)) {
	// 			fie->interval.denominator = vc_mipi_framerates[i];
	// 			return 0;
	// 		}
	// 	}
	// }

	// return -EINVAL;

	return 0;
}



// --- Utility functions ------------------------------------------------------

int vc_mipi_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	// struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	// struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
	// int val;

	TRACE
	
	// /* v4l2_ctrl_lock() locks our own mutex */

	// switch (ctrl->id) {
	// case V4L2_CID_AUTOGAIN:
	// 	val = vc_mipi_get_gain(sensor);
	// 	if (val < 0)
	// 		return val;
	// 	sensor->ctrls.gain->val = val;
	// 	break;
	// case V4L2_CID_EXPOSURE_AUTO:
	// 	val = vc_mipi_get_exposure(sensor);
	// 	if (val < 0)
	// 		return val;
	// 	sensor->ctrls.exposure->val = val;
	// 	break;
	// }

	return 0;
}

int vc_mipi_s_ctrl(struct v4l2_ctrl *ctrl)
{
	// struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	// struct vc_mipi_dev *sensor = to_vc_mipi_dev(sd);
	// int ret;

	TRACE
	
	// /* v4l2_ctrl_lock() locks our own mutex */

	// /*
	//  * If the device is not powered up by the host driver do
	//  * not apply any controls to H/W at this time. Instead
	//  * the controls will be restored right after power-up.
	//  */
	// if (sensor->power_count == 0)
	// 	return 0;

	// switch (ctrl->id) {
	// case V4L2_CID_AUTOGAIN:
	// 	ret = vc_mipi_set_ctrl_gain(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_EXPOSURE_AUTO:
	// 	ret = vc_mipi_set_ctrl_exposure(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_AUTO_WHITE_BALANCE:
	// 	ret = vc_mipi_set_ctrl_white_balance(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_HUE:
	// 	ret = vc_mipi_set_ctrl_hue(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_CONTRAST:
	// 	ret = vc_mipi_set_ctrl_contrast(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_SATURATION:
	// 	ret = vc_mipi_set_ctrl_saturation(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_TEST_PATTERN:
	// 	ret = vc_mipi_set_ctrl_test_pattern(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_POWER_LINE_FREQUENCY:
	// 	ret = vc_mipi_set_ctrl_light_freq(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_HFLIP:
	// 	ret = vc_mipi_set_ctrl_hflip(sensor, ctrl->val);
	// 	break;
	// case V4L2_CID_VFLIP:
	// 	ret = vc_mipi_set_ctrl_vflip(sensor, ctrl->val);
	// 	break;
	// default:
	// 	ret = -EINVAL;
	// 	break;
	// }

	// return ret;

	return 0;
}

// const char * const test_pattern_menu[] = {
// 	"Disabled",
// 	"Color bars",
// 	"Color bars w/ rolling bar",
// 	"Color squares",
// 	"Color squares w/ rolling bar",
// };

const struct v4l2_ctrl_ops vc_mipi_ctrl_ops = {
	.g_volatile_ctrl = vc_mipi_g_volatile_ctrl,
	.s_ctrl = vc_mipi_s_ctrl,
};

int vc_mipi_init_controls(struct vc_mipi_dev *sensor)
{
	// const struct v4l2_ctrl_ops *ops = &vc_mipi_ctrl_ops;
	// struct vc_mipi_ctrls *ctrls = &sensor->ctrls;
	// struct v4l2_ctrl_handler *hdl = &ctrls->handler;
	// int ret;

	TRACE
	
	// v4l2_ctrl_handler_init(hdl, 32);

	// /* we can use our own mutex for the ctrl lock */
	// hdl->lock = &sensor->lock;

	// /* Auto/manual white balance */
	// ctrls->auto_wb = v4l2_ctrl_new_std(hdl, ops,
	// 				   V4L2_CID_AUTO_WHITE_BALANCE,
	// 				   0, 1, 1, 1);
	// ctrls->blue_balance = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_BLUE_BALANCE,
	// 					0, 4095, 1, 0);
	// ctrls->red_balance = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_RED_BALANCE,
	// 				       0, 4095, 1, 0);
	// /* Auto/manual exposure */
	// ctrls->auto_exp = v4l2_ctrl_new_std_menu(hdl, ops,
	// 					 V4L2_CID_EXPOSURE_AUTO,
	// 					 V4L2_EXPOSURE_MANUAL, 0,
	// 					 V4L2_EXPOSURE_AUTO);
	// ctrls->exposure = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_EXPOSURE,
	// 				    0, 65535, 1, 0);
	// /* Auto/manual gain */
	// ctrls->auto_gain = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_AUTOGAIN,
	// 				     0, 1, 1, 1);
	// ctrls->gain = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_GAIN,
	// 				0, 1023, 1, 0);

	// ctrls->saturation = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_SATURATION,
	// 				      0, 255, 1, 64);
	// ctrls->hue = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_HUE,
	// 			       0, 359, 1, 0);
	// ctrls->contrast = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_CONTRAST,
	// 				    0, 255, 1, 0);
	// ctrls->test_pattern =
	// 	v4l2_ctrl_new_std_menu_items(hdl, ops, V4L2_CID_TEST_PATTERN,
	// 				     ARRAY_SIZE(test_pattern_menu) - 1,
	// 				     0, 0, test_pattern_menu);
	// ctrls->hflip = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_HFLIP,
	// 				 0, 1, 1, 0);
	// ctrls->vflip = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_VFLIP,
	// 				 0, 1, 1, 0);

	// ctrls->light_freq =
	// 	v4l2_ctrl_new_std_menu(hdl, ops,
	// 			       V4L2_CID_POWER_LINE_FREQUENCY,
	// 			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
	// 			       V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	// if (hdl->error) {
	// 	ret = hdl->error;
	// 	goto free_ctrls;
	// }

	// ctrls->gain->flags |= V4L2_CTRL_FLAG_VOLATILE;
	// ctrls->exposure->flags |= V4L2_CTRL_FLAG_VOLATILE;

	// v4l2_ctrl_auto_cluster(3, &ctrls->auto_wb, 0, false);
	// v4l2_ctrl_auto_cluster(2, &ctrls->auto_gain, 0, true);
	// v4l2_ctrl_auto_cluster(2, &ctrls->auto_exp, 1, true);

	// sensor->sd.ctrl_handler = hdl;
	return 0;

// free_ctrls:
// 	v4l2_ctrl_handler_free(hdl);
// 	return ret;
// 	return 0;
}