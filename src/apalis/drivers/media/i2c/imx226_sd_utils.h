#ifndef _IMX226_SD_UTILS_H
#define _IMX226_SD_UTILS_H

#include "imx226.h"

int imx226_set_power(struct imx226_dev *sensor, bool on);
int imx226_try_frame_interval(struct imx226_dev *sensor, struct v4l2_fract *fi, u32 width, u32 height);

int imx226_try_fmt_internal(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt, enum imx226_frame_rate fr, const struct imx226_mode_info **new_mode);
int imx226_set_framefmt(struct imx226_dev *sensor, struct v4l2_mbus_framefmt *format);

const struct imx226_mode_info * imx226_find_mode(struct imx226_dev *sensor, enum imx226_frame_rate fr, int width, int height, bool nearest);
int imx226_check_valid_mode(struct imx226_dev *sensor, const struct imx226_mode_info *mode, enum imx226_frame_rate rate);

int imx226_set_mode(struct imx226_dev *sensor);
int imx226_set_stream_mipi(struct imx226_dev *sensor, bool on);
int imx226_set_stream_dvp(struct imx226_dev *sensor, bool on);
int imx226_get_gain(struct imx226_dev *sensor);
int imx226_get_exposure(struct imx226_dev *sensor);

int imx226_set_ctrl_hue(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_contrast(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_saturation(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_white_balance(struct imx226_dev *sensor, int awb);
int imx226_set_ctrl_exposure(struct imx226_dev *sensor, enum v4l2_exposure_auto_type auto_exposure);
int imx226_set_ctrl_gain(struct imx226_dev *sensor, bool auto_gain);
int imx226_set_ctrl_test_pattern(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_light_freq(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_hflip(struct imx226_dev *sensor, int value);
int imx226_set_ctrl_vflip(struct imx226_dev *sensor, int value);

#endif // _IMX226_SD_UTILS_H