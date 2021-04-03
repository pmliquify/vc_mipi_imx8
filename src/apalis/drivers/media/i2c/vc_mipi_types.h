#ifndef _VC_MIPI_TYPES
#define _VC_MIPI_TYPES

#include <linux/types.h>

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

/*............. Sensor models */
enum sen_model {
    SEN_MODEL_UNKNOWN = 0,
    OV7251_MODEL_MONOCHROME,
    OV7251_MODEL_COLOR,
//    OV9281_MODEL,
    OV9281_MODEL_MONOCHROME,
    OV9281_MODEL_COLOR,

    IMX183_MODEL_MONOCHROME,
    IMX183_MODEL_COLOR,
    IMX226_MODEL_MONOCHROME,
    IMX226_MODEL_COLOR,
    IMX250_MODEL_MONOCHROME,
    IMX250_MODEL_COLOR,
    IMX252_MODEL_MONOCHROME,
    IMX252_MODEL_COLOR,
    IMX273_MODEL_MONOCHROME,
    IMX273_MODEL_COLOR,

    IMX290_MODEL_MONOCHROME,
    IMX290_MODEL_COLOR,
    IMX296_MODEL_MONOCHROME,
    IMX296_MODEL_COLOR,
    IMX297_MODEL_MONOCHROME,
    IMX297_MODEL_COLOR,

    IMX327C_MODEL,
    IMX327_MODEL_MONOCHROME,
    IMX327_MODEL_COLOR,
    IMX412_MODEL_MONOCHROME,
    IMX412_MODEL_COLOR,
    IMX415_MODEL_MONOCHROME,
    IMX415_MODEL_COLOR,
};

/*............. Sensor register params */
#define SENSOR_TABLE_END        0xffff

struct sensor_reg {
    u16 addr;
    u8 val;
};

/*............. Sensor params */
struct sensor_params {
    u32 gain_min;
    u32 gain_max;
    u32 gain_default;
    u32 exposure_min;
    u32 exposure_max;
    u32 exposure_default;
    u32 frame_dx;
    u32 frame_dy;
    struct sensor_reg *sensor_start_table;
    struct sensor_reg *sensor_stop_table;
    struct sensor_reg *sensor_mode_table;
};

struct imx_datafmt {
    u32 code;
    enum v4l2_colorspace        colorspace;
};

struct vc_mipi_module {
	struct i2c_client *i2c_client_mod;
	struct vc_mipi_mod_desc mod_desc;
	struct sensor_params sen_pars;
	struct v4l2_pix_format       pix;

	u32 digital_gain;
	u32 exposure_time;
	int color;              // color flag: 0=no, 1=yes
	int model;              // sensor model
	char model_name[32];    // sensor model name, e.g. "IMX327 color"

	int sensor_mode;        // sensor mode
	int num_lanes;          // # of data lanes: 1, 2, 4
	int sensor_ext_trig;    // ext. trigger flag: 0=no, 1=yes
	int flash_output;       // flash output enable
	int sen_clk;            // sen_clk: default=54Mhz imx183=72Mhz
	int fpga_addr;          // FPGA i2c address (def = 0x10)
};

#endif // _VC_MIPI_TYPES