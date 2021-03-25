/*
 * Copyright (C) 2011-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright 2018-2019 NXP
 *
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
* Original code modified by 3rd party to support VC MIPI sensors.
*/

#define VC_MIPI         1   /* CCC - vc_mipi_imx.c - VC MIPI driver */

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
#include "vc_mipi_user_ctrl.h"


#define MIN_FPS                       10
#define MAX_FPS                       120
#define DEFAULT_FPS                   30

struct imx_datafmt {
    u32 code;
    enum v4l2_colorspace        colorspace;
};

struct imx {
    struct v4l2_subdev           subdev;
    struct i2c_client           *i2c_client;
    struct v4l2_pix_format       pix;
    const struct imx_datafmt    *fmt;
    struct v4l2_captureparm      streamcap;
    bool on;

#if VC_MIPI
    struct media_pad         pad;
    struct v4l2_ctrl_handler ctrl_handler;
    struct i2c_client       *rom;
    struct vc_rom_table      rom_table;
    struct mutex mutex;

    bool streaming;
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

    struct sensor_params sen_pars;      // sensor parameters
#endif

};

/*!
 * Maintains the information on the current state of the sesor.
 */

// Valid sensor resolutions
struct imx_res {
    int width;
    int height;
};

/* Valid sensor resolutions */
static struct imx_res imx_valid_res[] = {
    [0] = {640, 480},
//    [1] = {320, 240},
//    [2] = {720, 480},
//    [3] = {1280, 720},
//    [4] = {1920, 1080},
//    [5] = {2592, 1944},
};

// static DEFINE_MUTEX(imx_mutex);

static struct imx_datafmt *imx_data_fmts = NULL;
static int                 imx_data_fmts_size = 0;

//---------------------------------------------------------------------------
#if VC_MIPI     // [[[ - VC MIPI specific code
//---------------------------------------------------------------------------


static int sensor_mode = 0;         // VC MIPI sensor mode
static int flash_output  = 0;       // flash output enable

//static int ext_trig_mode = -1;      // ext. trigger mode: -1:not set from DT, >=0:set from DT
//static int fpga_addr = 0x10;        // FPGA i2c address (def = 0x10)
//static struct sensor_params sen_pars;

#include "vc_mipi_modules.h"

#endif  // ]]] - VC_MIPI


/****** get_capturemode = Get capture mode = 11.2020 ********************/
static int get_capturemode(int width, int height)
{

#define TRACE_GET_CAPTUREMODE   0   /* DDD - get_capturemode - trace */

    int i;

    for (i = 0; i < ARRAY_SIZE(imx_valid_res); i++) {
        if ((imx_valid_res[i].width == width) &&
             (imx_valid_res[i].height == height))
        {
#if TRACE_GET_CAPTUREMODE
            pr_info("[vc_mipi] %s: width,height=%d,%d: Found: i=%d\n", __func__, width, height, i);
#endif
            return i;
        }
    }

#if TRACE_GET_CAPTUREMODE
    pr_info("[vc_mipi] %s: width,height=%d,%d: Not found: i=-1\n", __func__, width, height);
#endif

    return -1;
}

/****** to_imx = To IMX sensor = 11.2020 ********************************/
static struct imx *to_imx(const struct i2c_client *client)
{
    return container_of(i2c_get_clientdata(client), struct imx, subdev);
}

/****** imx_find_datafmt = Find data format = 11.2020 *******************/
/* Find a data format by a pixel code in an array */
static const struct imx_datafmt
            *imx_find_datafmt(u32 code)
{

#define TRACE_IMX_FIND_DATAFMT   0   /* DDD - imx_find_datafmt - trace */

    int i;

//    for (i = 0; i < ARRAY_SIZE(imx226_colour_fmts); i++)
    for (i = 0; i < imx_data_fmts_size; i++)
        if (imx_data_fmts[i].code == code)
        {
#if TRACE_IMX_FIND_DATAFMT
           pr_err("[vc_mipi] %s: Found: index=%d\n", __func__, i);
#endif
            return imx_data_fmts + i;
        }

#if TRACE_IMX_FIND_DATAFMT
    pr_err("[vc_mipi] %s: Failed\n", __func__);
#endif

    return NULL;
}

/****** imx_s_power = Set power = 11.2020 *******************************/
/*!
 * imx_s_power - V4L2 sensor interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int imx_s_power(struct v4l2_subdev *sd, int on)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct imx *priv = to_imx(client);

    priv->on = on;
    return 0;
}

/****** imx_set_gain = Set IMX gain = 11.2010 ***************************/
static int imx_set_gain(struct i2c_client *client)
{

#define TRACE_IMX_SET_GAIN      1   /* DDD - imx_set_gain() - trace */

     struct imx *priv = to_imx(client);
//    struct i2c_client *client = priv->i2c_client;
    struct device *dev = &priv->i2c_client->dev;
    int ret = 0;

//    priv->digital_gain = val;


    if(priv->digital_gain < priv->sen_pars.gain_min)  priv->digital_gain = priv->sen_pars.gain_min;
    if(priv->digital_gain > priv->sen_pars.gain_max)  priv->digital_gain = priv->sen_pars.gain_max;

#if TRACE_IMX_SET_GAIN
    dev_err(dev, "%s: Set gain = %d\n",__func__,  priv->digital_gain);
#endif

    switch(priv->model)
    {

        case OV7251_MODEL_MONOCHROME:
        case OV7251_MODEL_COLOR:
            ret = vc_mipi_common_reg_write(client, 0x3504,  0x03);
            if(sen_reg(priv, GAIN_HIGH))
                ret |= vc_mipi_common_reg_write(client, sen_reg(priv, GAIN_HIGH) ,
                        priv->digital_gain       & 0xfe);
            break;
        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            ret = vc_mipi_common_reg_write(client, 0x3507,  0x03);
            if(sen_reg(priv, GAIN_HIGH))
                ret |= vc_mipi_common_reg_write(client, sen_reg(priv, GAIN_HIGH) ,
                        priv->digital_gain       & 0xfe);
            break;
        default:
            if(sen_reg(priv, GAIN_HIGH))
                ret  = vc_mipi_common_reg_write(client, sen_reg(priv, GAIN_HIGH),
                    (priv->digital_gain >> 8) & 0xff);
            if(sen_reg(priv, GAIN_LOW))
                ret |= vc_mipi_common_reg_write(client, sen_reg(priv, GAIN_LOW) ,
                        priv->digital_gain       & 0xff);
    }
    if(ret)
      dev_err(dev, "%s: error=%d\n", __func__, ret);
    return ret;
}

//
// IMX296
// 1H period 14.815us
// NumberOfLines=1118
//
#define H1PERIOD_296 242726 // (U32)(14.815 * 16384.0)
#define NRLINES_296  (1118)
#define TOFFSET_296  233636 // (U32)(14.260 * 16384.0)
#define VMAX_296     1118
#define EXPOSURE_TIME_MIN_296  29
#define EXPOSURE_TIME_MIN2_296 16504
#define EXPOSURE_TIME_MAX_296  15534389


//
// IMX297
// 1H period 14.411us
// NumberOfLines=574
//
#define H1PERIOD_297 236106 // (U32)(14.411 * 16384.0)
#define NRLINES_297  (574)
#define TOFFSET_297  233636 // (U32)(14.260 * 16384.0)
#define VMAX_297    574
#define EXPOSURE_TIME_MIN_297  29
#define EXPOSURE_TIME_MIN2_297 8359
#define EXPOSURE_TIME_MAX_297  15110711

/****** imx_exposure_296_297 = IMX296/297 exposure = 12.2020 ************/
static int imx_exposure_296_297(struct imx *priv, s32 expMin0, s32 expMin1, s32 expMax, s32 nrLines, s32 tOffset, s32 h1Period, s32 vMax)
{
#define TRACE_IMX_EXPOSURE_296_297      0   /* DDD - imx_exposure_296_297 - trace */

//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret = 0;
    u32 exposure = 0;
    u32 base;

//    u32 denominator=priv->tpf.denominator;
//    u32 numerator=priv->tpf.numerator;
//    u32 fps=priv->cur_mode->max_fps;
//    u32 multiplier=(fps*numerator*1000)/denominator;

    u32 multiplier = 1000;   // (fps*numerator*1000)/denominator;

#if TRACE_IMX_EXPOSURE_296_297
    dev_info(&client->dev, "multiplier = %d \n",multiplier);
#endif

    switch(priv->model)
    {
#ifndef PLATFORM_QCOM
        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
            base = 0x0200;
            break;
#endif
        default:
            base = 0x3000;
    }

#if 1
    {
    int reg;
//    static int hmax=0;
    static int vmax=0;
    // VMAX
    //if(vmax == 0)
    {
    vmax=0;
    reg = vc_mipi_common_reg_read(client, base+0x12); // HIGH
    if(reg) vmax = reg & 0xff;
    reg = vc_mipi_common_reg_read(client, base+0x11); // MIDDLE
    if(reg) vmax = (vmax << 8) | (reg & 0xff);
    reg = vc_mipi_common_reg_read(client, base+0x10); // LOW
    if(reg) vmax = (vmax << 8) | (reg & 0xff);

#if TRACE_IMX_EXPOSURE_296_297
    dev_info(&client->dev, "vmax = %08x \n",vmax);
    dev_info(&client->dev, "vmax = %d \n",vmax);
#endif
    }
    vMax = vmax; // TEST

#if 0
    // HMAX
    //if (hmax == 0)
    {
    reg = vc_mipi_common_reg_read(client, 0x7003); // HIGH
    if(reg) hmax = (hmax << 8) | reg;
    reg = vc_mipi_common_reg_read(client, 0x7002); // LOW
    if(reg) hmax = (hmax << 8) | reg;
    dev_info(&client->dev, "hmax = %08x \n",hmax);
    dev_info(&client->dev, "hmax = %d \n",hmax);
    }
#endif

    }
#endif

//    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    if (priv->exposure_time < expMin0)
        priv->exposure_time = expMin0;

    if (priv->exposure_time > expMax)
        priv->exposure_time = expMax;

    //if (priv->exposure_time < expMin1*multiplier/2)
    if (priv->exposure_time < expMin1)
    {
        // exposure = (NumberOfLines - exp_time / 1Hperiod + toffset / 1Hperiod )
        ////exposure = (nrLines  -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        exposure = (vMax  -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        exposure = exposure - ( vMax - ( (vMax * multiplier) / 1000 )); // set frame rate
        if(exposure < 14)
            exposure = 14;


#if TRACE_IMX_EXPOSURE_296_297
        dev_info(&client->dev, "SHS = %d vMax= %d\n",exposure,vMax);
#endif

#if 1
#if 1
        //vMax = (vMax * multiplier) / 1000; // set frame rate
        ret  = vc_mipi_common_reg_write(client, base+0x12, (vMax >> 16) & 0x07);
#else
        ret  = vc_mipi_common_reg_write(client, base+0x12, 0x00);
#endif
        ret |= vc_mipi_common_reg_write(client, base+0x11, (vMax >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, base+0x10,  vMax        & 0xff);
#endif
        if(sen_reg(priv, EXPOSURE_HIGH))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_HIGH)  , (exposure >> 16) & 0x07);
        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), (exposure >>  8) & 0xff);
        if(sen_reg(priv, EXPOSURE_LOW))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW)   ,  exposure        & 0xff);
    }
    else
    {
        //exposure = 5 + ((u64)(priv->exposure_time) * 16384 - tOffset)/h1Period;
        u64 divresult;
        u32 divisor ,remainder;
        divresult = ((unsigned long long)priv->exposure_time * 16384) - tOffset;
        divisor   = h1Period;
        remainder = (u32)(do_div(divresult,divisor)); // caution: division result value at dividend!
#if 1
        //exposure = 5 + ((u32)divresult * multiplier) / 1000; // set frame rate
        exposure = 15 + ((u32)divresult * multiplier) / 1000; // set frame rate
#else
        //exposure = 5 + (u32)divresult;
        exposure = 15 + (u32)divresult;
#endif
#if TRACE_IMX_EXPOSURE_296_297
        dev_info(&client->dev, "VMAX = %d \n",exposure);
#endif

        if(sen_reg(priv, EXPOSURE_HIGH))
            ret  = vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_HIGH), 0x00);
        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), 0x00);
        if(sen_reg(priv, EXPOSURE_LOW))
            //ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW), 0x04);
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW), 0x0e);

        ret |= vc_mipi_common_reg_write(client, base+0x12, (exposure >> 16) & 0x07);
        ret |= vc_mipi_common_reg_write(client, base+0x11, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, base+0x10,  exposure        & 0xff);

    }

    return ret;
}
#define imx_exposure_296(priv)    imx_exposure_296_297(priv, EXPOSURE_TIME_MIN_296, EXPOSURE_TIME_MIN2_296, EXPOSURE_TIME_MAX_296, NRLINES_296 ,TOFFSET_296, H1PERIOD_296, VMAX_296)
#define imx_exposure_297(priv)    imx_exposure_296_297(priv, EXPOSURE_TIME_MIN_297, EXPOSURE_TIME_MIN2_297, EXPOSURE_TIME_MAX_297, NRLINES_297 ,TOFFSET_297, H1PERIOD_297, VMAX_297)

