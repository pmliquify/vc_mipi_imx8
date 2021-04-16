#ifndef _VC_MIPI_MOD_H
#define _VC_MIPI_MOD_H

#include <linux/types.h>
#include <linux/i2c.h>

// --- Helper Functions for the VC MIPI Controller Module -----------------------------------------

struct vc_mipi_mod_desc {
        __u8   magic[12];
        __u8   manuf[32];
        __u16  manuf_id;
        __u8   sen_manuf[8];
        __u8   sen_type[16];
        __u16  mod_id;
        __u16  mod_rev;
        __u16  chip_id_high;
        __u16  chip_id_low;
        __u16  chip_rev;
        __u8   regs[50];  
        __u16  nr_modes;
        __u16  bytes_per_mode;
        __u8   mode1[16];
        __u8   mode2[16];
        __u8   mode3[16];
        __u8   mode4[16];
};

struct i2c_client *vc_mipi_mod_setup(struct i2c_client *client_sen, __u8 addr_mod, struct vc_mipi_mod_desc *desc);
int vc_mipi_mod_is_color_sensor(struct vc_mipi_mod_desc *desc);
int vc_mipi_mod_reset_module(struct i2c_client* client_mod, int mode);
int vc_mipi_mod_set_exposure(struct i2c_client* client_mod, __u32 value, __u32 sen_clk);


// --- Helper Functions for the VC MIPI Sensors ---------------------------------------------------

struct sensor_reg {
    __u16 addr;
    __u8 val;
};

int vc_mipi_sen_write_table(struct i2c_client *client_sen, struct sensor_reg table[]);
int vc_mipi_sen_set_exposure(struct i2c_client *client_sen, int value);
int vc_mipi_sen_set_gain(struct i2c_client *client_sen, int value);
int vc_mipi_sen_start_stream(struct i2c_client *client_sen, struct i2c_client *client_mod, struct sensor_reg start_table[], int flash_output);
int vc_mipi_sen_stop_stream(struct i2c_client *client_sen, struct i2c_client *client_mod, struct sensor_reg stop_table[], int mode);


#endif // _VC_MIPI_MOD_H