#include "vc_mipi_ctrl.h"
#include "vc_mipi_utils.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_ctrl.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

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
        dev_err(dev, "[vc-mipi driver] VC MIPI Controller - Hardware Descriptor\n");
        dev_err(dev, "[vc-mipi driver] [ MAGIC  ] [ %s ]\n", hw_desc->magic);
        dev_err(dev, "[vc-mipi driver] [ MANUF. ] [ %s ] [ MID=0x%04x ]\n", hw_desc->manuf, hw_desc->manuf_id);
        dev_err(dev, "[vc-mipi driver] [ SENSOR ] [ %s %s ]\n", hw_desc->sen_manuf, hw_desc->sen_type);
        dev_err(dev, "[vc-mipi driver] [ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n", hw_desc->mod_id, hw_desc->mod_rev);
        dev_err(dev, "[vc-mipi driver] [ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n", hw_desc->nr_modes, hw_desc->bytes_per_mode);
}

struct i2c_client* vc_mipi_ctrl_get_client(struct i2c_adapter *adapter, u8 addr)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("dummy", addr),
    };
    unsigned short addr_list[2] = { addr, I2C_CLIENT_END };

    return i2c_new_probed_device(adapter, &info, addr_list, NULL);
}

int vc_mipi_ctrl_is_color_sensor(struct imx226_dev *sensor)
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

int vc_mipi_ctrl_reset_regs(struct imx226_dev *sensor)
{
        TRACE
        // priv->sensor_mode = sensor_mode;
        // err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
        // if(err)
        // {
        //   dev_err(dev, "[vc-mipi driver] %s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
        //   return err;
        // }
        return 0;
}

int vc_mipi_ctrl_setup(struct imx226_dev *sensor)
{
        struct i2c_client* client = sensor->i2c_client;
        struct i2c_adapter* adapter = client->adapter;
        struct device* dev = &client->dev;
        struct i2c_client* client_ctrl;
        struct vc_mipi_hw_desc* hw_desc = &sensor->hw_desc;
        int ret;

        TRACE

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
                dev_err(dev, "[vc-mipi driver] %s(): I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n", __func__);
                return -EIO;
        }

        client_ctrl = vc_mipi_ctrl_get_client(adapter, 0x10);
        if (client_ctrl) {
                int addr,reg;
                for (addr=0; addr<sizeof(*hw_desc); addr++) {
                        reg = reg_read(client_ctrl, addr+0x1000);
                        if (reg < 0) {
                                i2c_unregister_device(client_ctrl);
                                return -EIO;
                        }
                        *((char *)(hw_desc)+addr)=(char)reg;

                        //vc_mipi_dump_reg_value(dev, addr, reg);
                }

                vc_mipi_dump_hw_desc(dev, hw_desc);
        }

        ret = vc_mipi_ctrl_reset_regs(sensor);
        if (ret) {
                return -EIO;
        }
        return 0;
}