/****** imx_exposure_290_327 = IMX290/IMX327 exposure = 01.2021 *********/
static int imx_exposure_290_327(struct imx *priv)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret;
    u32 exposure = 0;
    unsigned int lf = priv->sensor_mode ? 2 : 1;

    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    if (priv->exposure_time < 29)
        priv->exposure_time = 29;

    if (priv->exposure_time > 7767184)
        priv->exposure_time = 7767184;

    if (priv->exposure_time < 38716)
    {
        // range 1..1123
        exposure = (1124 * 20000 -  (u32)(priv->exposure_time) * 29 * 20 * lf) / 20000;
        dev_info(&client->dev, "SHS = %d \n",exposure);

        ret  = vc_mipi_common_reg_write(client, 0x301A, 0x00);
        ret |= vc_mipi_common_reg_write(client, 0x3019, 0x04);
        ret |= vc_mipi_common_reg_write(client, 0x3018, 0x65);

        ret |= vc_mipi_common_reg_write(client, 0x3022, (exposure >> 16) & 0x01);
        ret |= vc_mipi_common_reg_write(client, 0x3021, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x3020,  exposure        & 0xff);
    }
    else // range 1123..
    {
        exposure = ( 1 * 20000 + (u32)(priv->exposure_time) * 29 * 20 * lf ) / 20000;

        dev_info(&client->dev, "VMAX = %d \n",exposure);

        ret  = vc_mipi_common_reg_write(client, 0x3022, 0x00);
        ret |= vc_mipi_common_reg_write(client, 0x3021, 0x00);
        ret |= vc_mipi_common_reg_write(client, 0x3020, 0x01);

        ret |= vc_mipi_common_reg_write(client, 0x301A, (exposure >> 16) & 0x01);
        ret |= vc_mipi_common_reg_write(client, 0x3019, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x3018,  exposure        & 0xff);

    }

    return ret;
}
#define imx_exposure_290(priv)    imx_exposure_290_327(priv)
#define imx_exposure_327(priv)    imx_exposure_290_327(priv)

/****** imx_exposure_412 = IMX412 exposure = 01.2021 ********************/
static int imx_exposure_412(struct imx *priv)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret=0;
    u32 exposure = 0;

    // alpha=1932/5792
    // N={1,5792}
    // tline=16129.2119 ns -> 16us
    // exposure_time=(N+alpha)*tline
    // N=(exposure_time/tline)-alpha
    // EXP_MIN=(1+alpha)*tline=21509.34885 ns -> 22us
    // EXP_MAX=(5792+alpha)*tline=93425775.44 ns -> 93426us
    //

    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    if (priv->exposure_time < 22)
        priv->exposure_time = 22;
    if (priv->exposure_time > 92693)
        priv->exposure_time = 92693;

    if (priv->exposure_time < 92694)
    {
        exposure = ( ((u32)(priv->exposure_time) * 20000 / 16) - (1932 * 20000 / 5792) ) / 20000;
        dev_info(&client->dev, "EXP = %d \n",exposure);

        ret |= vc_mipi_common_reg_write(client, 0x0202, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x0203,  exposure        & 0xff);
    }

    return ret;
}

/****** imx_exposure_415 = IMX415 exposure = 01.2021 ********************/
static int imx_exposure_415(struct imx *priv)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret=0;
    u32 exposure = 0;
    unsigned int lf = priv->sensor_mode ? 8 : 14;

    // offset=1,79 us -> 0us
    // SRH0={8,2192-4}
    // VMAX=2250
    // 1H= 7.63885 us ->  8us (4 lanes)
    // 1H=14.44444 us -> 14us (2 lanes)
    // exposure_time= (2192 - SRH0)*1H + offset
    // SRH0 = 2250 - (exposure_time - offset)/1H
    // EXP_MIN=(2250-2246)*7.63885us+1.79us=32.3454us -> 32 us -> (4*8=32))
    // EXP_MAX=(2250-8)*7.63885us+1.79us=17173,9248us -> 17174 us -> ((2250-8)*8=17936)


    if (priv->exposure_time < 4*lf)
        priv->exposure_time = 4*lf;
    if (priv->exposure_time > 2097152)
        priv->exposure_time = 2097152;

    if (priv->exposure_time <= (2250-8)*4*lf)
    {
        exposure = ( 2250 - ( (u32)(priv->exposure_time) * 20000) / (4 * lf * 20000) );
        dev_info(&client->dev, "EXP = %d \n",exposure);

        ret |= vc_mipi_common_reg_write(client, 0x3052, (exposure >> 16) & 0x07);
        ret |= vc_mipi_common_reg_write(client, 0x3051, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x3050,  exposure        & 0xff);

        ret |= vc_mipi_common_reg_write(client, 0x3026, 0x00  );
        ret |= vc_mipi_common_reg_write(client, 0x3025, 0x08  );
        ret |= vc_mipi_common_reg_write(client, 0x3024, 0xca );
    }
    else
    {
        exposure = 9 + ( ( (u32)(priv->exposure_time) / (lf*4) ) );
        dev_info(&client->dev, "VMAX = %d \n",exposure);

        ret |= vc_mipi_common_reg_write(client, 0x3052, 0x00);
        ret |= vc_mipi_common_reg_write(client, 0x3051, 0x00);
        ret |= vc_mipi_common_reg_write(client, 0x3050, 0x08);

        ret |= vc_mipi_common_reg_write(client, 0x3026, (exposure >> 16) & 0x07);
        ret |= vc_mipi_common_reg_write(client, 0x3025, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x3024,  exposure        & 0xff);
    }

    return ret;
}

/****** imx_exposure_9281 = OV9281 exposure = 01.2021 *******************/
static int imx_exposure_9281(struct imx *priv)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret=0;
    u32 exposure = 0;

    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    //
    // 9100 ns <==> 16
    //
#if 0
    exposure = (priv->exposure_time / 9100) * 16000; // value in ns - 4 bit shift

    if (exposure < OV9281_DIGITAL_EXPOSURE_MIN)
        exposure = OV9281_DIGITAL_EXPOSURE_MIN;
    if (exposure > OV9281_DIGITAL_EXPOSURE_MAX)
        exposure = OV9281_DIGITAL_EXPOSURE_MAX;

    priv->exposure_time = (exposure / 16000) * 9100;
#endif

    exposure = (((u32)(priv->exposure_time * 1000) / 9100)) << 4; // calculate in ns - 4 bit shift

    dev_info(&client->dev, "EXPOSURE = %d \n",exposure);

    ret  = vc_mipi_common_reg_write(client, 0x3500, (exposure >> 16) & 0x0f);
    ret |= vc_mipi_common_reg_write(client, 0x3501, (exposure >>  8) & 0xff);
    ret |= vc_mipi_common_reg_write(client, 0x3502,  exposure        & 0xff);

    exposure = (((u32)(priv->exposure_time * 1000) / 9100)) ; // calculate in ns

    /* flash duration for flashout signal*/
    ret |= vc_mipi_common_reg_write(client, 0x3926, (exposure >> 16) & 0x0f);
    ret |= vc_mipi_common_reg_write(client, 0x3927, (exposure >>  8) & 0xff);
    ret |= vc_mipi_common_reg_write(client, 0x3928,  exposure        & 0xff);

    /* flash offset for flashout signal , not necessary for >= Rev3 of sensor module */
    ret |= vc_mipi_common_reg_write(client, 0x3922, 0);
    ret |= vc_mipi_common_reg_write(client, 0x3923, 0);
    ret |= vc_mipi_common_reg_write(client, 0x3924, 4);

    return ret;
}

//
// IMX183
// 1H period 20.00us
// NumberOfLines=3694
//
#define H1PERIOD_183 327680 // (U32)(20.000 * 16384.0)
#define NRLINES_183  (3694)
#define TOFFSET_183  47563  // (U32)(2.903 * 16384.0)
#define VMAX_183     3728
#define EXPOSURE_TIME_MIN_183  160
#define EXPOSURE_TIME_MIN2_183 74480
#define EXPOSURE_TIME_MAX_183  10000000

#define imx_exposure_183(priv) imx_exposure_imx183(priv, EXPOSURE_TIME_MIN_183, EXPOSURE_TIME_MIN2_183, EXPOSURE_TIME_MAX_183, NRLINES_183 ,TOFFSET_183, H1PERIOD_183, VMAX_183)

//
// IMX226
// 1H period 20.00us
// NumberOfLines=3694
//
#define H1PERIOD_226 327680 // (U32)(20.000 * 16384.0)
#define NRLINES_226  (3694)
#define TOFFSET_226  47563  // (U32)(2.903 * 16384.0)
#define VMAX_226     3728
#define EXPOSURE_TIME_MIN_226  160
#define EXPOSURE_TIME_MIN2_226 74480
#define EXPOSURE_TIME_MAX_226  10000000

#define imx_exposure_226(priv) imx_exposure_imx183(priv, EXPOSURE_TIME_MIN_226, EXPOSURE_TIME_MIN2_226, EXPOSURE_TIME_MAX_226, NRLINES_226 ,TOFFSET_226, H1PERIOD_226, VMAX_226)

/****** imx_exposure_imx183 = IMX183 exposure = 12.2020 *****************/
static int imx_exposure_imx183(struct imx *priv, s32 expMin0, s32 expMin1, s32 expMax, s32 nrLines, s32 tOffset, s32 h1Period, s32 vMax)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret=0;
    u32 exposure = 0;

//    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    if (priv->exposure_time < expMin0)
        priv->exposure_time = expMin0;

    if (priv->exposure_time > expMax)
        priv->exposure_time = expMax;
#if 1
    {
    int reg;
    static int hmax=0;
    static int vmax=0;
    // VMAX
    if(vmax == 0)
    {
    reg = vc_mipi_common_reg_read(client, 0x7006); // HIGH
    if(reg) vmax = reg;
    reg = vc_mipi_common_reg_read(client, 0x7005); // MIDDLE
    if(reg) vmax = (vmax << 8) | reg;
    reg = vc_mipi_common_reg_read(client, 0x7004); // LOW
    if(reg) vmax = (vmax << 8) | reg;
    dev_info(&client->dev, "vmax = %08x \n",vmax);
    dev_info(&client->dev, "vmax = %d \n",vmax);
    }
    vMax = vmax; // TEST

    // HMAX
    if (hmax == 0)
    {
    reg = vc_mipi_common_reg_read(client, 0x7003); // HIGH
    if(reg) hmax = (hmax << 8) | reg;
    reg = vc_mipi_common_reg_read(client, 0x7002); // LOW
    if(reg) hmax = (hmax << 8) | reg;
    dev_info(&client->dev, "hmax = %08x \n",hmax);
    dev_info(&client->dev, "hmax = %d \n",hmax);
    }

    }
#endif

    if (priv->exposure_time < expMin1)
    {
        // exposure = (NumberOfLines - exp_time / 1Hperiod + toffset / 1Hperiod )
        // shutter = {VMAX - SHR}*HMAX + 209(157) clocks

        //exposure = (nrLines  -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        exposure = (vMax -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        dev_info(&client->dev, "SHS = %d \n",exposure);

        ret  = vc_mipi_common_reg_write(client, 0x7006, (vMax >> 16) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x7005, (vMax >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x7004,  vMax        & 0xff);

        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), (exposure >>  8) & 0xff);
        if(sen_reg(priv, EXPOSURE_LOW))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW)   ,  exposure        & 0xff);
    }
#if 1
    else
    {
        // exposure = 5 + ((unsigned long long)(priv->exposure_time * 16384) - tOffset)/h1Period;
        u64 divresult;
        u32 divisor ,remainder;
        divresult = ((unsigned long long)priv->exposure_time * 16384) - tOffset;
        divisor   = h1Period;
        remainder = (u32)(do_div(divresult,divisor)); // caution: division result value at dividend!
        exposure = 5 + (u32)divresult;

        dev_info(&client->dev, "VMAX = %d \n",exposure);

        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret  = vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), 0x00);
        if(sen_reg(priv, EXPOSURE_LOW))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW), 0x04);

        ret |= vc_mipi_common_reg_write(client, 0x7006, (exposure >> 16) & 0x07);
        ret |= vc_mipi_common_reg_write(client, 0x7005, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x7004,  exposure        & 0xff);

    }
#endif
    return ret;
}

//
// IMX252
// 1H period 20.00us
// NumberOfLines=3694
//
#define H1PERIOD_252 327680 // (U32)(20.000 * 16384.0)
#define NRLINES_252  (3694)
#define TOFFSET_252  47563  // (U32)(2.903 * 16384.0)
#define VMAX_252     3079
#define EXPOSURE_TIME_MIN_252  160
#define EXPOSURE_TIME_MIN2_252 74480
#define EXPOSURE_TIME_MAX_252  10000000

#define imx_exposure_252(priv) imx_exposure_imx252(priv, EXPOSURE_TIME_MIN_252, EXPOSURE_TIME_MIN2_252, EXPOSURE_TIME_MAX_252, NRLINES_252 ,TOFFSET_252, H1PERIOD_252, VMAX_252)

/****** imx_exposure_imx252 = IMX252 exposure = 12.2020 *****************/
static int imx_exposure_imx252(struct imx *priv, s32 expMin0, s32 expMin1, s32 expMax, s32 nrLines, s32 tOffset, s32 h1Period, s32 vMax)
{
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct i2c_client *client = priv->i2c_client;
    int ret=0;
    u32 exposure = 0;

    // do nothing in ext_trig mode
//    if (priv->sensor_ext_trig) return 0;

    if (priv->exposure_time < expMin0)
        priv->exposure_time = expMin0;

    if (priv->exposure_time > expMax)
        priv->exposure_time = expMax;
#if 1
    {
    int reg;
    static int hmax=0;
    static int vmax=0;
    // VMAX
    if(vmax == 0)
    {
    reg = vc_mipi_common_reg_read(client, 0x0212); // HIGH
    if(reg) vmax = reg;
    reg = vc_mipi_common_reg_read(client, 0x0211); // MIDDLE
    if(reg) vmax = (vmax << 8) | reg;
    reg = vc_mipi_common_reg_read(client, 0x0210); // LOW
    if(reg) vmax = (vmax << 8) | reg;
    dev_info(&client->dev, "vmax = %08x \n",vmax);
    dev_info(&client->dev, "vmax = %d \n",vmax);
    }
    vMax = vmax; // TEST

    // HMAX
    if (hmax == 0)
    {
    reg = vc_mipi_common_reg_read(client, 0x0215); // HIGH
    if(reg) hmax = (hmax << 8) | reg;
    reg = vc_mipi_common_reg_read(client, 0x0214); // LOW
    if(reg) hmax = (hmax << 8) | reg;
    dev_info(&client->dev, "hmax = %08x \n",hmax);
    dev_info(&client->dev, "hmax = %d \n",hmax);
    }

    }
#endif

    if (priv->exposure_time < expMin1)
    {
        // exposure = (NumberOfLines - exp_time / 1Hperiod + toffset / 1Hperiod )
        // shutter = {VMAX - SHR}*HMAX + 209(157) clocks

        //exposure = (nrLines  -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        exposure = (vMax -  ((u32)(priv->exposure_time) * 16384 - tOffset)/h1Period);
        dev_info(&client->dev, "SHS = %d \n",exposure);

        ret  = vc_mipi_common_reg_write(client, 0x0212, (vMax >> 16) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x0211, (vMax >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x0210,  vMax        & 0xff);

        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), (exposure >>  8) & 0xff);
        if(sen_reg(priv, EXPOSURE_LOW))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW)   ,  exposure        & 0xff);
    }
