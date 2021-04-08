#ifndef _VC_MIPI_SENSOR_H
#define _VC_MIPI_SENSOR_H

#include <linux/i2c.h>

int vc_mipi_sen_set_exposure(struct i2c_client *client, int value);
int vc_mipi_sen_set_gain(struct i2c_client *client, int value);

#endif // _VC_MIPI_SENSOR_H