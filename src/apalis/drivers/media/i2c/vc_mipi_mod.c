#include "vc_mipi_mod.h"
#include "vc_mipi_utils.h"

//#define TRACE printk("        TRACE [vc-mipi] vc_mipi_mod.c --->  %s : %d", __FUNCTION__, __LINE__);
#define TRACE

void vc_mipi_dump_reg_value(struct device* dev, int addr, int reg) 
{
        int sval = 0;   // short 2-byte value
        if (addr & 1) { // odd addr
            sval |= (int)reg << 8;
            dev_err(dev, "[vc-mipi driver] addr=0x%04x reg=0x%04x\n",addr+0x1000-1,sval);
        }     
}

void vc_mipi_dump_hw_desc(struct device* dev, struct vc_mipi_hw_desc* hw_desc)
{
        dev_info(dev, "[vc-mipi driver] VC MIPI Module - Hardware Descriptor\n");
        dev_info(dev, "[vc-mipi driver] [ MAGIC  ] [ %s ]\n", hw_desc->magic);
        dev_info(dev, "[vc-mipi driver] [ MANUF. ] [ %s ] [ MID=0x%04x ]\n", hw_desc->manuf, hw_desc->manuf_id);
        dev_info(dev, "[vc-mipi driver] [ SENSOR ] [ %s %s ]\n", hw_desc->sen_manuf, hw_desc->sen_type);
        dev_info(dev, "[vc-mipi driver] [ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n", hw_desc->mod_id, hw_desc->mod_rev);
        dev_info(dev, "[vc-mipi driver] [ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n", hw_desc->nr_modes, hw_desc->bytes_per_mode);
}

struct i2c_client* vc_mipi_mod_get_client(struct i2c_adapter *adapter, u8 addr)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("dummy", addr),
    };
    unsigned short addr_list[2] = { addr, I2C_CLIENT_END };

    return i2c_new_probed_device(adapter, &info, addr_list, NULL);
}

int vc_mipi_mod_sensor_power_down(struct imx226_dev *sensor) 
{
        TRACE
        return reg_write(sensor->i2c_client_mod, MOD_REG_RESET, REG_RESET_PWR_DOWN);
}

int vc_mipi_mod_sensor_power_up(struct imx226_dev *sensor) 
{
        TRACE
        return reg_write(sensor->i2c_client_mod, MOD_REG_RESET, REG_RESET_PWR_DOWN);
}

int vc_mipi_mod_get_status(struct imx226_dev *sensor) 
{
        TRACE
        return reg_read(sensor->i2c_client_mod, MOD_REG_STATUS);
}

int vc_mipi_mod_wait_until_sensor_is_ready(struct imx226_dev *sensor) 
{
        struct i2c_client* client = sensor->i2c_client;
        struct device* dev = &client->dev;
        int status;
        int try;

        TRACE

        status = REG_STATUS_NO_COM;
        try = 0;
        while(status == REG_STATUS_NO_COM && try < 10) {
                mdelay(100);
                status = vc_mipi_mod_get_status(sensor);
                try++;
        }
        if(status == REG_STATUS_ERROR) {
                dev_err(dev, "[vc-mipi driver] %s(): Internal Error!", __func__);
        }
        return status;
}

int vc_mipi_mod_set_mode(struct imx226_dev *sensor, int mode)
{
        TRACE
        return reg_write(sensor->i2c_client_mod, MOD_REG_MODE, mode);
} 

int vc_mipi_mod_reset_sensor(struct imx226_dev *sensor, int mode)
{
        int ret;
        
        TRACE

        ret = vc_mipi_mod_sensor_power_down(sensor);
        if (ret) {
                return -EIO;
        }
        // TODO: Check what if sensor_mode < 0
        mdelay(200);
        // TODO: Check status and print it!?
        // TODO: Check if it is neccessary to set mode when sensor is down.
        ret = vc_mipi_mod_set_mode(sensor, mode);
        if (ret) {
                return -EIO;
        }
        ret = vc_mipi_mod_sensor_power_up(sensor);
        if (ret) {
                return -EIO;
        }
        ret = vc_mipi_mod_wait_until_sensor_is_ready(sensor);
        if (ret) {
                return -EIO;
        }
        return 0;
}

int vc_mipi_mod_setup(struct imx226_dev *sensor)
{
        struct i2c_client* client = sensor->i2c_client;
        struct i2c_adapter* adapter = client->adapter;
        struct device* dev = &client->dev;
        struct vc_mipi_hw_desc* hw_desc = &sensor->hw_desc;
        int ret;

        TRACE

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
                dev_err(dev, "[vc-mipi driver] %s(): I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n", __FUNCTION__);
                return -EIO;
        }

        sensor->i2c_client_mod = vc_mipi_mod_get_client(adapter, 0x10);
        if (sensor->i2c_client_mod) {
                int addr,reg;
                for (addr=0; addr<sizeof(*hw_desc); addr++) {
                        reg = reg_read(sensor->i2c_client_mod, addr+0x1000);
                        if (reg < 0) {
                                i2c_unregister_device(sensor->i2c_client_mod);
                                return -EIO;
                        }
                        *((char *)(hw_desc)+addr)=(char)reg;

                        //vc_mipi_dump_reg_value(dev, addr, reg);
                }

                vc_mipi_dump_hw_desc(dev, hw_desc);
        }

        dev_info(dev, "[vc-mipi driver] Reset VC MIPI Sensor");
        ret = vc_mipi_mod_reset_sensor(sensor, 0); // TODO: Set sensor mode in last parameter.
        if (ret) {
                return -EIO;
        }
        dev_info(dev, "[vc-mipi driver] VC MIPI Sensor succesfully initialized.");
        return 0;
}

int vc_mipi_mod_is_color_sensor(struct imx226_dev *sensor)
{
        struct vc_mipi_hw_desc* hw_desc = &sensor->hw_desc;

        TRACE

        if(hw_desc->sen_type) {
                u32 len = strnlen(hw_desc->sen_type, 16);
                if (len > 0 && len < 17) {
                        return *(hw_desc->sen_type+len-1) == 'C';
                }
        }
        return 0;
}