#if 1
    else
    {
        // exposure = 5 + ((unsigned long long)(priv->exposure_time * 16384) - tOffset)/h1Period;
        u64 divresult;
        u32 divisor ,remainder;
        divresult = ((unsigned long long)priv->exposure_time * 16384) - tOffset;
        divisor   = h1Period;
        remainder = (u32)(do_div(divresult,divisor)); // caution: division result value at dividend!
        exposure = 5 + (u32)divresult;

        dev_info(&client->dev, "VMAX = %d \n",exposure);

        if(sen_reg(priv, EXPOSURE_MIDDLE))
            ret  = vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), 0x00);
        if(sen_reg(priv, EXPOSURE_LOW))
            ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW), 0x04);

        ret |= vc_mipi_common_reg_write(client, 0x0212, (exposure >> 16) & 0x07);
        ret |= vc_mipi_common_reg_write(client, 0x0211, (exposure >>  8) & 0xff);
        ret |= vc_mipi_common_reg_write(client, 0x0210,  exposure        & 0xff);

    }
#endif
    return ret;
}

/****** imx_set_exposure = Set IMX exposure = 01.2021 *******************/
static int imx_set_exposure(struct i2c_client *client)
{

#define TRACE_IMX_SET_EXPOSURE  1   /* DDD - imx_set_exposure() - trace */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;

    int ret = 0;


    if(priv->exposure_time < priv->sen_pars.exposure_min) priv->exposure_time = priv->sen_pars.exposure_min;
    if(priv->exposure_time > priv->sen_pars.exposure_max) priv->exposure_time = priv->sen_pars.exposure_max;

/*----------------------------------------------------------------------*/
/*                   Set exposure - ext. trigger mode                   */
/*----------------------------------------------------------------------*/
    if(priv->sensor_ext_trig)
    {
        u64 exposure = (priv->exposure_time * (priv->sen_clk/1000000)); // sen_clk default=54Mhz, imx183=72Mhz
        int addr;
        int data;
        int ret = 0;

#if TRACE_IMX_SET_EXPOSURE
        dev_err(dev, "%s(): Set exposure=%d: TRIG exposure=%llu (0x%llx)\n", __func__, priv->exposure_time, exposure, exposure);
#endif


/*
register 9  [0x0109]: exposure LSB (R/W, default: 0x10)
register 10 [0x010A]: exposure     (R/W, default: 0x27)
register 11 [0x010B]: exposure     (R/W, default: 0x00)
register 12 [0x010C]: exposure MSB (R/W, default: 0x00)

This is the 32bit register (4 x 8bit) for the exposure control when
using the fast trigger mode.
The exposure counter is using the internal 74.25MHz clock, i.e.
the exact exposure value is calculated as follows:

   exposure_time [nsec] = exposure_register[31:0] * 13.4680nsec

Make sure to write registers 9-12 LSB first, MSB last.
Writing the MSB will transfer the complete exposure value into
a buffer register which will update the internal exposure
counter as soon as the current exposure has finished.
*/

//        addr = 0x0108; // ext trig enable
//        // data =      1; // external trigger enable
//        // data =      4; // TEST external self trigger enable
//        data = priv->sensor_ext_trig; // external trigger enable
//        ret = reg_write(priv->rom, addr, data);
//
//        addr = 0x0103; // flash output enable
//        data =      1; // flash output enable
//        ret = reg_write(priv->rom, addr, data);

        addr = 0x0109; // shutter lsb
        data = exposure & 0xff;
        ret |= reg_write(priv->rom, addr, data);

        addr = 0x010a;
        data = (exposure >> 8) & 0xff;
        ret |= reg_write(priv->rom, addr, data);

        addr = 0x010b;
        data = (exposure >> 16) & 0xff;
        ret |= reg_write(priv->rom, addr, data);

        addr = 0x010c; // shutter msb
        data = (exposure >> 24) & 0xff;
        ret |= reg_write(priv->rom, addr, data);

    } /* if(priv->sensor_ext_trig) */

/*----------------------------------------------------------------------*/
/*                       Set exposure - free-run mode                   */
/*----------------------------------------------------------------------*/
    else
    {

#if TRACE_IMX_SET_EXPOSURE
      dev_err(dev, "%s: Set exposure = %d\n", __func__, priv->exposure_time);
#endif

      switch(priv->model)
      {
        case OV7251_MODEL_MONOCHROME:
        case OV7251_MODEL_COLOR:
        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            ret = imx_exposure_9281(priv);
            break;

        case IMX183_MODEL_MONOCHROME:
        case IMX183_MODEL_COLOR:
            ret = imx_exposure_183(priv);
            break;

        case IMX226_MODEL_MONOCHROME:
        case IMX226_MODEL_COLOR:
            ret = imx_exposure_226(priv);
            if(ret)
            {
                dev_err(dev, "%s(): imx_exposure_226() err=%d\n", __func__, ret);
                return ret;
            }
            break;

        case IMX250_MODEL_MONOCHROME:
        case IMX250_MODEL_COLOR:
        case IMX252_MODEL_MONOCHROME:
        case IMX252_MODEL_COLOR:
            ret = imx_exposure_252(priv);
            break;

        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
        case IMX296_MODEL_MONOCHROME:
        case IMX296_MODEL_COLOR:
            ret = imx_exposure_296(priv);
            if(ret)
            {
                dev_err(dev, "%s(): imx_exposure_296() err=%d\n", __func__, ret);
                return ret;
            }
            break;

        case IMX297_MODEL_MONOCHROME:
        case IMX297_MODEL_COLOR:
            ret = imx_exposure_297(priv);
            if(ret)
            {
                dev_err(dev, "%s(): imx_exposure_297() err=%d\n", __func__, ret);
                return ret;
            }
            break;

        case IMX290_MODEL_MONOCHROME:
        case IMX290_MODEL_COLOR:
        case IMX327_MODEL_MONOCHROME:
        case IMX327_MODEL_COLOR:
            ret = imx_exposure_290_327(priv);
            break;

        case IMX412_MODEL_MONOCHROME:
        case IMX412_MODEL_COLOR:
            ret = imx_exposure_412(priv);
            break;

        case IMX415_MODEL_MONOCHROME:
        case IMX415_MODEL_COLOR:
            ret = imx_exposure_415(priv);
            break;

         default:
            ret = -EINVAL;
            break;
      }

    } /* else: free-run mode */

//#if IMX_ENB_MUTEX
//    mutex_unlock(&priv->mutex);
//#endif

    if(ret)
      dev_err(dev, "%s: error=%d\n", __func__, ret);
    return ret;
}

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD1  /* [[[ */

#include "user_ctrl_method1.h"

#endif  /* ]]] - user ctrl method 1 */


#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2  /* [[[ */

/****** imx_set_ctrl = Set control = 01.2021 ****************************/
static int imx_set_ctrl(struct v4l2_ctrl *ctrl)
{

#define TRACE_IMX_SET_CTRL  0   /* DDD - imx_set_ctrl - trace */

    struct imx *priv = container_of(ctrl->handler, struct imx, ctrl_handler);
    struct i2c_client *client = priv->i2c_client;
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    int ret = 0;


#if TRACE_IMX_SET_CTRL
    dev_err(&client->dev, "%s: cid=0x%x\n", __func__, ctrl->id);
#endif

    /*
     * Applying V4L2 control value only happens
     * when power is up for streaming
     */
//    if (pm_runtime_get_if_in_use(&client->dev) == 0)
//        return 0;

    switch (ctrl->id)
    {

      case V4L2_CID_GAIN:
#if TRACE_IMX_SET_CTRL
        dev_err(&client->dev, "%s: Set gain=%d\n", __func__, ctrl->val);
#endif
        priv->digital_gain = ctrl->val;
        ret = imx_set_gain(client);
        break;

      case V4L2_CID_EXPOSURE:
#if TRACE_IMX_SET_CTRL
        dev_err(&client->dev, "%s: Set exposure=%d\n", __func__, ctrl->val);
#endif
        priv->exposure_time = ctrl->val;
        ret = imx_set_exposure(client);
        break;

      default:
        dev_err(&client->dev, "%s: ctrl(id:0x%x,val:0x%x) is not handled\n", __func__, ctrl->id, ctrl->val);
        ret = -EINVAL;
        break;
    }

//    pm_runtime_put(&client->dev);

    return ret;
}

static const struct v4l2_ctrl_ops imx_ctrl_ops = {
    .s_ctrl = imx_set_ctrl,
};

//{
///* Do not change the name field for the controls! */
//  {
//      .ops = &imx_ctrl_ops,
//      .id = V4L2_CID_GAIN,
//      .name = "Gain",
//      .type = V4L2_CTRL_TYPE_INTEGER,
//      .flags = V4L2_CTRL_FLAG_SLIDER,
//      .min = 0,
//      .max = 0xfff,
//      .def = 10,
//      .step = 1,
//  },
//  {
//      .ops = &imx_ctrl_ops,
//      .id = V4L2_CID_EXPOSURE,
//      .name = "Exposure",
//      .type = V4L2_CTRL_TYPE_INTEGER,
//      .flags = V4L2_CTRL_FLAG_SLIDER,
//      .min = 0,
//      .max = 0xfffff,
//      .def = 1000,
//      .step = 1,
//  },
//};

/* Initialize control handlers */
/****** imx_init_controls = Init user controls = 01.2021 ****************/
static int imx_init_controls(struct imx *priv)
{

#define TRACE_IMX_INIT_CONTROLS     0   /* DDD - imx_init_controls - trace */

    struct i2c_client *client = priv->i2c_client;
//    struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
    struct v4l2_ctrl_handler *ctrl_hdlr;
    struct v4l2_ctrl *ctrl;
    int num_ctrls = ARRAY_SIZE(ctrl_config_list);
    int ret;
    int i;

    ctrl_hdlr = &priv->ctrl_handler;
    ret = v4l2_ctrl_handler_init(ctrl_hdlr, num_ctrls);
    if (ret)
        return ret;

//    mutex_init(&priv->mutex);
//    ctrl_hdlr->lock = &priv->mutex;

#if TRACE_IMX_INIT_CONTROLS
    dev_err(&client->dev, "%s: ...\n", __func__);
#endif

    for (i = 0; i < num_ctrls; i++)
    {
        ctrl_config_list[i].ops = &imx_ctrl_ops;
        ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &ctrl_config_list[i], NULL);
        if (ctrl == NULL)
        {
            dev_err(&client->dev, "%s: Failed to init %s ctrl\n",  __func__, ctrl_config_list[i].name);
            continue;
        }

//        if (ctrl_config_list[i].type == V4L2_CTRL_TYPE_STRING &&
//            ctrl_config_list[i].flags & V4L2_CTRL_FLAG_READ_ONLY) {
//            ctrl->p_new.p_char = devm_kzalloc(&client->dev,
//                ctrl_config_list[i].max + 1, GFP_KERNEL);
//            if (!ctrl->p_new.p_char)
//                return -ENOMEM;
//        }
//        priv->ctrls[i] = ctrl;
    }


    if (ctrl_hdlr->error) {
        ret = ctrl_hdlr->error;
        dev_err(&client->dev, "%s: control init failed (%d)\n", __func__, ret);
        goto error;
    }

    priv->subdev.ctrl_handler = ctrl_hdlr;

    return 0;

error:
    v4l2_ctrl_handler_free(ctrl_hdlr);
//    mutex_destroy(&priv->mutex);

    return ret;
}

/****** imx_free_controls = Free user controls = 01.2021 ****************/
static void imx_free_controls(struct imx *priv)
{
    v4l2_ctrl_handler_free(priv->subdev.ctrl_handler);
//    mutex_destroy(&priv->mutex);
}

#endif  /* ]]] - user ctrl method 2 */

