#ifndef _VC_MIPI_I2C_H
#define _VC_MIPI_I2C_H

#include <linux/i2c.h>

int i2c_read_reg(struct i2c_client *client, const __u16 addr);
int i2c_write_reg(struct i2c_client *client, const __u16 addr, const __u8 data);

#endif // _VC_MIPI_I2C_H