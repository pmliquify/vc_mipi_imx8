/*
 * VC MIPI driver header file.
 *
 * Copyright (c) 2020, Vision Components GmbH <mipi-tech@vision-components.com>"
 *
 */

#ifndef _VC_MIPI_H      /* [[[ */
#define _VC_MIPI_H

#include <linux/types.h>
#include <linux/i2c.h>


/*............. Driver version */
#define VC_MIPI_VERS    100     /* Version number (100 for 1.00)      */
#define LPR_SUB_VERS     ""     /* Sub-version letter or empty string */
#define LPR_VERS_STR    "1.00"  /* Version string                     */

/*............. Bytes per pixel (BPP) defines: */
#define RAW10_BPP    2
#define RAW12_BPP    2

//#define MIN_FPS                       15
//#define MAX_FPS                       120
//#define DEFAULT_FPS                   30

/*............. User control handler methods */
#define USER_CTRL_NONE        0   /* no user controls */
#define USER_CTRL_METHOD1     1   /* method 1 (old) */
#define USER_CTRL_METHOD2     2   /* method 2 (new) */

#define CTRL_HANDLER_METHOD   USER_CTRL_METHOD2


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

//struct imx_mode {
//    u32 sensor_mode;
//    u32 sensor_depth;
//    u32 sensor_ext_trig;
//    u32 width;
//    u32 height;
//    u32 max_fps;
//    u32 hts_def;
//    u32 vts_def;
//    const struct sensor_reg *reg_list;
//};

struct vc_rom_table {
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

enum vc_rom_table_sreg_names {
    MODEL_ID_HIGH = 0,
    MODEL_ID_LOW,
    CHIP_REV,
    IDLE,
    H_START_HIGH,
    H_START_LOW,
    V_START_HIGH,
    V_START_LOW,
    H_SIZE_HIGH,
    H_SIZE_LOW,
    V_SIZE_HIGH,
    V_SIZE_LOW,
    H_OUTPUT_HIGH,
    H_OUTPUT_LOW,
    V_OUTPUT_HIGH,
    V_OUTPUT_LOW,
    EXPOSURE_HIGH,
    EXPOSURE_MIDDLE,
    EXPOSURE_LOW,
    GAIN_HIGH,
    GAIN_LOW,
    RESERVED1,
    RESERVED2,
    RESERVED3,
    RESERVED4,
    RESERVED5,
    RESERVED6,
    RESERVED7,
};

#define sen_reg(priv, addr) (*((u16 *)(&priv->rom_table.regs[(addr)*2])))

#define vc_mipi_common_reg_write  reg_write
#define vc_mipi_common_reg_read   reg_read

/*----------------------------------------------------------------------*/
/*                          Function prototypes                         */
/*----------------------------------------------------------------------*/
int reg_write(struct i2c_client *client, const u16 addr, const u8 data);
int reg_read(struct i2c_client *client, const u16 addr);
int reg_write_table(struct i2c_client *client,
               const struct sensor_reg table[]);
int vc_mipi_common_rom_init(struct i2c_client *client, struct i2c_client *rom, int _sensor_mode);
int vc_mipi_common_trigmode_write(struct i2c_client *rom, u32 sensor_ext_trig, u32 exposure_time, u32 io_config, u32 enable_extrig, u32 sen_clk);

void mx6s_get_pix_fmt(struct v4l2_pix_format *pix);
void mx6s_set_pix_fmt(struct v4l2_pix_format *pix);
int mx6s_set_ctrl_range (
    int id,             /* [in] control id                              */
    int minimum,        /* [in] minimum control value                   */
    int maximum,        /* [in] minimum control value                   */
    int default_value,  /* [in] default control value                   */
    int step );         /* [in] control's step value                    */
//void set_sensor_model (char *model);

int csi_num_lanes(void);

#endif  /* ]]] */