///****** imx_g_parm = Get param = 11.2020 ********************************/
///*!
// * imx_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
// * @s: pointer to standard V4L2 sub device structure
// * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
// *
// * Returns the sensor's video CAPTURE parameters.
// */
//static int imx_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
//{
//
//#define TRACE_IMX_G_PARM     0   /* DDD - imx_g_parm - trace */
//
//    struct i2c_client *client = v4l2_get_subdevdata(sd);
//    struct imx *sensor = to_imx(client);
//    struct device *dev = &sensor->i2c_client->dev;
//    struct v4l2_captureparm *cparm = &a->parm.capture;
//    int ret = 0;
//
//#if TRACE_IMX_G_PARM
//    dev_err(dev, "%s: v4l2_streamparm type = %d\n", __func__, a->type);
//#endif
//
//    switch (a->type) {
//    /* This is the only case currently handled. */
//    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
//
//#if TRACE_IMX_G_PARM
//    dev_err(dev, "%s: type = V4L2_BUF_TYPE_VIDEO_CAPTURE\n", __func__);
//#endif
//
//        memset(a, 0, sizeof(*a));
//        a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        cparm->capability = sensor->streamcap.capability;
//        cparm->timeperframe = sensor->streamcap.timeperframe;
//        cparm->capturemode = sensor->streamcap.capturemode;
//        ret = 0;
//        break;
//
//    /* These are all the possible cases. */
//    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
//    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
//    case V4L2_BUF_TYPE_VBI_CAPTURE:
//    case V4L2_BUF_TYPE_VBI_OUTPUT:
//    case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
//    case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
//        dev_err(dev, "%s: Type not supported: %d\n", __func__, a->type);
//        ret = -EINVAL;
//        break;
//
//    default:
//        dev_err(dev, "%s: Type is unknown - %d\n", __func__, a->type);
//        ret = -EINVAL;
//        break;
//    }
//
//    return ret;
//}
//
///****** imx_s_parm = Set param  = 11.2020 *******************************/
///*!
// * ov5460_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
// * @s: pointer to standard V4L2 sub device structure
// * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
// *
// * Configures the sensor to use the input parameters, if possible.  If
// * not possible, reverts to the old parameters and returns the
// * appropriate error code.
// */
//static int imx_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
//{
//
//#define TRACE_IMX_S_PARM     0   /* DDD - imx_s_parm - trace */
//
//    struct i2c_client *client = v4l2_get_subdevdata(sd);
//    struct imx *sensor = to_imx(client);
//    struct device *dev = &sensor->i2c_client->dev;
//    int ret = 0;
//
//#if TRACE_IMX_S_PARM
//    dev_err(dev, "%s: v4l2_streamparm type = %d\n", __func__, a->type);
//#endif
//
//
//    switch (a->type) {
//
//    /* This is the only case currently handled. */
//    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
//
//        break;
//
//    /* These are all the possible cases. */
//    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
//    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
//    case V4L2_BUF_TYPE_VBI_CAPTURE:
//    case V4L2_BUF_TYPE_VBI_OUTPUT:
//    case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
//    case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
//        dev_err(dev, "%s: Type is not V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n", __func__, a->type);
//        ret = -EINVAL;
//        break;
//
//    default:
//        dev_err(dev, "%s: Type is unknown - %d\n", __func__, a->type);
//        ret = -EINVAL;
//        break;
//    }
//
//    return ret;
//}

/****** imx_set_fmt = Set format = 11.2020 ******************************/
static int imx_set_fmt(struct v4l2_subdev *sd,
            struct v4l2_subdev_pad_config *cfg,
            struct v4l2_subdev_format *format)
{

#define TRACE_IMX_SET_FMT   0   /* DDD - imx_set_fmt - trace */

    struct v4l2_mbus_framefmt *mf = &format->format;
    const struct imx_datafmt *fmt = imx_find_datafmt(mf->code);
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct imx *priv = to_imx(client);
    int capturemode;

#if TRACE_IMX_SET_FMT
//    dev_err(&client->dev, "%s: Set format\n", __func__);
//    dev_err(&client->dev, "%s: Set format, fmt=0x%x\n", __func__, (int)fmt);
#endif

    if (!fmt) {
        mf->code    = imx_data_fmts[0].code;
        mf->colorspace  = imx_data_fmts[0].colorspace;
        fmt     = &imx_data_fmts[0];
    }

    mf->field   = V4L2_FIELD_NONE;

    if (format->which == V4L2_SUBDEV_FORMAT_TRY)
    {
#if TRACE_IMX_SET_FMT
        dev_err(&client->dev, "%s: format->which == V4L2_SUBDEV_FORMAT_TRY: exit\n", __func__);
#endif
        return 0;
    }

    priv->fmt = fmt;

#if TRACE_IMX_SET_FMT
    dev_err(&client->dev, "%s: width,height=%d,%d\n", __func__, mf->width, mf->height);
#endif

    capturemode = get_capturemode(mf->width, mf->height);
    if (capturemode >= 0) {
        priv->streamcap.capturemode = capturemode;
        priv->pix.width = mf->width;
        priv->pix.height = mf->height;

#if TRACE_IMX_SET_FMT
{
        int pix_fmt = priv->pix.pixelformat;
//        dev_err(&client->dev, "%s: get_capturemode() OK: capturemode=%d, width,height=%d,%d\n", __func__,
//                       capturemode, priv->pix.width, priv->pix.height);
        dev_err(&client->dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pixelformat=0x%08x '%c%c%c%c' code=%d, colorspace=%d\n", __func__,
                        priv->pix.width, priv->pix.height,
                        priv->pix.bytesperline, priv->pix.sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF),
                        fmt->code, fmt->colorspace);
}
#endif
        return 0;
    }

    dev_err(&client->dev, "%s: width,height=%d,%d\n", __func__, mf->width, mf->height);
    dev_err(&client->dev, "%s: get_capturemode() failed: capturemode=%d\n", __func__, capturemode);
    dev_err(&client->dev, "%s: Set format failed code=%d, colorspace=%d\n", __func__,
        fmt->code, fmt->colorspace);
    return -EINVAL;
}

/****** imx_get_fmt = Get format = 11.2020 ******************************/
static int imx_get_fmt(struct v4l2_subdev *sd,
              struct v4l2_subdev_pad_config *cfg,
              struct v4l2_subdev_format *format)
{

#define TRACE_IMX_GET_FMT    0   /* DDD - imx_get_fmt - trace */

    struct v4l2_mbus_framefmt *mf = &format->format;
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct imx *priv = to_imx(client);
    const struct imx_datafmt *fmt = priv->fmt;

#if TRACE_IMX_GET_FMT
    dev_err(&client->dev, "%s: Get format\n", __func__);
#endif

    if (format->pad)
    {
#if TRACE_IMX_GET_FMT
        dev_err(&client->dev, "%s: format->pad: error\n", __func__);
#endif
        return -EINVAL;
    }

    mf->code    = fmt->code;
    mf->colorspace  = fmt->colorspace;
    mf->field   = V4L2_FIELD_NONE;

    mf->width   = priv->pix.width;
    mf->height  = priv->pix.height;

#if TRACE_IMX_GET_FMT
    dev_err(&client->dev, "%s: width,height=%d,%d\n", __func__, priv->pix.width, priv->pix.height);
#endif

    return 0;
}

/****** imx_enum_mbus_code = = 11.2020 **********************************/
static int imx_enum_mbus_code(struct v4l2_subdev *sd,
                 struct v4l2_subdev_pad_config *cfg,
                 struct v4l2_subdev_mbus_code_enum *code)
{
//    if (code->pad || code->index >= ARRAY_SIZE(imx226_colour_fmts))
    if (code->pad || code->index >= imx_data_fmts_size)
        return -EINVAL;

    code->code = imx_data_fmts[code->index].code;

//   struct i2c_client *client = v4l2_get_subdevdata(sd);
//   struct imx *priv = to_imx(client);
//   const struct imx226_mode *mode = priv->cur_mode;
//
//   if (code->index !=0)
//       return -EINVAL;
//
//   if(mode->sensor_depth==8)
//       code->code = MEDIA_BUS_FMT_Y8_1X8;
//
//   if(mode->sensor_depth==10)
//       code->code = MEDIA_BUS_FMT_Y10_1X10;
//endif

    return 0;
}

/****** imx_enum_framesizes = Frame sizes = 11.2020 *********************/
/*!
 * imx_enum_framesizes - V4L2 sensor interface handler for
 *             VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int imx_enum_framesizes(struct v4l2_subdev *sd,
                   struct v4l2_subdev_pad_config *cfg,
                   struct v4l2_subdev_frame_size_enum *fse)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct imx *priv = to_imx(client);

//    if (fse->index > imx226_mode_MAX)
    if (fse->index > 0)
        return -EINVAL;

//    if (fse->code != MEDIA_BUS_FMT_SGRBG10_1X10)
//    if (fse->code != MEDIA_BUS_FMT_SGRBG8_1X8)

    fse->max_width = priv->sen_pars.frame_dx;
    fse->min_width = priv->sen_pars.frame_dx;
    fse->max_height = priv->sen_pars.frame_dy;
    fse->min_height = priv->sen_pars.frame_dy;

    return 0;
}

///****** imx_enum_frameintervals = Frame intervals = 11.2020 *************/
///*!
// * imx_enum_frameintervals - V4L2 sensor interface handler for
// *                 VIDIOC_ENUM_FRAMEINTERVALS ioctl
// * @s: pointer to standard V4L2 device structure
// * @fival: standard V4L2 VIDIOC_ENUM_FRAMEINTERVALS ioctl structure
// *
// * Return 0 if successful, otherwise -EINVAL.
// */
//static int imx_enum_frameintervals(struct v4l2_subdev *sd,
//        struct v4l2_subdev_pad_config *cfg,
//        struct v4l2_subdev_frame_interval_enum *fie)
//{
//
//#define TRACE_IMX_ENUM_FRAMEINTERVALS    0   /* DDD - imx_enum_frameintervals - trace */
//
//    struct i2c_client *client = v4l2_get_subdevdata(sd);
//    struct device *dev = &client->dev;
////    int i, j, count = 0;
//
//#if TRACE_IMX_ENUM_FRAMEINTERVALS
//    dev_err(dev, "%s: index=%d\n", __func__, fie->index);
//#endif
//
//    if (fie->width == 0 || fie->height == 0 ||
//        fie->code == 0) {
//        dev_err(dev, "%s: Please assign pixel format, width and height\n", __func__);
//        return -EINVAL;
//    }
//
//    fie->interval.numerator = 1;
//
//    if(fie->index == 0)
//    {
//        fie->interval.denominator = DEFAULT_FPS;
//        return 0;
//    }
//    else
//    {
////        dev_err(dev, "%s: Invalid index=%d\n", __func__, fie->index);
//        return -EINVAL;
//    }
//
//    return -EINVAL;
//}

/****** dump_fpga_regs = Dump FPGA registers = 01.2021 ******************/
int dump_fpga_regs(struct imx *priv)
{

    struct i2c_client *client = priv->i2c_client;
    struct device *dev = &client->dev;
//    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

#define FPGA_REG_CNT  17
    int reg_buf[FPGA_REG_CNT];

    int err = 0;
    int base = 0x100;
    int addr;
    int i;

/*............. Setup */
    if(!priv->rom)
    {
        dev_err(dev, "%s: Error !!! VC FPGA not found !!!\n", __func__);
        return -EIO;
    }

/*............. Read control registers into reg_buf  */
    i = 0;
    for(addr=base; addr<base+FPGA_REG_CNT; addr++)
    {
      reg_buf[i++] = reg_read(priv->rom, addr);
    }

    dev_err(dev, "%s: FPGA control registers:\n", __func__);

/*............. Dump control registers  */
    dev_err(dev, "0-3   : %02x %02x %02x %02x\n", reg_buf[0],reg_buf[1],reg_buf[2],reg_buf[3]);
    dev_err(dev, "4-8   : %02x %02x %02x %02x %02x\n", reg_buf[4],reg_buf[5],reg_buf[6],reg_buf[7],reg_buf[8]);
    dev_err(dev, "9-12  : %02x %02x %02x %02x\n", reg_buf[9],reg_buf[10],reg_buf[11],reg_buf[12]);
    dev_err(dev, "13-16 : %02x %02x %02x %02x\n", reg_buf[13],reg_buf[14],reg_buf[15],reg_buf[16]);

    return err;
}

/****** ov9281_set_mode = Set mode = 11.2020 ****************************/
static int ov9281_set_mode(struct i2c_client *client)
{

#define TRACE_OV9281_SET_MODE   1   /* DDD - ov9281_set_mode() - trace */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    struct v4l2_pix_format pix;
    int pix_fmt;
    int err = 0;

/*............. Get pixel format */
    mx6s_get_pix_fmt(&pix);
    pix_fmt = pix.pixelformat;

/*............. Set new sensor mode */
// OV9281 sensor modes:
//  0x00 : 10bit free-run streaming
//  0x01 : 8bit  free-run streaming
//  0x02 : 10bit external trigger
//  0x03 : 8bit  external trigger

    if(pix_fmt == V4L2_PIX_FMT_Y10 || pix_fmt == V4L2_PIX_FMT_SRGGB10)
    {
      sensor_mode = 0;      // 10-bit
    }
    else if(pix_fmt == V4L2_PIX_FMT_GREY || pix_fmt == V4L2_PIX_FMT_SRGGB8)
    {
      sensor_mode = 1;      // 8-bit
    }

// Ext. trigger mode
    if(priv->sensor_ext_trig)      // 0=free run, 1=ext. trigger, 2=trigger self test
    {
      sensor_mode += 2;
    }

// Change VC MIPI sensor mode
    if(priv->sensor_mode != sensor_mode)
    {
      priv->sensor_mode = sensor_mode;
#if TRACE_OV9281_SET_MODE
      dev_err(dev, "%s(): New sensor_mode=%d (0=10bit, 1=8bit, 2=10bit trig, 3=8bit trig), sensor_ext_trig=%d\n", __func__,
                    sensor_mode, priv->sensor_ext_trig);
#endif

//      err = vc_mipi_reset(tc_dev, sensor_mode);
      err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
      if(err)
      {
        dev_err(dev, "%s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
        return err;
      }

//// Xavier: switch between 8-bit and 10-bit modes
////      mdelay(300); // wait 300ms : added in vc_mipi_reset()
//      if(err)
//      {
//          dev_err(dev, "%s(): vc_mipi_reset() error=%d\n", __func__, err);
//      }
//
    } /* if(priv->sensor_mode != sensor_mode) */

#if TRACE_OV9281_SET_MODE
{
    dev_err(dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt=0x%08x '%c%c%c%c', sensor_mode=%d\n", __func__,
                        pix.width, pix.height,
                        pix.bytesperline, pix.sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF),
                        sensor_mode );
}
#endif

#if TRACE_OV9281_SET_MODE
//    dev_err(dev, "%s(): mode_prop_idx=%d sensor_mode=%d err=%d\n", __func__, s_data->mode_prop_idx, sensor_mode, err);
//    dev_err(dev, "%s(): sensor_mode=%d (0=10,1=8-bit) err=%d\n", __func__, sensor_mode, err);
#endif

// Write mode table:
    err = reg_write_table(client, priv->sen_pars.sensor_mode_table);
    if(err)
    {
        dev_err(dev, "%s(): reg_write_table() error=%d\n", __func__, err);
//        goto exit;
    }
    else
    {
#if TRACE_OV9281_SET_MODE
//        dev_err(dev, "%s(): reg_write_table() OK, cam_mode=%d\n", __func__, priv->cam_mode);
#endif
    }

    return err;
}

