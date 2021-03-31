#ifndef _VC_MIPI_CONTROLLER_H
#define _VC_MIPI_CONTROLLER_H

#include "imx226.h"

int vc_mipi_ctrl_setup(struct imx226_dev *sensor);
int vc_mipi_ctrl_reset_regs(struct imx226_dev *sensor);
int vc_mipi_ctrl_is_color_sensor(struct imx226_dev *sensor);

#endif // _VC_MIPI_CONTROLLER_H