#ifndef _IMX226_SD_H
#define _IMX226_SD_H

#include "imx226.h"

// v4l2_subdev_core_ops
int imx226_s_power(struct v4l2_subdev *sd, int on);

// v4l2_subdev_video_ops
int imx226_g_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi);
int imx226_s_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi);
int imx226_s_stream(struct v4l2_subdev *sd, int enable);

// v4l2_subdev_pad_ops
int imx226_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_mbus_code_enum *code);
int imx226_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format);
int imx226_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format);
int imx226_enum_frame_size(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_frame_size_enum *fse);
int imx226_enum_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_frame_interval_enum *fie);

// Utility functions
int imx226_init_controls(struct imx226_dev *sensor);

#endif // _IMX226_SD_H