/****** imx183_273_set_mode = Set IMX183-273 mode = 01.2021 *************/
static int imx183_273_set_mode(struct i2c_client *client)
{

#define TRACE_IMX183_273_SET_MODE   1   /* DDD - imx183_273_set_mode() - trace */
//#define IMX183_273_FORCE_SET_MODE   0   /* CCC - imx183_273_set_mode() - force set mode */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    struct v4l2_pix_format pix;
    int pix_fmt;
    int err = 0;

/*............. Get pixel format */
    mx6s_get_pix_fmt(&pix);
    pix_fmt = pix.pixelformat;

/*............. Set new sensor mode */
// IMX183-273 sensor modes:
//   0x00 :  8bit, 2 lanes, streaming          V4L2_PIX_FMT_GREY 'GREY', V4L2_PIX_FMT_SRGGB8  'RGGB'
//   0x01 : 10bit, 2 lanes, streaming          V4L2_PIX_FMT_Y10  'Y10 ', V4L2_PIX_FMT_SRGGB10 'RG10'
//   0x02 : 12bit, 2 lanes, streaming          V4L2_PIX_FMT_Y12  'Y12 ', V4L2_PIX_FMT_SRGGB12 'RG12'
//   0x03 :  8bit, 2 lanes, external trigger global shutter reset
//   0x04 : 10bit, 2 lanes, external trigger global shutter reset
//   0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
//   0x06 :  8bit, 4 lanes, streaming
//   0x07 : 10bit, 4 lanes, streaming
//   0x08 : 12bit, 4 lanes, streaming
//   0x09 :  8bit, 4 lanes, external trigger global shutter reset
//   0x0A : 10bit, 4 lanes, external trigger global shutter reset
//   0x0B : 12bit, 4 lanes, external trigger global shutter reset

    if(pix_fmt == V4L2_PIX_FMT_GREY || pix_fmt == V4L2_PIX_FMT_SRGGB8)
    {
      sensor_mode = 0;      // 8-bit
    }
    else if(pix_fmt == V4L2_PIX_FMT_Y10 || pix_fmt == V4L2_PIX_FMT_SRGGB10)
    {
      sensor_mode = 1;      // 10-bit
    }
    else if(pix_fmt == V4L2_PIX_FMT_Y12 || pix_fmt == V4L2_PIX_FMT_SRGGB12)
    {
      sensor_mode = 2;      // 12-bit
    }

    if(priv->num_lanes == 4)
    {
      sensor_mode += 6;
    }

// Ext. trigger mode
    if(priv->sensor_ext_trig)
    {
      sensor_mode += 3;
    }

//#if IMX183_273_FORCE_SET_MODE

/* ??? PK 06.01.2021 : IMX273 requires FPGA reset, otherwise it produces noise image */
    if(priv->model == IMX273_MODEL_COLOR || priv->model == IMX273_MODEL_MONOCHROME)
    {
      priv->sensor_mode = -1;
    }
//#endif

// Change VC MIPI sensor mode
    if(priv->sensor_mode != sensor_mode)
    {
      priv->sensor_mode = sensor_mode;

#if TRACE_IMX183_273_SET_MODE
      dev_err(dev, "%s(): New sensor_mode=%d (0-2=8/10/12-bit 3-5=8/10/12-bit trig), sensor_ext_trig=%d\n", __func__,
                    sensor_mode, priv->sensor_ext_trig);
#endif

      err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
      if(err)
      {
        dev_err(dev, "%s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
        return err;
      }

//// Xavier: switch between 8-bit and 10-bit modes
////      mdelay(300); // wait 300ms : added in vc_mipi_reset()
//      if(err)
//      {
//          dev_err(dev, "%s(): vc_mipi_reset() error=%d\n", __func__, err);
//      }
//
    } /* if(priv->sensor_mode != sensor_mode) */

#if TRACE_IMX183_273_SET_MODE
{
    dev_err(dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt=0x%08x '%c%c%c%c', sensor_mode=%d\n", __func__,
                        pix.width, pix.height,
                        pix.bytesperline, pix.sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF),
                        sensor_mode );
}
#endif

// Write mode table:
    err = reg_write_table(client, priv->sen_pars.sensor_mode_table);
    if(err)
    {
        dev_err(dev, "%s(): reg_write_table() error=%d\n", __func__, err);
//        goto exit;
    }
    else
    {
#if TRACE_IMX183_273_SET_MODE
//        dev_err(dev, "%s(): reg_write_table() OK, cam_mode=%d\n", __func__, priv->cam_mode);
#endif
    }

    return err;
}

/****** imx296_297_set_mode = Set IMX296-297 mode = 01.2021 *************/
static int imx296_297_set_mode(struct i2c_client *client)
{

#define TRACE_IMX296_297_SET_MODE   1   /* DDD - imx296_297_set_mode() - trace */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    struct v4l2_pix_format pix;
    int pix_fmt;
    int err = 0;

/*............. Get pixel format */
    mx6s_get_pix_fmt(&pix);
    pix_fmt = pix.pixelformat;

/*............. Set new sensor mode */
// IMX296,IMX297 sensor modes:
//   0 = 10-bit free-run
//   1 = 10-bit external trigger
    sensor_mode = 0;      // 10-bit free-run

// Ext. trigger mode
    if(priv->sensor_ext_trig)
    {
      sensor_mode += 1;
    }

// Change VC MIPI sensor mode
    if(priv->sensor_mode != sensor_mode)
    {
      priv->sensor_mode = sensor_mode;

#if TRACE_IMX296_297_SET_MODE
      dev_err(dev, "%s(): New sensor_mode=%d (0=10-bit free-run, 1=10-bit trig), sensor_ext_trig=%d\n", __func__,
                    sensor_mode, priv->sensor_ext_trig);
#endif

      err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
      if(err)
      {
        dev_err(dev, "%s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
        return err;
      }

//// Xavier: switch between 8-bit and 10-bit modes
////      mdelay(300); // wait 300ms : added in vc_mipi_reset()
//      if(err)
//      {
//          dev_err(dev, "%s(): vc_mipi_reset() error=%d\n", __func__, err);
//      }
//
    } /* if(priv->sensor_mode != sensor_mode) */

#if TRACE_IMX296_297_SET_MODE
{
    dev_err(dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt=0x%08x '%c%c%c%c', sensor_mode=%d\n", __func__,
                        pix.width, pix.height,
                        pix.bytesperline, pix.sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF),
                        sensor_mode );
}
#endif

// Write mode table:
    err = reg_write_table(client, priv->sen_pars.sensor_mode_table);
    if(err)
    {
        dev_err(dev, "%s(): reg_write_table() error=%d\n", __func__, err);
//        goto exit;
    }
    else
    {
#if TRACE_IMX296_297_SET_MODE
//        dev_err(dev, "%s(): reg_write_table() OK, cam_mode=%d\n", __func__, priv->cam_mode);
#endif
    }

    return err;
}

/****** imx327_415_set_mode = Set IMX327-IMX415 mode = 01.2021 **********/
static int imx327_415_set_mode(struct i2c_client *client)
{

#define TRACE_IMX327_SET_MODE   1   /* DDD - imx327_set_mode() - trace */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    struct v4l2_pix_format pix;
    int pix_fmt;
    int err = 0;

/*............. Get pixel format */
    mx6s_get_pix_fmt(&pix);
    pix_fmt = pix.pixelformat;


/*............. Set new sensor mode */
// IMX327 sensor modes:
//  0x00 : 10bit free-run 2 lanes
//  0x01 : 10bit free-run 4 lanes

    sensor_mode = 0;        // 10-bit, 2 lanes
    if(priv->num_lanes == 4)
    {
      sensor_mode += 1;     // 10-bit, 4 lanes
    }

// Change VC MIPI sensor mode
    if(priv->sensor_mode != sensor_mode)
    {
      priv->sensor_mode = sensor_mode;
#if TRACE_IMX327_SET_MODE
      dev_err(dev, "%s(): New sensor_mode=%d (0,1=2/4-lanes)\n", __func__, sensor_mode);
#endif

//      err = vc_mipi_reset(tc_dev, sensor_mode);
      err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
      if(err)
      {
        dev_err(dev, "%s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
        return err;
      }

    } /* if(priv->sensor_mode != sensor_mode) */

#if TRACE_IMX327_SET_MODE
{
    dev_err(dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt=0x%08x '%c%c%c%c'\n", __func__,
                        pix.width, pix.height,
                        pix.bytesperline, pix.sizeimage,
                        pix_fmt,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF));
}
#endif

// Write mode table:
    err = reg_write_table(client, priv->sen_pars.sensor_mode_table);
    if(err)
    {
        dev_err(dev, "%s(): reg_write_table() error=%d\n", __func__, err);
//        goto exit;
    }
    else
    {
#if TRACE_IMX327_SET_MODE
//        dev_err(dev, "%s(): reg_write_table() OK, cam_mode=%d\n", __func__, priv->cam_mode);
#endif
    }

    return err;
}

/****** imx_set_mode = Set IMX mode = 01.2021 ***************************/
static int imx_set_mode(struct i2c_client *client)
{

#define TRACE_IMX_SET_MODE   1   /* DDD - imx_set_mode() - trace */
//#define IMX226_FORCE_SET_MODE   0   /* CCC - imx_set_mode() - force set mode */

    struct imx *priv = to_imx(client);
//    struct device *dev = &priv->i2c_client->dev;
//    struct v4l2_pix_format pix;
//    int pix_fmt;
    int err = 0;

    switch(priv->model)
    {
        case OV7251_MODEL_MONOCHROME:
        case OV7251_MODEL_COLOR:
            err = -EINVAL;
            break;

        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            err = ov9281_set_mode(client);
            break;

        case IMX183_MODEL_MONOCHROME:
        case IMX183_MODEL_COLOR:
        case IMX226_MODEL_MONOCHROME:
        case IMX226_MODEL_COLOR:
        case IMX250_MODEL_MONOCHROME:
        case IMX250_MODEL_COLOR:
        case IMX252_MODEL_MONOCHROME:
        case IMX252_MODEL_COLOR:
        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
            err = imx183_273_set_mode(client);
            break;

        case IMX296_MODEL_MONOCHROME:
        case IMX296_MODEL_COLOR:
        case IMX297_MODEL_MONOCHROME:
        case IMX297_MODEL_COLOR:
            err = imx296_297_set_mode(client);
            break;

        case IMX290_MODEL_MONOCHROME:
        case IMX290_MODEL_COLOR:
        case IMX327_MODEL_MONOCHROME:
        case IMX327_MODEL_COLOR:
        case IMX412_MODEL_MONOCHROME:
        case IMX412_MODEL_COLOR:
        case IMX415_MODEL_MONOCHROME:
        case IMX415_MODEL_COLOR:
            err = imx327_415_set_mode(client);
            break;

        default:
            return -EINVAL;
    }


    return err;
}

/****** imx_stop_stream = Stop streaming = 11.2020 **********************/
static int imx_stop_stream(struct i2c_client *client)
{

#define TRACE_IMX_STOP_STREAM           1   /* DDD - imx_stop_stream - trace */
#define STOP_STREAMING_SENSOR_RESET     1   /* CCC - imx_stop_stream - reset sensor before streaming stop */

    struct imx *priv = to_imx(client);
    struct device *dev = &client->dev;
    int ret = 0;

#if STOP_STREAMING_SENSOR_RESET
{
//  /* reinit sensor */
    ret = vc_mipi_common_rom_init(client, priv->rom, -1);
    if (ret)
      return ret;

    ret = vc_mipi_common_rom_init(client, priv->rom, priv->sensor_mode);
    if (ret)
      return ret;

    ret = vc_mipi_common_trigmode_write(priv->rom, 0, 0, 0, 0, 0); /* disable external trigger counter */
    if (ret)
        dev_err(dev, "%s: REINIT: Error %d disabling trigger counter\n", __func__, ret);
}
#endif

//............. Stop sensor streaming
    ret = reg_write_table(client, priv->sen_pars.sensor_stop_table);
    priv->streaming = false;

#if TRACE_IMX_STOP_STREAM
    dev_err(dev, "%s(): err=%d\n", __func__, ret);
#endif

    return ret;
}

