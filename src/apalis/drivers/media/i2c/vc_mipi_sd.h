#ifndef _VC_MIPI_SD_H
#define _VC_MIPI_SD_H

#include "vc_mipi.h"

// v4l2_subdev_core_ops
int vc_mipi_s_power(struct v4l2_subdev *sd, int on);

// v4l2_subdev_video_ops
int vc_mipi_g_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi);
int vc_mipi_s_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi);
int vc_mipi_s_stream(struct v4l2_subdev *sd, int enable);

// v4l2_subdev_pad_ops
int vc_mipi_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_mbus_code_enum *code);
int vc_mipi_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format);
int vc_mipi_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format);
int vc_mipi_enum_frame_size(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_frame_size_enum *fse);
int vc_mipi_enum_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_frame_interval_enum *fie);

// Utility functions
int vc_mipi_init_controls(struct vc_mipi_dev *sensor);

#endif // _VC_MIPI_SD_H