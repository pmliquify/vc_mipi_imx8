#ifndef _VC_MIPI_SUBDEV_H
#define _VC_MIPI_SUBDEV_H

#define DEBUG

#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <linux/i2c.h>

int vc_sd_init(struct v4l2_subdev *sd, struct i2c_client *client);

#endif // _VC_MIPI_SUBDEV_H