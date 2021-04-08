#ifndef _VC_MIPI_SD_H
#define _VC_MIPI_SD_H

#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <linux/i2c.h>

int vc_mipi_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client);

#endif // _VC_MIPI_SD_H