#include "vc_mipi_i2c.h"
#include <linux/device.h>
#include <linux/delay.h>


int i2c_read_reg(struct i2c_client *client, const __u16 addr)
{
    __u8 buf[2] = {addr >> 8, addr & 0xff};
    int ret;
    struct i2c_msg msgs[] = {
        {
            .addr  = client->addr,
            .flags = 0,
            .len   = 2,
            .buf   = buf,
        }, {
            .addr  = client->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = buf,
        },
    };

    ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
    if (ret < 0) {
        dev_err(&client->dev, "[vc-mipi driver common] Reading register %x from %x failed\n",
             addr, client->addr);
        return ret;
    }

    return buf[0];
}

int i2c_write_reg(struct i2c_client *client, const __u16 addr, const __u8 data)
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg;
    __u8 tx[3];
    int ret;

    msg.addr = client->addr;
    msg.buf = tx;
    msg.len = 3;
    msg.flags = 0;
    tx[0] = addr >> 8;
    tx[1] = addr & 0xff;
    tx[2] = data;
    ret = i2c_transfer(adap, &msg, 1);
    mdelay(2);

    return ret == 1 ? 0 : -EIO;
}