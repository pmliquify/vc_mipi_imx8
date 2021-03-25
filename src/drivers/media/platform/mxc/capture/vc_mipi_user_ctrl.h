/*
 * VC MIPI driver user controls.
 *
 * Copyright (c) 2021, Vision Components GmbH <mipi-tech@vision-components.com>"
 *
 */

#ifndef _VC_MIPI_USER_CTRL_H      /* [[[ */
#define _VC_MIPI_USER_CTRL_H

#include <linux/types.h>
#include <media/v4l2-ctrls.h>

/*............. User controls */
static struct v4l2_ctrl_config ctrl_config_list[] =
{
/* Do not change the name field for the controls! */
  {
//      .ops = &imx_ctrl_ops,
      .id = V4L2_CID_GAIN,
      .name = "Gain",
      .type = V4L2_CTRL_TYPE_INTEGER,
      .flags = V4L2_CTRL_FLAG_SLIDER,
      .min = 0,
      .max = 0xfff,
      .def = 0,
      .step = 1,
  },
  {
//      .ops = &imx_ctrl_ops,
      .id = V4L2_CID_EXPOSURE,
      .name = "Exposure",
      .type = V4L2_CTRL_TYPE_INTEGER,
      .flags = V4L2_CTRL_FLAG_SLIDER,
      .min = 0,
      .max = 16000000,
      .def = 0,
      .step = 1,
  },
};

//static int num_ctrls = ARRAY_SIZE(ctrl_config_list);
#define MAX_NUM_CTRLS  10   // keep >= ARRAY_SIZE(ctrl_config_list)  !


#endif  /* ]]] */
