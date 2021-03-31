/*
* VC MIPI driver common code.
*
* Copyright (c) 2020, Vision Components GmbH <mipi-tech@vision-components.com>"
*
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-common.h>

#include "vc_mipi.h"

/****** reg_write = Write reg = 07.2020 *********************************/
int reg_write(struct i2c_client *client, const u16 addr, const u8 data)
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg;
    u8 tx[3];
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
EXPORT_SYMBOL(reg_write);

/****** reg_read = Read reg = 07.2020 ***********************************/
int reg_read(struct i2c_client *client, const u16 addr)
{
    u8 buf[2] = {addr >> 8, addr & 0xff};
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
        dev_err(&client->dev, "Reading register %x from %x failed\n",
             addr, client->addr);
        return ret;
    }

    return buf[0];
}
EXPORT_SYMBOL(reg_read);

/****** reg_write_table = Write reg table = 07.2020 *********************/
int reg_write_table(struct i2c_client *client,
               const struct sensor_reg table[])
{
    const struct sensor_reg *reg;
    int ret;

    for (reg = table; reg->addr != SENSOR_TABLE_END; reg++) {
        ret = reg_write(client, reg->addr, reg->val);
        if (ret < 0)
            return ret;
    }

    return 0;
}
EXPORT_SYMBOL(reg_write_table);

/****** vc_mipi_common_rom_init = VC MIPI reset (ROM init) = 11.2020 ****/
int vc_mipi_common_rom_init(struct i2c_client *client, struct i2c_client *rom, int _sensor_mode)
{

#define TRACE_VC_MIPI_COMMON_ROM_INIT   0   /* DDD - vc_mipi_common_rom_init() - trace */

    static int try=1;
    int addr,reg,data;
    int err = 0;

    if(!rom)
    {
        dev_err(&client->dev, "%s: ERROR: VC FPGA not present !!!\n", __func__);
        err = -EIO;
    }

    addr = 0x0100; // reset
    data =      2; // powerdown sensor
    reg = reg_write(rom, addr, data);
    if(reg)
        return -EIO;

    if(_sensor_mode < 0)
    {
        mdelay(200); // wait 200ms

        addr = 0x0101; // status
        reg = reg_read(rom, addr);
#if TRACE_VC_MIPI_COMMON_ROM_INIT
        dev_err(&client->dev, "VC_SEN_MODE=%d PowerOFF STATUS=0x%02x\n",_sensor_mode,reg);
#endif
        return 0;
    }

    addr = 0x0102; // mode
    data = _sensor_mode; // default 10-bit streaming
    reg = reg_write(rom, addr, data);
    if(reg)
        return -EIO;

    addr = 0x0100; // reset
    data =      0; // powerup sensor
    reg = reg_write(rom, addr, data);
    if(reg)
        return -EIO;

    while(1)
    {
        mdelay(100); // wait 200ms

        addr = 0x0101; // status
        reg = reg_read(rom, addr);

        if(reg & 0x80)
        {
           err = 0;
           break;
        }

        if(reg & 0x01)
        {
            dev_err(&client->dev, "%s: !!! ERROR !!! setting VC_SEN_MODE=%d STATUS=0x%02x try=%d\n",__func__,_sensor_mode,reg,try);
            err = -EIO;
        }

        if(try++ >  4)
            break;
    }

#if TRACE_VC_MIPI_COMMON_ROM_INIT
//    dev_err(&client->dev, "%s: VC_SEN_MODE=%d PowerOn STATUS=0x%02x try=%d\n",__func__, _sensor_mode,reg,try);
    dev_err(&client->dev, "%s: sensor_mode=%d STATUS=0x%02x try=%d err=%d\n", __func__, _sensor_mode, reg, try, err);
#else
    if(err)
    {
      dev_err(&client->dev, "%s: sensor_mode=%d STATUS=0x%02x try=%d ERROR=%d\n", __func__, _sensor_mode, reg, try, err);
    }
#endif

    try = 1; // reset try counter

    return err;
}
EXPORT_SYMBOL(vc_mipi_common_rom_init);

/****** vc_mipi_common_trigmode_write = Trigger mode setup = 10.2020 ****/
int vc_mipi_common_trigmode_write(struct i2c_client *rom, u32 sensor_ext_trig, u32 exposure_time, u32 io_config, u32 enable_extrig, u32 sen_clk)
{
    int ret;

    if(sensor_ext_trig)
    {
        u64 exposure = (exposure_time * (sen_clk/1000000)); // sen_clk default=54Mhz imx183=72Mhz

        //exposure = (exposure_time * 24 or 25?); //TODO OV9281
        //exposure = (exposure_time * 54); //default
        //exposure = (exposure_time * 72); //TEST IMX183
        //printk("ext_trig exposure=%lld",exposure);

        int addr = 0x0108; // ext trig enable
        //int data =      1; // external trigger enable
        //int data =      2; // external static trigger variable shutter enable
        //int data =      4; // TEST external self trigger enable
        int data = enable_extrig; // external trigger enable
        ret = reg_write(rom, addr, data);

        addr = 0x0103; // io configuration
        data = io_config;
        ret |= reg_write(rom, addr, data);

        addr = 0x0109; // shutter lsb
        data = exposure & 0xff;
        ret |= reg_write(rom, addr, data);

        addr = 0x010a;
        data = (exposure >> 8) & 0xff;
        ret |= reg_write(rom, addr, data);

        addr = 0x010b;
        data = (exposure >> 16) & 0xff;
        ret |= reg_write(rom, addr, data);

        addr = 0x010c; // shutter msb
        data = (exposure >> 24) & 0xff;
        ret |= reg_write(rom, addr, data);

    }
    else
    {
        int addr = 0x0108; // ext trig enable
        int data =      0; // external trigger disable
        ret = reg_write(rom, addr, data);

        addr = 0x0103; // io configuration
        data = io_config;
        ret |= reg_write(rom, addr, data);
    }
    return ret;
}
EXPORT_SYMBOL(vc_mipi_common_trigmode_write);

