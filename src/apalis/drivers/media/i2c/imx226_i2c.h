#ifndef _IMX226_I2C_H
#define _IMX226_I2C_H

#include "imx226.h"

int imx226_init_slave_id(struct imx226_dev *sensor);
int imx226_write_reg(struct imx226_dev *sensor, u16 reg, u8 val);
int imx226_read_reg(struct imx226_dev *sensor, u16 reg, u8 *val);
int imx226_read_reg16(struct imx226_dev *sensor, u16 reg, u16 *val);
int imx226_write_reg16(struct imx226_dev *sensor, u16 reg, u16 val);
int imx226_mod_reg(struct imx226_dev *sensor, u16 reg, u8 mask, u8 val);

#endif // _IMX226_I2C_H