#include "vc_mipi_module.h"
#include "vc_mipi_i2c.h"
#include <linux/delay.h>

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

void vc_mipi_dump_hw_desc(struct device* dev, struct vc_mipi_mod_desc* mod_desc)
{
        dev_info(dev, "[vc-mipi driver] VC MIPI Module - Hardware Descriptor\n");
        dev_info(dev, "[vc-mipi driver] [ MAGIC  ] [ %s ]\n", mod_desc->magic);
        dev_info(dev, "[vc-mipi driver] [ MANUF. ] [ %s ] [ MID=0x%04x ]\n", mod_desc->manuf, mod_desc->manuf_id);
        dev_info(dev, "[vc-mipi driver] [ SENSOR ] [ %s %s ]\n", mod_desc->sen_manuf, mod_desc->sen_type);
        dev_info(dev, "[vc-mipi driver] [ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n", mod_desc->mod_id, mod_desc->mod_rev);
        dev_info(dev, "[vc-mipi driver] [ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n", mod_desc->nr_modes, mod_desc->bytes_per_mode);
}

struct i2c_client* vc_mipi_mod_get_client(struct i2c_adapter *adapter, u8 addr)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("dummy", addr),
    };
    unsigned short addr_list[2] = { addr, I2C_CLIENT_END };

    return i2c_new_probed_device(adapter, &info, addr_list, NULL);
}

int vc_mipi_mod_module_power_down(struct vc_mipi_module *module) 
{
        TRACE
        return i2c_write_reg(module->client_mod, MOD_REG_RESET, REG_RESET_PWR_DOWN);
}

int vc_mipi_mod_module_power_up(struct vc_mipi_module *module) 
{
        TRACE
        return i2c_write_reg(module->client_mod, MOD_REG_RESET, REG_RESET_PWR_DOWN);
}

int vc_mipi_mod_get_status(struct vc_mipi_module *module) 
{
        TRACE
        return i2c_read_reg(module->client_mod, MOD_REG_STATUS);
}

int vc_mipi_mod_wait_until_module_is_ready(struct vc_mipi_module *module) 
{
        struct i2c_client* client = module->client_mod;
        struct device* dev = &client->dev;
        int status;
        int try;

        TRACE

        status = REG_STATUS_NO_COM;
        try = 0;
        while(status == REG_STATUS_NO_COM && try < 10) {
                mdelay(100);
                status = vc_mipi_mod_get_status(module);
                try++;
        }
        if(status == REG_STATUS_ERROR) {
                dev_err(dev, "[vc-mipi driver] %s(): Internal Error!", __func__);
        }
        return status;
}

int vc_mipi_mod_set_mode(struct vc_mipi_module *module, int mode)
{
        TRACE
        return i2c_write_reg(module->client_mod, MOD_REG_MODE, mode);
} 

int vc_mipi_mod_reset_module(struct vc_mipi_module *module, int mode)
{
        int ret;
        
        TRACE

        ret = vc_mipi_mod_module_power_down(module);
        if (ret) {
                return -EIO;
        }
        // TODO: Check what if module_mode < 0
        mdelay(200);
        // TODO: Check status and print it!?
        // TODO: Check if it is neccessary to set mode when module is down.
        ret = vc_mipi_mod_set_mode(module, mode);
        if (ret) {
                return -EIO;
        }
        ret = vc_mipi_mod_module_power_up(module);
        if (ret) {
                return -EIO;
        }
        ret = vc_mipi_mod_wait_until_module_is_ready(module);
        if (ret) {
                return -EIO;
        }
        return 0;
}

int vc_mipi_mod_setup(struct vc_mipi_module *module)
{
        struct i2c_client* client = module->client_sen;
        struct i2c_adapter* adapter = client->adapter;
        struct device* dev = &client->dev;
        struct vc_mipi_mod_desc* mod_desc = &module->desc;
        int ret;

        TRACE

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
                dev_err(dev, "[vc-mipi driver] %s(): I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n", __FUNCTION__);
                return -EIO;
        }

        module->client_mod = vc_mipi_mod_get_client(adapter, 0x10);
        if (module->client_mod) {
                int addr,reg;
                for (addr=0; addr<sizeof(*mod_desc); addr++) {
                        reg = i2c_read_reg(module->client_mod, addr+0x1000);
                        if (reg < 0) {
                                i2c_unregister_device(module->client_mod);
                                return -EIO;
                        }
                        *((char *)(mod_desc)+addr)=(char)reg;

                        //vc_mipi_dump_reg_value(dev, addr, reg);
                }

                vc_mipi_dump_hw_desc(dev, mod_desc);
        }

        dev_info(dev, "[vc-mipi driver] Reset VC MIPI Sensor");
        ret = vc_mipi_mod_reset_module(module, 0); // TODO: Set module mode in last parameter.
        if (ret) {
                return -EIO;
        }
        dev_info(dev, "[vc-mipi driver] VC MIPI Sensor succesfully initialized.");
        return 0;
}

int vc_mipi_mod_is_color_module(struct vc_mipi_module *module)
{
        struct vc_mipi_mod_desc* mod_desc = &module->desc;

        TRACE

        if(mod_desc->sen_type) {
                u32 len = strnlen(mod_desc->sen_type, 16);
                if (len > 0 && len < 17) {
                        return *(mod_desc->sen_type+len-1) == 'C';
                }
        }
        return 0;
}