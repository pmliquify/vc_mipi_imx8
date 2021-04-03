#ifndef _VC_MIPI_MOD_H
#define _VC_MIPI_MOD_H

#include "vc_mipi.h"

int vc_mipi_mod_setup(struct vc_mipi_dev *sensor);
int vc_mipi_mod_is_color_sensor(struct vc_mipi_dev *sensor);

#endif // _VC_MIPI_MOD_H