/****** imx_start_stream = Start streaming = 11.2020 ********************/
static int imx_start_stream(struct i2c_client *client)
{

#define TRACE_IMX_START_STREAM          1   /* DDD - imx_start_stream - trace */
#define VC_EXT_TRIG_SET_EXPOSURE        1   /* CCC - imx_start_stream - set exposure in trigger mode */

    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    int ret = 0;

    if(priv->streaming)
    {
      ret = imx_stop_stream(client);
      if(ret)
      {
        dev_err(dev, "%s: imx_stop_stream() error=%d\n", __func__, ret);
//        goto exit;
      }
    }

//    imx_set_gain(client);
//    imx_set_exposure(client);

    ret = imx_set_mode(client);
    if(ret)
    {
      dev_err(dev, "%s: imx_set_mode() error=%d\n", __func__, ret);
      goto exit;
    }

// Aft FPGA reset
    imx_set_gain(client);
    imx_set_exposure(client);

    if(priv->sensor_ext_trig)
    {
//        u64 exposure = (priv->exposure_time * 10000) / 185185;
        u64 exposure = (priv->exposure_time * (priv->sen_clk/1000000)); // sen_clk default=54Mhz imx183=72Mhz
        int addr;
        int data;
        int flash_output_enb = priv->flash_output;

        // OV9281 uses this setting for external triggering only
        if(priv->model == OV9281_MODEL_MONOCHROME || priv->model == OV9281_MODEL_COLOR)
        {
          ret += reg_write_table(client, ov9281_stop);
          flash_output_enb += 8;
        }

#if TRACE_IMX_START_STREAM
        dev_err(dev, "%s(): sensor_ext_trig=%d, exposure=%llu (0x%llx)\n", __func__, priv->sensor_ext_trig, exposure, exposure);
#endif

        addr = 0x0108; // ext trig enable
        // data =      1; // external trigger enable
        // data =      4; // TEST external self trigger enable
        data = priv->sensor_ext_trig; // external trigger enable
        ret += reg_write(priv->rom, addr, data);

        addr = 0x0103; // flash output enable
        data = flash_output_enb; // priv->flash_output; // flash output enable
        ret += reg_write(priv->rom, addr, data);
#if TRACE_IMX_START_STREAM
        dev_err(dev, "%s(): flash-output=%d\n", __func__, (int)data);
#endif

#if VC_EXT_TRIG_SET_EXPOSURE  /* [[[ */
        addr = 0x0109; // shutter lsb
        data = exposure & 0xff;
        ret += reg_write(priv->rom, addr, data);

        addr = 0x010a;
        data = (exposure >> 8) & 0xff;
        ret += reg_write(priv->rom, addr, data);

        addr = 0x010b;
        data = (exposure >> 16) & 0xff;
        ret += reg_write(priv->rom, addr, data);

        addr = 0x010c; // shutter msb
        data = (exposure >> 24) & 0xff;
        ret += reg_write(priv->rom, addr, data);
#endif  /* ]]] */

        if(priv->model == OV9281_MODEL_MONOCHROME || priv->model == OV9281_MODEL_COLOR)
        {
          if(ret)
          {
            dev_err(dev, "%s(): reg_write() error=%d\n", __func__, ret);
          }
          else
          {
            priv->streaming = true;
          }
          goto exit;
        }

    }
    else
    {
        int addr = 0x0108; // ext trig enable
        int data =      0; // external trigger disable
        ret = reg_write(priv->rom, addr, data);

        addr = 0x0103; // flash output enable
        data = priv->flash_output; // flash output enable
        ret += reg_write(priv->rom, addr, data);
    }
//    mdelay(10);    //  ms

    if(ret)
    {
      dev_err(dev, "%s(): reg_write() error=%d\n", __func__, ret);
      goto exit;
    }

//............. Start sensor streaming
//    mutex_lock(&priv->mutex);
    ret = reg_write_table(client, priv->sen_pars.sensor_start_table);
    if (ret) {
        goto exit;
    } else {
        priv->streaming = true;
    }

//    mutex_unlock(&priv->mutex);
//    usleep_range(50000, 51000);

//............... Exit
exit:
#if TRACE_IMX_START_STREAM
    dev_err(dev, "%s(): err=%d\n", __func__, ret);
#endif

    return ret;
}

/****** imx_s_stream = Start/stop stream = 11.2020 **********************/
static int imx_s_stream(struct v4l2_subdev *sd, int enable)
{

#define TRACE_IMX_S_STREAM      0   /* DDD - imx_s_stream - trace */
#define IMX_S_STREAM_DUMP_FPGA  0   /* DDD - imx_s_stream - dump FPGA control regs */

    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct imx *priv = to_imx(client);
    struct device *dev = &priv->i2c_client->dev;
    int err = 0;

//#if TRACE_IMX_S_STREAM || IMX_S_STREAM_DUMP_FPGA
//#endif

#if TRACE_IMX_S_STREAM
    dev_err(dev, "%s: enable=%d\n", __func__, enable);
#endif

    if (enable)
    {
        err = imx_start_stream(client);
        if(err)
        {
          dev_err(dev, "%s: imx_start_stream() error=%d\n", __func__, err);
        }
#if IMX_S_STREAM_DUMP_FPGA
        dev_err(dev, "%s: ----- FPGA dump after start stream -----\n", __func__);
        dump_fpga_regs(priv);
#endif
    }
    else
    {
#if IMX_S_STREAM_DUMP_FPGA
        dev_err(dev, "%s: ----- FPGA dump before stop stream -----\n", __func__);
        dump_fpga_regs(priv);
#endif
        err = imx_stop_stream(client);
        if(err)
        {
          dev_err(dev, "%s: imx_stop_stream() error=%d\n", __func__, err);
        }
    }

    return err;
}

static struct v4l2_subdev_video_ops imx_subdev_video_ops = {
//    .g_parm = imx_g_parm,
//    .s_parm = imx_s_parm,
    .s_stream = imx_s_stream,
};

static const struct v4l2_subdev_pad_ops imx_subdev_pad_ops = {
    .enum_frame_size       = imx_enum_framesizes,
//    .enum_frame_interval   = imx_enum_frameintervals,
    .enum_mbus_code        = imx_enum_mbus_code,
    .set_fmt               = imx_set_fmt,
    .get_fmt               = imx_get_fmt,
};

static struct v4l2_subdev_core_ops imx_subdev_core_ops = {
    .s_power     = imx_s_power,

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD1  /* [[[ */
// custom controls...
    .queryctrl     = imx_queryctrl,
    .queryextctrl  = imx_queryextctrl,
    .querymenu     = imx_querymenu,
    .g_ctrl        = imx_g_ctrl,
    .s_ctrl        = imx_s_ctrl,
    .g_ext_ctrls   = imx_g_ext_ctrls,
    .s_ext_ctrls   = imx_s_ext_ctrls,
    .try_ext_ctrls = imx_try_ext_ctrls,
#endif  /* ]]] */

//#ifdef CONFIG_VIDEO_ADV_DEBUG
//    .g_register = imx_get_register,
//    .s_register = imx_set_register,
//#endif
};

static struct v4l2_subdev_ops imx_subdev_ops = {
    .core  = &imx_subdev_core_ops,
    .video = &imx_subdev_video_ops,
    .pad   = &imx_subdev_pad_ops,
};

/****** imx_probe_vc_rom = Probe VC ROM = 11.2020 ***********************/
static struct i2c_client * imx_probe_vc_rom(struct i2c_adapter *adapter, u8 addr)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("dummy", addr),
    };
    unsigned short addr_list[2] = { addr, I2C_CLIENT_END };

    return i2c_new_probed_device(adapter, &info, addr_list, NULL);
}

/****** imx_board_setup = IMX sensor board setup = 11.2020 **************/
static int imx_board_setup(struct imx *priv)
{

#define TRACE_IMX_BOARD_SETUP     1   /* DDD - imx_board_setup() - trace */
#define DUMP_ROM_TABLE_REGS       0   /* DDD - imx_board_setup() - dump ROM table registers */
#define DUMP_CTL_REG_DATA         0   /* DDD - imx_board_setup() - dump module control registers 0x100-0x108 (I2C addr=0x10) */
#define DUMP_HWD_DESC_ROM_DATA    0   /* DDD - imx_board_setup() - dump Hardware Desriptor ROM data (I2C addr=0x10) */

    struct i2c_client *client = priv->i2c_client;
    struct device *dev = &client->dev;
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

    int err = 0;
//    int ret;
//    char *model_name;


/*............. Setup */
    priv->num_lanes = csi_num_lanes();
    priv->color = 0;

///*............. Set sensor mode for 4 lanes */
//    if(priv->num_lanes == 2)
//    {
//      sensor_mode = 1;    // autoswitch to sensor_mode=1 if 2 lanes are given
//    }
//    else if(priv->num_lanes == 4)
//    {
//      sensor_mode = 7;    // autoswitch to sensor_mode=7 if 4 lanes are given
//    }
//    else
//    {
//        dev_err(dev, "%s: VC Sensor device-tree: Invalid number of data-lanes: %d\n",__func__, priv->num_lanes);
//        return -EINVAL;
//    }

#if TRACE_IMX_BOARD_SETUP
    dev_err(dev, "%s: VC Sensor device-tree has configured %d data-lanes: sensor_mode=%d\n",__func__, priv->num_lanes, sensor_mode);
#endif


/*............. Read ROM table */
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(dev, "%s(): I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n", __func__);
        return -EIO;
    }

    priv->rom = imx_probe_vc_rom(adapter,0x10);
    if ( priv->rom )
    {
//        static int i=1;
        int addr,reg;

#if DUMP_HWD_DESC_ROM_DATA      /* dump Hardware Desriptor ROM data */
        dev_err(&client->dev, "%s(): Dump Hardware Descriptor ROM data:\n", __func__);
#endif

        for (addr=0; addr<sizeof(priv->rom_table); addr++)
        {
          reg = reg_read(priv->rom, addr+0x1000);
          if(reg < 0)
          {
              i2c_unregister_device(priv->rom);
              return -EIO;
          }
          *((char *)(&(priv->rom_table))+addr)=(char)reg;
#if DUMP_HWD_DESC_ROM_DATA      /* [[[ - dump Hardware Desriptor ROM data */
{
          static int sval = 0;   // short 2-byte value

          if(addr & 1)  // odd addr
          {
            sval |= (int)reg << 8;
            dev_err(dev, "addr=0x%04x reg=0x%04x\n",addr+0x1000-1,sval);
          }
          else
          {
            sval = reg;
          }
}
#endif  // ]]]

        } /* for (addr=0; addr<sizeof(priv->rom_table); addr++) */

        dev_err(dev, "%s(): VC FPGA found!\n", __func__);

        dev_err(dev, "[ MAGIC  ] [ %s ]\n",
                priv->rom_table.magic);

        dev_err(dev, "[ MANUF. ] [ %s ] [ MID=0x%04x ]\n",
                priv->rom_table.manuf,
                priv->rom_table.manuf_id);

        dev_err(dev, "[ SENSOR ] [ %s %s ]\n",
                priv->rom_table.sen_manuf,
                priv->rom_table.sen_type);

        dev_err(dev, "[ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n",
                priv->rom_table.mod_id,
                priv->rom_table.mod_rev);

        dev_err(dev, "[ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n",
                priv->rom_table.nr_modes,
                priv->rom_table.bytes_per_mode);

        if(priv->rom_table.sen_type)
        {
            u32 len = strnlen(priv->rom_table.sen_type,16);
            if(len > 0 && len < 17)
            {
                if( *(priv->rom_table.sen_type+len-1) == 'C' )
                {
                    dev_err(dev, "[ COLOR  ] [  %c ]\n",*(priv->rom_table.sen_type+len-1));
                    priv->color = 1;
                }
                else
                {
                    dev_err(dev, "[ MONO   ] [ B/W ]\n");
                    priv->color = 0;
                }
            } //TODO else

        } /* if(priv->rom_table.sen_type) */

//# sensor registers:
//004A:0B 70          # chip ID high byte     = 0x700B    value = 0x01
//004C:0A 70          # chip ID low byte      = 0x700A    value = 0x83
//004E:0C 70          # chip revision         = 0x700C    value = 0x00
//#
//0050:00 70          # idle                  = 0x7000
//0052:14 60          # horizontal start high = 0x6014
//0054:13 60          # horizontal start low  = 0x6013
//0056:0F 60          # vertical start high   = 0x600F
//0058:0E 60          # vertical start low    = 0x600E
//005A:00 00          # horizontal end H      = 0x0000
//005C:00 00          # horizontal end L      = 0x0000
//005E:00 00          # vertical   end H      = 0x0000
//0060:00 00          # vertical   end L      = 0x0000
//0062:16 60          # hor. output width H   = 0x6016
//0064:15 60          # hor. output width L   = 0x6015
//0066:11 60          # ver. output height H  = 0x6011
//0068:10 60          # ver. output height L  = 0x6010
//006A:00 00          # exposure H            = 0x0000
//006C:0C 00          # exposure M            = 0x000C
//006E:0B 00          # exposure L            = 0x000B
//0070:0A 00          # gain high             = 0x000A
//0072:09 00          # gain low              = 0x0009
//#

#if DUMP_ROM_TABLE_REGS
{
    int i;
    dev_err(dev, "ROM table register dump:\n");
    for(i=0; i<56; i+=2)
    {
      dev_err(dev, "0x%02x: 0x%02x 0x%02x\n", i, (int)priv->rom_table.regs[i], (int)priv->rom_table.regs[i+1]);
    }
}
#endif

/*............. Detect sensor model */
        switch(priv->rom_table.mod_id)
        {
            case 0x7251:
                priv->model = priv->color ? OV9281_MODEL_COLOR : OV9281_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "OV7251 color" : "OV7251 mono");
                break;

            case 0x9281:
                priv->model = priv->color ? OV9281_MODEL_COLOR : OV9281_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "OV9281 color" : "OV9281 mono");
                break;

            case 0x0183:
                priv->model = priv->color ? IMX183_MODEL_COLOR : IMX183_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX183 color" : "IMX183 mono");
                break;

            case 0x0226:
                priv->model = priv->color ? IMX226_MODEL_COLOR : IMX226_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX226 color" : "IMX226 mono");
                break;

            case 0x0250:
                priv->model = priv->color ? IMX250_MODEL_COLOR : IMX250_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX250 color" : "IMX250 mono");
                break;

            case 0x0252:
                priv->model = priv->color ? IMX252_MODEL_COLOR : IMX252_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX252 color" : "IMX252 mono");
                break;

            case 0x0273:
                priv->model = priv->color ? IMX273_MODEL_COLOR : IMX273_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX273 color" : "IMX273 mono");
                break;

            case 0x0290:
                priv->model = priv->color ? IMX290_MODEL_COLOR : IMX290_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX290 color" : "IMX290 mono");
                break;

            case 0x0296:
                priv->model = priv->color ? IMX296_MODEL_COLOR : IMX296_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX296 color" : "IMX296 mono");
                break;

            case 0x0297:
                priv->model = priv->color ? IMX297_MODEL_COLOR : IMX297_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX297 color" : "IMX297 mono");
                break;

            case 0x0327:
                priv->model = priv->color ? IMX327_MODEL_COLOR : IMX327_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX327 color" : "IMX327 mono");
                break;

            case 0x0412:
                priv->model = priv->color ? IMX412_MODEL_COLOR : IMX412_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX412 color" : "IMX412 mono");
                break;

            case 0x0415:
                priv->model = priv->color ? IMX415_MODEL_COLOR : IMX415_MODEL_MONOCHROME;
                strcpy(priv->model_name, priv->color ? "IMX415 color" : "IMX415 mono");
                break;

            default:
                priv->model = SEN_MODEL_UNKNOWN;
                break;

        } /* switch(priv->rom_table.mod_id) */

        if(priv->model == SEN_MODEL_UNKNOWN)
        {
          dev_err(dev, "%s(): Unknown sensor model=0x%04x, err=%d\n", __func__, priv->rom_table.mod_id, -EIO);
          return -EIO;
        }
        else
        {
#if TRACE_IMX_BOARD_SETUP
          dev_err(dev, "%s(): Detected sensor model: '%s'\n", __func__, priv->model_name);
#endif
        }

