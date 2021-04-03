#ifndef _VC_MIPI_UTILS
#define _VC_MIPI_UTILS

#include "vc_mipi.h"

int reg_read(struct i2c_client *client, const u16 addr);
int reg_write(struct i2c_client *client, const u16 addr, const u8 data);

#endif // _VC_MIPI_UTILS