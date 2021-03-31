#ifndef _VC_MIPI_TYPES
#define _VC_MIPI_TYPES

#include <linux/types.h>


#define MOD_ID_OV9281   0x9281
#define MOD_ID_IMX183   0x0183
#define MOD_ID_IMX226   0x0226
#define MOD_ID_IMX250   0x0250
#define MOD_ID_IMX252   0x0252
#define MOD_ID_IMX273   0x0273
#define MOD_ID_IMX290   0x0290
#define MOD_ID_IMX296   0x0296
#define MOD_ID_IMX297   0x0297
#define MOD_ID_IMX327   0x0327
#define MOD_ID_IMX415   0x0415


struct vc_mipi_hw_desc {
    char   magic[12];
    char   manuf[32];
    u16    manuf_id;
    char   sen_manuf[8];
    char   sen_type[16];
    u16    mod_id;
    u16    mod_rev;
    char   regs[56];
    u16    nr_modes;
    u16    bytes_per_mode;
    char   mode1[16];
    char   mode2[16];
    char   mode3[16];
    char   mode4[16];
};

#endif // _VC_MIPI_TYPES