//............. Reset VC MIPI sensor: Initialize FPGA: reset sensor registers to def. values
        priv->sensor_mode = sensor_mode;
        err = vc_mipi_common_rom_init(client, priv->rom, sensor_mode);
        if(err)
        {
          dev_err(dev, "%s(): vc_mipi_common_rom_init() err=%d\n", __func__, err);
          return err;
        }

#if DUMP_CTL_REG_DATA
{
        int i;
        int reg_val[10];

        dev_err(dev, "%s(): Module controller registers (0x10):\n", __func__);

//        addr = 0x100;
        i = 0;
        for(addr=0x100; addr<=0x108; addr++)
        {
          reg_val[i] = reg_read(priv->rom, addr);
//          dev_err(&client->dev, "0x%04x: %02x\n", addr, reg_val[i]);
          i++;
        }

        dev_err(dev, "0x100-103: %02x %02x %02x %02x\n", reg_val[0],reg_val[1],reg_val[2],reg_val[3]);
        dev_err(dev, "0x104-108: %02x %02x %02x %02x %02x\n", reg_val[4],reg_val[5],reg_val[6],reg_val[7],reg_val[8]);

}
#endif

    } /* if ( priv->rom ) */

    else
    {
        dev_err(dev, "%s(): Error !!! VC FPGA not found !!!\n", __func__);
        return -EIO;
    }

//done:
    return err;
}

/****** imx_param_setup = Sensor parameter setup = 01.2021 **************/
static int imx_param_setup(struct imx *priv)
{

#define TRACE_IMX_PARAM_SETUP    0   /* DDD - imx_param_setup() - trace */
#define DUMP_V4L2_CID_PARAMS        0   /* DDD - imx_param_setup() - dump V4L2 CID params */

    struct i2c_client *client = priv->i2c_client;
    struct device *dev = &client->dev;

#if TRACE_IMX_PARAM_SETUP || DUMP_V4L2_CID_PARAMS
//    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
#endif

    int ret = 0;

#if TRACE_IMX_PARAM_SETUP
//    dev_err(dev, "%s: ...\n", __func__);
#endif

    switch(priv->model)
    {

/*............. OV9281 */
        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            priv->sen_pars.gain_min         = OV9281_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = OV9281_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = OV9281_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = OV9281_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = OV9281_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = OV9281_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = OV9281_DX;
            priv->sen_pars.frame_dy         = OV9281_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)ov9281_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)ov9281_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)ov9281_mode_1280_800;

            if(priv->model == OV9281_MODEL_MONOCHROME)
            {
              imx_data_fmts      = ov9281_mono_fmts;
              imx_data_fmts_size = ov9281_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = ov9281_color_fmts;
              imx_data_fmts_size = ov9281_color_fmts_size;
            }

            break;

/*............. IMX183 */
        case IMX183_MODEL_MONOCHROME:
        case IMX183_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX183_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX183_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX183_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX183_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX183_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX183_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX183_DX;
            priv->sen_pars.frame_dy         = IMX183_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx183_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx183_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx183_mode_5440_3692;

            if(priv->model == IMX183_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx183_mono_fmts;
              imx_data_fmts_size = imx183_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx183_color_fmts;
              imx_data_fmts_size = imx183_color_fmts_size;
            }

            break;

/*............. IMX226 */
        case IMX226_MODEL_MONOCHROME:
        case IMX226_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX226_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX226_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX226_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX226_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX226_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX226_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX226_DX;
            priv->sen_pars.frame_dy         = IMX226_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx226_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx226_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx226_mode_3840_3040;

            if(priv->model == IMX226_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx226_mono_fmts;
              imx_data_fmts_size = imx226_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx226_color_fmts;
              imx_data_fmts_size = imx226_color_fmts_size;
            }
            break;

/*............. IMX250 */
        case IMX250_MODEL_MONOCHROME:
        case IMX250_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX250_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX250_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX250_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX250_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX250_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX250_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX250_DX;
            priv->sen_pars.frame_dy         = IMX250_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx250_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx250_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx250_mode_2432_2048;

            if(priv->model == IMX250_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx250_mono_fmts;
              imx_data_fmts_size = imx250_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx250_color_fmts;
              imx_data_fmts_size = imx250_color_fmts_size;
            }
            break;

/*............. IMX252 */
        case IMX252_MODEL_MONOCHROME:
        case IMX252_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX252_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX252_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX252_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX252_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX252_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX252_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX252_DX;
            priv->sen_pars.frame_dy         = IMX252_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx252_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx252_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx252_mode_2048_1536;

            if(priv->model == IMX252_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx252_mono_fmts;
              imx_data_fmts_size = imx252_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx252_color_fmts;
              imx_data_fmts_size = imx252_color_fmts_size;
            }
            break;

/*............. IMX273 */
        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX273_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX273_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX273_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX273_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX273_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX273_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX273_DX;
            priv->sen_pars.frame_dy         = IMX273_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx273_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx273_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx273_mode_1440_1080;

            if(priv->model == IMX273_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx273_mono_fmts;
              imx_data_fmts_size = imx273_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx273_color_fmts;
              imx_data_fmts_size = imx273_color_fmts_size;
            }

            break;

/*............. IMX296 */
        case IMX296_MODEL_MONOCHROME:
        case IMX296_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX296_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX296_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX296_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX296_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX296_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX296_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX296_DX;
            priv->sen_pars.frame_dy         = IMX296_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx296_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx296_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx296_mode_1440_1080;

            if(priv->model == IMX296_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx296_mono_fmts;
              imx_data_fmts_size = imx296_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx296_color_fmts;
              imx_data_fmts_size = imx296_color_fmts_size;
            }
            break;

/*............. IMX297 */
        case IMX297_MODEL_MONOCHROME:
        case IMX297_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX297_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX297_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX297_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX297_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX297_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX297_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX297_DX;
            priv->sen_pars.frame_dy         = IMX297_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx297_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx297_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx297_mode_720_540;

            if(priv->model == IMX297_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx297_mono_fmts;
              imx_data_fmts_size = imx297_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx297_color_fmts;
              imx_data_fmts_size = imx297_color_fmts_size;
            }
            break;

/*............. IMX290 */
        case IMX290_MODEL_MONOCHROME:
        case IMX290_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX290_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX290_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX290_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX290_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX290_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX290_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX290_DX;
            priv->sen_pars.frame_dy         = IMX290_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx290_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx290_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx290_mode_1920_1080;

            if(priv->model == IMX290_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx290_mono_fmts;
              imx_data_fmts_size = imx290_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx290_color_fmts;
              imx_data_fmts_size = imx290_color_fmts_size;
            }
            break;

/*............. IMX327 */
        case IMX327_MODEL_MONOCHROME:
        case IMX327_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX327_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX327_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX327_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX327_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX327_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX327_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX327_DX;
            priv->sen_pars.frame_dy         = IMX327_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx327_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx327_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx327_mode_1920_1080;

            if(priv->model == IMX327_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx327_mono_fmts;
              imx_data_fmts_size = imx327_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx327_color_fmts;
              imx_data_fmts_size = imx327_color_fmts_size;
            }
            break;

/*............. IMX415 */
        case IMX415_MODEL_MONOCHROME:
        case IMX415_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX415_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX415_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX415_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX415_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX415_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX415_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX415_DX;
            priv->sen_pars.frame_dy         = IMX415_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx415_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx415_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx415_mode_3864_2192;

            if(priv->model == IMX415_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx415_mono_fmts;
              imx_data_fmts_size = imx415_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx415_color_fmts;
              imx_data_fmts_size = imx415_color_fmts_size;
            }
            break;

/*............. IMX412 */
        case IMX412_MODEL_MONOCHROME:
        case IMX412_MODEL_COLOR:
            priv->sen_pars.gain_min         = IMX412_DIGITAL_GAIN_MIN;
            priv->sen_pars.gain_max         = IMX412_DIGITAL_GAIN_MAX;
            priv->sen_pars.gain_default     = IMX412_DIGITAL_GAIN_DEFAULT;
            priv->sen_pars.exposure_min     = IMX412_DIGITAL_EXPOSURE_MIN;
            priv->sen_pars.exposure_max     = IMX412_DIGITAL_EXPOSURE_MAX;
            priv->sen_pars.exposure_default = IMX412_DIGITAL_EXPOSURE_DEFAULT;
            priv->sen_pars.frame_dx         = IMX412_DX;
            priv->sen_pars.frame_dy         = IMX412_DY;

            priv->sen_pars.sensor_start_table = (struct sensor_reg *)imx412_start;
            priv->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx412_stop;
            priv->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx412_mode_4032_3040;

            if(priv->model == IMX412_MODEL_MONOCHROME)
            {
              imx_data_fmts      = imx412_mono_fmts;
              imx_data_fmts_size = imx412_mono_fmts_size;
            }
            else
            {
              imx_data_fmts      = imx412_color_fmts;
              imx_data_fmts_size = imx412_color_fmts_size;
            }
            break;

        default:
            return -EIO;

    } /* switch(priv->model) */

    priv->pix.width  = priv->sen_pars.frame_dx; // 640;
    priv->pix.height = priv->sen_pars.frame_dy; // 480;

    imx_valid_res[0].width  = priv->sen_pars.frame_dx;
    imx_valid_res[0].height = priv->sen_pars.frame_dy;

    priv->digital_gain  = priv->sen_pars.gain_default;
    priv->exposure_time = priv->sen_pars.exposure_default;

    priv->sensor_ext_trig = 0;    // ext. trigger flag: 0=no, 1=yes

    if(priv->model == IMX183_MODEL_MONOCHROME || priv->model == IMX183_MODEL_COLOR)
    {
      priv->sen_clk = 72000000;     // clock-frequency: default=54Mhz imx183=72Mhz
    }
    else
    {
      priv->sen_clk = 54000000;     // clock-frequency: default=54Mhz imx183=72Mhz
    }

    priv->flash_output = flash_output;
    priv->streaming = false;
//    priv->num_lanes = csi_num_lanes();

/*............. Set user control ranges */
#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    ret = mx6s_set_ctrl_range (
            V4L2_CID_GAIN,                  /* int id,             [in] control id            */
            priv->sen_pars.gain_min,        /* int minimum,        [in] minimum control value */
            priv->sen_pars.gain_max,        /* int maximum,        [in] minimum control value */
            priv->sen_pars.gain_default,    /* int default_value,  [in] default control value */
            1);                             /* int step );         [in] control's step value  */
    if(ret)
    {
      dev_err(dev, "%s: mx6s_set_ctrl_range() gain error=%d\n", __func__, ret);
      goto done;
    }

    ret = mx6s_set_ctrl_range (
            V4L2_CID_EXPOSURE,              /* int id,             [in] control id            */
            priv->sen_pars.exposure_min,    /* int minimum,        [in] minimum control value */
            priv->sen_pars.exposure_max,    /* int maximum,        [in] minimum control value */
            priv->sen_pars.exposure_default,/* int default_value,  [in] default control value */
            1);                             /* int step );         [in] control's step value  */
    if(ret)
    {
      dev_err(dev, "%s: mx6s_set_ctrl_range() exp error=%d\n", __func__, ret);
      goto done;
    }
#endif

#if TRACE_IMX_PARAM_SETUP
    dev_err(dev, "%s: num_lanes=%d\n", __func__, priv->num_lanes);
#endif

