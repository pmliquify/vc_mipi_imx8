#ifndef _VC_MIPI_MOD_H
#define _VC_MIPI_MOD_H

#include "imx226.h"

int vc_mipi_mod_setup(struct imx226_dev *sensor);
int vc_mipi_mod_is_color_sensor(struct imx226_dev *sensor);

#endif // _VC_MIPI_MOD_H