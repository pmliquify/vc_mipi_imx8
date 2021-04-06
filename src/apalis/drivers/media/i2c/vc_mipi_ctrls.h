#ifndef _VC_MIPI_CTRLS_H
#define _VC_MIPI_CTRLS_H

#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

struct vc_mipi_ctrls {
	struct v4l2_ctrl_handler handler;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *gain;
};

int vc_mipi_ctrls_init(struct v4l2_subdev *sd, struct vc_mipi_ctrls *ctrls, struct device *dev);

#endif // _VC_MIPI_CTRLS_H