#if DUMP_V4L2_CID_PARAMS
    dev_err(dev, "V4L2_CID_BRIGHTNESS         = 0x%08x\n",V4L2_CID_BRIGHTNESS         );
    dev_err(dev, "V4L2_CID_CONTRAST           = 0x%08x\n",V4L2_CID_CONTRAST           );
    dev_err(dev, "V4L2_CID_SATURATION         = 0x%08x\n",V4L2_CID_SATURATION         );
    dev_err(dev, "V4L2_CID_HUE                = 0x%08x\n",V4L2_CID_HUE                );
    dev_err(dev, "V4L2_CID_AUDIO_VOLUME       = 0x%08x\n",V4L2_CID_AUDIO_VOLUME       );
    dev_err(dev, "V4L2_CID_AUDIO_BALANCE      = 0x%08x\n",V4L2_CID_AUDIO_BALANCE      );
    dev_err(dev, "V4L2_CID_AUDIO_BASS         = 0x%08x\n",V4L2_CID_AUDIO_BASS         );
    dev_err(dev, "V4L2_CID_AUDIO_TREBLE       = 0x%08x\n",V4L2_CID_AUDIO_TREBLE       );
    dev_err(dev, "V4L2_CID_AUDIO_MUTE         = 0x%08x\n",V4L2_CID_AUDIO_MUTE         );
    dev_err(dev, "V4L2_CID_AUDIO_LOUDNESS     = 0x%08x\n",V4L2_CID_AUDIO_LOUDNESS     );
    dev_err(dev, "V4L2_CID_BLACK_LEVEL        = 0x%08x\n",V4L2_CID_BLACK_LEVEL        );
    dev_err(dev, "V4L2_CID_AUTO_WHITE_BALANCE = 0x%08x\n",V4L2_CID_AUTO_WHITE_BALANCE );
    dev_err(dev, "V4L2_CID_DO_WHITE_BALANCE   = 0x%08x\n",V4L2_CID_DO_WHITE_BALANCE   );
    dev_err(dev, "V4L2_CID_RED_BALANCE        = 0x%08x\n",V4L2_CID_RED_BALANCE        );
    dev_err(dev, "V4L2_CID_BLUE_BALANCE       = 0x%08x\n",V4L2_CID_BLUE_BALANCE       );
    dev_err(dev, "V4L2_CID_GAMMA              = 0x%08x\n",V4L2_CID_GAMMA              );
    dev_err(dev, "V4L2_CID_WHITENESS          = 0x%08x\n",V4L2_CID_WHITENESS          );
    dev_err(dev, "V4L2_CID_EXPOSURE           = 0x%08x\n",V4L2_CID_EXPOSURE           );
    dev_err(dev, "V4L2_CID_AUTOGAIN           = 0x%08x\n",V4L2_CID_AUTOGAIN           );
    dev_err(dev, "V4L2_CID_GAIN               = 0x%08x\n",V4L2_CID_GAIN               );
    dev_err(dev, "V4L2_CID_HFLIP              = 0x%08x\n",V4L2_CID_HFLIP              );
    dev_err(dev, "V4L2_CID_VFLIP              = 0x%08x\n",V4L2_CID_VFLIP              );
#endif

done:
    return ret;
}

/****** imx_set_pixel_format = Set pixel format = 12.2020 ***************/
static int imx_set_pixel_format(struct imx *priv)
{

#define TRACE_IMX_SET_PIXEL_FORMAT      1   /* DDD - imx_set_pixel_format - trace */

    struct v4l2_pix_format pix = {0};
    int bpp = 1;                    // bytes per pixel
    int sen_data_mode = 0;          // sensor data mode: 0=8-bit, 1=10-bit, 2=12-bit
//    int model = priv->model;      // sensor model
    int rc = 0;

//    switch(priv->family)
    switch(priv->model)
    {

//------------------------------------------------------------------------------
// OV9281 - OV7251 sensor modes:
//------------------------------------------------------------------------------
// OV9281 sensor modes:
//  0x00 : 10bit free-run streaming
//  0x01 : 8bit  free-run streaming
//  0x02 : 10bit external trigger
//  0x03 : 8bit  external trigger
        case OV7251_MODEL_MONOCHROME:
        case OV7251_MODEL_COLOR:
        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            sen_data_mode = sensor_mode % 2;    // sensor data mode: 0=10-bit, 1=8-bit
            if(sen_data_mode == 0)        /* VC sensor mode: 10-bit data */
            {
              pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB10 : V4L2_PIX_FMT_Y10;
              bpp = RAW10_BPP;
            }
            else if(sen_data_mode == 1)   /* VC sensor mode: 8-bit data */
            {
              pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB8 : V4L2_PIX_FMT_GREY;
              bpp = 1;
            }
            break;

//------------------------------------------------------------------------------
// IMX183 - IMX273 sensor modes:
//------------------------------------------------------------------------------
//   0x00 :  8bit, 2 lanes, streaming          V4L2_PIX_FMT_GREY 'GREY', V4L2_PIX_FMT_SRGGB8  'RGGB'
//   0x01 : 10bit, 2 lanes, streaming          V4L2_PIX_FMT_Y10  'Y10 ', V4L2_PIX_FMT_SRGGB10 'RG10'
//   0x02 : 12bit, 2 lanes, streaming          V4L2_PIX_FMT_Y12  'Y12 ', V4L2_PIX_FMT_SRGGB12 'RG12'
//   0x03 :  8bit, 2 lanes, external trigger global shutter reset
//   0x04 : 10bit, 2 lanes, external trigger global shutter reset
//   0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
//   0x06 :  8bit, 4 lanes, streaming
//   0x07 : 10bit, 4 lanes, streaming
//   0x08 : 12bit, 4 lanes, streaming
//   0x09 :  8bit, 4 lanes, external trigger global shutter reset
//   0x0A : 10bit, 4 lanes, external trigger global shutter reset
//   0x0B : 12bit, 4 lanes, external trigger global shutter reset
        case IMX183_MODEL_MONOCHROME:
        case IMX183_MODEL_COLOR:
        case IMX226_MODEL_MONOCHROME:
        case IMX226_MODEL_COLOR:
        case IMX250_MODEL_MONOCHROME:
        case IMX250_MODEL_COLOR:
        case IMX252_MODEL_MONOCHROME:
        case IMX252_MODEL_COLOR:
        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
            sen_data_mode = sensor_mode % 3;    // sensor data mode: 0=8-bit, 1=10-bit, 2=12-bit
            if(sen_data_mode == 0)        /* VC sensor mode: 8-bit data */
            {
              pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB8 : V4L2_PIX_FMT_GREY;
              bpp = 1;
            }
            else if(sen_data_mode == 1)   /* VC sensor mode: 10-bit data */
            {
              pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB10 : V4L2_PIX_FMT_Y10;
              bpp = RAW10_BPP;
            }
            else if(sen_data_mode == 2)   /* VC sensor mode: 12-bit data */
            {
              pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB12 : V4L2_PIX_FMT_Y12;
              bpp = RAW12_BPP;
            }
            break;

//------------------------------------------------------------------------------
// IMX296, IMX297 sensor modes:
//------------------------------------------------------------------------------
//  0 = 10-bit free-run
//  1 = 10-bit external trigger
        case IMX296_MODEL_MONOCHROME:
        case IMX296_MODEL_COLOR:
        case IMX297_MODEL_MONOCHROME:
        case IMX297_MODEL_COLOR:
            pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB10 : V4L2_PIX_FMT_Y10;
            bpp = RAW10_BPP;
            break;

//------------------------------------------------------------------------------
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//------------------------------------------------------------------------------
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)
        case IMX290_MODEL_MONOCHROME:
        case IMX290_MODEL_COLOR:
        case IMX327_MODEL_MONOCHROME:
        case IMX327_MODEL_COLOR:
        case IMX412_MODEL_MONOCHROME:
        case IMX412_MODEL_COLOR:
            pix.pixelformat = priv->color ? V4L2_PIX_FMT_SRGGB10 : V4L2_PIX_FMT_Y10;
            bpp = RAW10_BPP;
            break;

        case IMX415_MODEL_MONOCHROME:
        case IMX415_MODEL_COLOR:
            pix.pixelformat = priv->color ? V4L2_PIX_FMT_SGBRG10 : V4L2_PIX_FMT_Y10;
            bpp = RAW10_BPP;
            break;

        default:
            return -EINVAL;

    } /* switch(priv->model) */

    pix.width        = priv->sen_pars.frame_dx;
    pix.height       = priv->sen_pars.frame_dy;
    pix.bytesperline = bpp * pix.width;
    pix.sizeimage    = pix.bytesperline * pix.height;
    pix.field        = V4L2_FIELD_NONE;

    mx6s_set_pix_fmt(&pix);

#if TRACE_IMX_SET_PIXEL_FORMAT
{
    struct device *dev = &priv->i2c_client->dev;
    int pix_fmt = pix.pixelformat;

//    dev_err(dev, "%s: width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt=0x%08x '%c%c%c%c', sensor_mode=%d\n", __func__,
    dev_err(dev, "width,height=%d,%d bytesperline=%d sizeimage=%d pix_fmt='%c%c%c%c', sensor_mode=%d\n",
                        pix.width, pix.height,
                        pix.bytesperline, pix.sizeimage,
                        (char)((pix_fmt      ) & 0xFF),
                        (char)((pix_fmt >>  8) & 0xFF),
                        (char)((pix_fmt >> 16) & 0xFF),
                        (char)((pix_fmt >> 24) & 0xFF),
                        sensor_mode );
}
#endif

    return rc;
}

/****** imx_probe = IMX sensor probe = 11.2020 **************************/
/*!
 * IMX sensor I2C probe function
 *
 * @param client
 * @param id
 * @return
 *   Error code indicating success or failure
 */
static int imx_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{

#define TRACE_IMX_PROBE         1   /* DDD - imx_probe - trace */

    struct device *dev = &client->dev;
    int ret;
//    int retval;
    struct imx *priv;

#if TRACE_IMX_PROBE
    dev_err(dev, "%s(): Probing v4l2 sensor at addr 0x%0x - %s/%s\n", __func__, client->addr, __DATE__, __TIME__);
#endif

    priv = devm_kzalloc(dev, sizeof(struct imx), GFP_KERNEL);
    if (!priv)
    {
        dev_err(dev, "%s: devm_kzalloc error\n", __func__);
        return -ENOMEM;
    }

//    ret = of_property_read_u32(dev->of_node, "csi_id",
//                    &(priv->csi));
//    if (ret) {
//        dev_err(dev, "csi id missing or invalid\n");
//        return ret;
//    }

//    priv->io_init = imx_reset;
    priv->i2c_client = client;
    priv->pix.pixelformat = V4L2_PIX_FMT_GREY;
    priv->pix.width  = IMX226_DX;
    priv->pix.height = IMX226_DY;

#if 0  // ???
    priv->streamcap.capability = V4L2_MODE_HIGHQUALITY |
                       V4L2_CAP_TIMEPERFRAME;
    priv->streamcap.capturemode = 0;
    priv->streamcap.timeperframe.denominator = DEFAULT_FPS;
    priv->streamcap.timeperframe.numerator = 1;
#endif


//----------------------------------------------------------------------
//                                VC stuff
//----------------------------------------------------------------------
#if VC_MIPI     /* [[[ */
    ret = imx_board_setup(priv);
    if(ret)
    {
        dev_err(dev, "%s: imx_board_setup() ret=%d\n", __func__, ret);
        return -EIO;
    }

    ret = imx_param_setup(priv);
    if(ret)
    {
        dev_err(dev, "%s: imx_param_setup() ret=%d\n", __func__, ret);
        return -EIO;
    }

#endif  /* ]]] */

//
//#if 0  // ???
//    ret = imx_init_device(priv);
//    if (ret < 0) {
//        dev_err(dev, "%s: Camera init failed\n", __func__);
////        clk_disable_unprepare(priv->sensor_clk);
////        imx_power_down(priv, 1);
//        return ret;
//    }
//#endif


    v4l2_i2c_subdev_init(&priv->subdev, client, &imx_subdev_ops);

//#if 0   // VC
//    ret = imx_ctrls_init(&priv->subdev);
//    if (ret < 0)
//        goto err_out;
//#endif

#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    ret = imx_init_controls(priv);
    if (ret)
    {
        dev_err(&client->dev, "imx_init_controls error=%d\n", ret);
        imx_free_controls(priv);
        goto err_out;
    }
#endif

    priv->subdev.grp_id = 678;
    priv->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

#if 1   // VC
    priv->pad.flags = MEDIA_PAD_FL_SOURCE;
    priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
    if (ret < 0)
        goto err_out;
#endif

    ret = v4l2_async_register_subdev(&priv->subdev);
    if(ret < 0)
    {
        dev_err(&client->dev, "%s: Async register failed, ret=%d\n", __func__, ret);
        goto err_out;
    }

/*............. Set pixel format in the 'mx6s-csi' driver */
    ret = imx_set_pixel_format(priv);
    if(ret)
    {
        dev_err(&client->dev, "%s: imx_set_pixel_format() failed, ret=%d\n", __func__, ret);
        goto err_out;
    }


/*............. Exit */
err_out:
    if (ret < 0)
    {
        if(priv->rom)
            i2c_unregister_device(priv->rom);
//        return ret;
    }

//#if !IMX_PROBE_DIS_ERR
//    imx_stop_stream(priv);
//#endif
//    dev_err(dev, "Camera is found\n");

#if TRACE_IMX_PROBE
    if(!ret)
      dev_err(dev, "%s(): Probed VC MIPI sensor '%s' - %s/%s\n",__func__, priv->model_name, __DATE__, __TIME__);
    else
      dev_err(dev, "%s(): Probing VC MIPI sensor failed: rc=%d\n",__func__, ret);
#endif

    return ret;
}

/****** imx_remove = IMX remove = 11.2020 *******************************/
/*!
 * imx I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int imx_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    struct imx *priv = to_imx(client);
#endif

    v4l2_async_unregister_subdev(sd);
#if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
    imx_free_controls(priv);
#endif

//    clk_disable_unprepare(priv->sensor_clk);

    return 0;
}

//static int imx_probe(struct i2c_client *adapter,
//                const struct i2c_device_id *device_id);
//static int imx_remove(struct i2c_client *client);


#ifdef CONFIG_OF
static const struct of_device_id vc_mipi_imx_of_match[] = {
    { .compatible = "sony,vc_mipi_imx" },
    { .compatible = "omnivision,vc_mipi_ov9281" },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, vc_mipi_imx_of_match);
#endif

static const struct i2c_device_id imx_id[] = {
    {"vc_mipi_imx", 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, imx_id);

static struct i2c_driver imx_i2c_driver = {
    .driver = {
          .owner = THIS_MODULE,
          .name  = "vc_mipi_imx",
#ifdef CONFIG_OF
          .of_match_table = of_match_ptr(vc_mipi_imx_of_match),
#endif
          },
    .probe  = imx_probe,
    .remove = imx_remove,
    .id_table = imx_id,
};
module_i2c_driver(imx_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("VC MIPI IMX Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");
