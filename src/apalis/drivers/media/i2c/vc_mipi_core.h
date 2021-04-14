#ifndef _VC_MIPI_MOD_H
#define _VC_MIPI_MOD_H

#include <linux/types.h>
#include <linux/i2c.h>


// --- Helper Functions for the VC MIPI Controller Module -----------------------------------------

struct vc_mipi_chip_desc {
        __u16  chip_id_high;
        __u16  chip_id_low;
        __u16  chip_rev;
        char   regs[50];   
};

struct vc_mipi_mod_desc {
        char   magic[12];
        char   manuf[32];
        __u16  manuf_id;
        char   sen_manuf[8];
        char   sen_type[16];
        __u16  mod_id;
        __u16  mod_rev;
        struct vc_mipi_chip_desc chip;   
        __u16  nr_modes;
        __u16  bytes_per_mode;
        char   mode1[16];
        char   mode2[16];
        char   mode3[16];
        char   mode4[16];
};

struct i2c_client *vc_mipi_mod_setup(struct i2c_client *client_sen, struct vc_mipi_mod_desc *desc);
int vc_mipi_mod_is_color_sensor(struct vc_mipi_mod_desc *desc);
int vc_mipi_mod_set_exposure(struct i2c_client* client_mod, __u32 value, __u32 sen_clk);


// --- Helper Functions for the VC MIPI Sensors ---------------------------------------------------

int vc_mipi_sen_set_exposure(struct i2c_client *client_sen, int value);
int vc_mipi_sen_set_gain(struct i2c_client *client_sen, int value);


#endif // _VC_MIPI_MOD_H