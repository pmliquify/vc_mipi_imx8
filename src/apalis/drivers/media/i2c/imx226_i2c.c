#include "imx226_i2c.h"

//#define TRACE printk("        TRACE [vc-mipi] imx226_i2c.c --->  %s : %d", __FUNCTION__, __LINE__);
#define TRACE


int imx226_init_slave_id(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg;
	u8 buf[3];
	int ret;

	TRACE

	return 0;

	if (client->addr == IMX226_DEFAULT_SLAVE_ID)
		return 0;

	buf[0] = IMX226_REG_SLAVE_ID >> 8;
	buf[1] = IMX226_REG_SLAVE_ID & 0xff;
	buf[2] = client->addr << 1;

	msg.addr = IMX226_DEFAULT_SLAVE_ID;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: failed with %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int imx226_write_reg(struct imx226_dev *sensor, u16 reg, u8 val)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg;
	u8 buf[3];
	int ret;

	TRACE

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: error: reg=%x, val=%x\n",
			__func__, reg, val);
		return ret;
	}

	return 0;
}

int imx226_read_reg(struct imx226_dev *sensor, u16 reg, u8 *val)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg[2];
	u8 buf[2];
	int ret;

	TRACE

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].addr = client->addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = 1;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: error: reg=%x\n",
			__func__, reg);
		return ret;
	}

	*val = buf[0];
	return 0;
}

int imx226_read_reg16(struct imx226_dev *sensor, u16 reg, u16 *val)
{
	u8 hi, lo;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, reg, &hi);
	if (ret)
		return ret;
	ret = imx226_read_reg(sensor, reg + 1, &lo);
	if (ret)
		return ret;

	*val = ((u16)hi << 8) | (u16)lo;
	return 0;
}

int imx226_write_reg16(struct imx226_dev *sensor, u16 reg, u16 val)
{
	int ret;

	TRACE

	ret = imx226_write_reg(sensor, reg, val >> 8);
	if (ret)
		return ret;

	return imx226_write_reg(sensor, reg + 1, val & 0xff);
}

int imx226_mod_reg(struct imx226_dev *sensor, u16 reg, u8 mask, u8 val)
{
	u8 readval;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, reg, &readval);
	if (ret)
		return ret;

	readval &= ~mask;
	val &= mask;
	val |= readval;

	return imx226_write_reg(sensor, reg, val);
}