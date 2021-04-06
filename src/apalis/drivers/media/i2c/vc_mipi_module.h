#ifndef _VC_MIPI_MOD_H
#define _VC_MIPI_MOD_H

#include <linux/types.h>
#include <linux/i2c.h>

#define MOD_ID_OV9281       0x9281
#define MOD_ID_IMX183       0x0183
#define MOD_ID_IMX226       0x0226
#define MOD_ID_IMX250       0x0250
#define MOD_ID_IMX252       0x0252
#define MOD_ID_IMX273       0x0273
#define MOD_ID_IMX290       0x0290
#define MOD_ID_IMX296       0x0296
#define MOD_ID_IMX297       0x0297
#define MOD_ID_IMX327       0x0327
#define MOD_ID_IMX415       0x0415

#define MOD_REG_RESET       0x0100 // register 0 [0x0100]: reset and init register (R/W)
#define MOD_REG_STATUS      0x0101 // register 1 [0x0101]: status (R) 
#define MOD_REG_MODE        0x0102 // register 2 [0x0102]: initialisation mode (R/W)
#define MOD_REG_IOCTRL      0x0103 // register 3 [0x0103]: input/output control (R/W)
#define MOD_REG_MOD_ADDR    0x0104 // register 4 [0x0104]: module i2c address (R/W, default: 0x10)
#define MOD_REG_SEN_ADDR    0x0105 // register 5 [0x0105]: sensor i2c address (R/W, default: 0x1A)
#define MOD_REG_OUTPUT      0x0106 // register 6 [0x0106]: output signal override register (R/W, default: 0x00)
#define MOD_REG_INPUT       0x0107 // register 7 [0x0107]: input signal status register (R)
#define MOD_REG_EXTTRIG     0x0108 // register 8 [0x0108]: external trigger enable (R/W, default: 0x00)
#define MOD_REG_EXP_1       0x0109 // register 9  [0x0109]: exposure LSB (R/W, default: 0x10)
#define MOD_REG_EXP_2       0x010A // register 10 [0x010A]: exposure 	   (R/W, default: 0x27)
#define MOD_REG_EXP_3       0x010B // register 11 [0x010B]: exposure     (R/W, default: 0x00)
#define MOD_REG_EXP_4       0x010C // register 12 [0x010C]: exposure MSB (R/W, default: 0x00)
#define MOD_REG_RETRIG_1    0x010D // register 13 [0x010D]: retrigger LSB (R/W, default: 0x40)
#define MOD_REG_RETRIG_2    0x010E // register 14 [0x010E]: retrigger     (R/W, default: 0x2D)
#define MOD_REG_RETRIG_3    0x010F // register 15 [0x010F]: retrigger     (R/W, default: 0x29)
#define MOD_REG_RETRIG_4    0x0110 // register 16 [0x0110]: retrigger MSB (R/W, default: 0x00)

#define REG_RESET_PWR_UP    0x00
#define REG_RESET_SENSOR    0x01   // reg0[0] = 0 sensor reset the sensor is held in reset when this bit is 1
#define REG_RESET_PWR_DOWN  0x02   // reg0[1] = 0 power down power for the sensor is switched off

#define REG_STATUS_NO_COM   0x00   // reg1[7:0] = 0x00 default, no communication with sensor possible
#define REG_STATUS_READY    0x80   // reg1[7:0] = 0x80 sensor ready after successful initialization sequence
#define REG_STATUS_ERROR    0x01   // reg1[7:0] = 0x01 internal error during initialization

struct vc_mipi_chip_desc {
        u16    chip_id_high;
        u16    chip_id_low;
        u16    chip_rev;
        char   regs[50];   
};

struct vc_mipi_mod_desc {
        char   magic[12];
        char   manuf[32];
        u16    manuf_id;
        char   sen_manuf[8];
        char   sen_type[16];
        u16    mod_id;
        u16    mod_rev;
        struct vc_mipi_chip_desc chip;   
        u16    nr_modes;
        u16    bytes_per_mode;
        char   mode1[16];
        char   mode2[16];
        char   mode3[16];
        char   mode4[16];
};

struct vc_mipi_module {
        struct i2c_client *client_mod;
 	struct i2c_client *client_sen;
         struct vc_mipi_mod_desc desc;
};

int vc_mipi_mod_setup(struct vc_mipi_module *module);
int vc_mipi_mod_is_color_sensor(struct vc_mipi_module *module);

#endif // _VC_MIPI_MOD_H