#ifndef _VC_MIPI_MODULES_H
#define _VC_MIPI_MODULES_H

#include "vc_mipi_types.h"

// OV9281 sensor modes:
//  0x00 : 10bit free-run streaming
//  0x01 : 8bit  free-run streaming
//  0x02 : 10bit external trigger
//  0x03 : 8bit  external trigger

/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

// IMX296,IMX297 sensor modes:
// 0 = 10bit free-run
// 1 = 10bit external trigger

// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

/*----------------------------------------------------------------------*/
/*                                  OV9281                              */
/*----------------------------------------------------------------------*/

#define OV9281_DIGITAL_GAIN_MIN     0x00
#define OV9281_DIGITAL_GAIN_MAX     0xfe
#define OV9281_DIGITAL_GAIN_DEFAULT 0x10

/* In usec */
#define OV9281_DIGITAL_EXPOSURE_MIN           8  //     8721 ns // 0x00000010 (16)       (16 * 8721)/16     =    8721
#define OV9281_DIGITAL_EXPOSURE_MAX        7719  //  7718085 ns // 0x00003750 (14160)    (14160 * 8721)/16  = 7718085
#define OV9281_DIGITAL_EXPOSURE_DEFAULT    2233  //  2232576 ns // 0x00001000 (4096)     (4096 * 8721)/16   = 2232576

#define OV9281_DX   1280
#define OV9281_DY   800

static const struct sensor_reg ov9281_start[] = {
    {0x0100, 0x01},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg ov9281_stop[] = {
    {0x0100, 0x00},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};

/* MCLK:25MHz  1280x800  120fps   MIPI LANE2 */
static const struct sensor_reg ov9281_mode_1280_800[] = {
    {SENSOR_TABLE_END, 0x00}
};

// OV9281 - mono formats
static struct imx_datafmt ov9281_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
};
static int ov9281_mono_fmts_size = ARRAY_SIZE(ov9281_mono_fmts);

// OV9281 - color formats
static struct imx_datafmt ov9281_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int ov9281_color_fmts_size = ARRAY_SIZE(ov9281_color_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX183                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

/* In dB*10 */
#define IMX183_DIGITAL_GAIN_MIN        0x00
#define IMX183_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX183_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX183_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX183_DIGITAL_EXPOSURE_MAX     10000000
#define IMX183_DIGITAL_EXPOSURE_DEFAULT 10000

#if 0
  #define IMX183_DX 5440
  #define IMX183_DY 3692
#else    // test mode
  #define IMX183_DX 1920    // IMX327
  #define IMX183_DY 1080
//  #define IMX183_DX 1440    // IMX273
//  #define IMX183_DY 1080
//  #define IMX183_DX 1280    // OV9281
//  #define IMX183_DY 800
#endif

static const struct sensor_reg imx183_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx183_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx183_mode_5440_3692[] = {
#if 1
    { 0x6015, IMX183_DX & 0xFF }, { 0x6016, (IMX183_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX183_DY & 0xFF }, { 0x6011, (IMX183_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX183 - mono formats
static struct imx_datafmt imx183_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
};
static int imx183_mono_fmts_size = ARRAY_SIZE(imx183_mono_fmts);

// IMX183 - color formats
static struct imx_datafmt imx183_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx183_color_fmts_size = ARRAY_SIZE(imx183_color_fmts);


/*----------------------------------------------------------------------*/
/*                                  IMX226                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

/* In dB*10 */
#define IMX226_DIGITAL_GAIN_MIN        0x00
#define IMX226_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX226_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX226_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX226_DIGITAL_EXPOSURE_MAX     10000000
#define IMX226_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
  #define IMX226_DX 3840
  #define IMX226_DY 3040
#else    // test mode
  #define IMX226_DX 1440    // IMX273
  #define IMX226_DY 1080
//#define IMX226_DX 1920    // IMX327
//#define IMX226_DY 1080
//#define IMX226_DX 1280    // OV9281
//#define IMX226_DY 800
#endif

static const struct sensor_reg imx226_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx226_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
/* MCLK:25MHz  3840 x 3040   MIPI LANE2 */
static const struct sensor_reg imx226_mode_3840_3040[] = {
#if 1
    { 0x6015, IMX226_DX & 0xFF }, { 0x6016, (IMX226_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX226_DY & 0xFF }, { 0x6011, (IMX226_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX226 - mono formats
static struct imx_datafmt imx226_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
};
static int imx226_mono_fmts_size = ARRAY_SIZE(imx226_mono_fmts);

// IMX226 - color formats
static struct imx_datafmt imx226_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx226_color_fmts_size = ARRAY_SIZE(imx226_color_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX250                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

/* In dB*10 */
#define IMX250_DIGITAL_GAIN_MIN        0x00
#define IMX250_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX250_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX250_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX250_DIGITAL_EXPOSURE_MAX     10000000
#define IMX250_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
  #define IMX250_DX 2432
  #define IMX250_DY 2048
#else    // test mode
  #define IMX250_DX 1440    // IMX273
  #define IMX250_DY 1080
//#define IMX250_DX 1920    // IMX327
//#define IMX250_DY 1080
//#define IMX250_DX 1280    // OV9281
//#define IMX250_DY 800
#endif

static const struct sensor_reg imx250_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx250_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx250_mode_2432_2048[] = {
#if 0
    { 0x6015, IMX250_DX & 0xFF }, { 0x6016, (IMX250_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX250_DY & 0xFF }, { 0x6011, (IMX250_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX250 - mono formats
static struct imx_datafmt imx250_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
};
static int imx250_mono_fmts_size = ARRAY_SIZE(imx250_mono_fmts);

// IMX250 - color formats
static struct imx_datafmt imx250_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx250_color_fmts_size = ARRAY_SIZE(imx250_color_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX252                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

/* In dB*10 */
#define IMX252_DIGITAL_GAIN_MIN        0x00
#define IMX252_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX252_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX252_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX252_DIGITAL_EXPOSURE_MAX     10000000
#define IMX252_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
  #define IMX252_DX 2048
  #define IMX252_DY 1536
#else    // test mode
  #define IMX252_DX 1440    // IMX273
  #define IMX252_DY 1080
#endif

static const struct sensor_reg imx252_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx252_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx252_mode_2048_1536[] = {
#if 0
    { 0x6015, IMX252_DX & 0xFF }, { 0x6016, (IMX252_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX252_DY & 0xFF }, { 0x6011, (IMX252_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX252 - mono formats
static struct imx_datafmt imx252_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
};
static int imx252_mono_fmts_size = ARRAY_SIZE(imx252_mono_fmts);

// IMX252 - color formats
static struct imx_datafmt imx252_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx252_color_fmts_size = ARRAY_SIZE(imx252_color_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX273                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

// imx296, 297, 273:
///* In dB*10 */
#define IMX273_DIGITAL_GAIN_MIN     0
#define IMX273_DIGITAL_GAIN_MAX     480
#define IMX273_DIGITAL_GAIN_DEFAULT 20

/* In usec */
#define IMX273_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX273_DIGITAL_EXPOSURE_MAX     15110711
#define IMX273_DIGITAL_EXPOSURE_DEFAULT 10000

#define IMX273_DX 1440
#define IMX273_DY 1080

static const struct sensor_reg imx273_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx273_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
static const struct sensor_reg imx273_mode_1440_1080[] = {
#if 1
    { 0x6015, IMX273_DX & 0xFF }, { 0x6016, (IMX273_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX273_DY & 0xFF }, { 0x6011, (IMX273_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX273 - color formats
static struct imx_datafmt imx273_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx273_color_fmts_size = ARRAY_SIZE(imx273_color_fmts);

// IMX273 - mono formats
static struct imx_datafmt imx273_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
    { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
//    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
//    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx273_mono_fmts_size = ARRAY_SIZE(imx273_mono_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX296                              */
/*----------------------------------------------------------------------*/
// IMX296,IMX297 sensor modes:
// 0 = 10bit free-run
// 1 = 10bit external trigger

/* In dB*10 */
#define IMX296_DIGITAL_GAIN_MIN     0
#define IMX296_DIGITAL_GAIN_MAX     480
#define IMX296_DIGITAL_GAIN_DEFAULT 20

/* In usec */
#define IMX296_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX296_DIGITAL_EXPOSURE_MAX     15534389
#define IMX296_DIGITAL_EXPOSURE_DEFAULT 2000

#define IMX296_DX 1440
#define IMX296_DY 1080

static const struct sensor_reg imx296_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x300A, 0x00},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx296_stop[] = {
    {0x300A, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};
// M.Engel: 0x4183,0x4182 = Vertical direction effective pixels (H,L) for IMX296, IMX297
static const struct sensor_reg imx296_mode_1440_1080[] = {
//    { 0x4183, IMX296_DY>>8 }, { 0x4182, IMX296_DY & 0xFF },   //  0x4183,0x4182 = Vertical direction effective pixels (H,L) for IMX296, IMX297
    { 0x3300, 0x01 },
//    { 0x3316, 0x38 },  // 0x38 : 1080, 0x40 : 1088
//    { 0x3317, 0x04 },
    { 0xaf,   0x0B },
  #if 0
    { 0x3311, X0>>8 }, { 0x3310, X0 & 0xFF },                 // horizontal start H,L   0x3311, 0x3310 = X0
    { 0x3313, Y0>>8 }, { 0x3312, Y0 & 0xFF },                 // vertical start H,L     0x3313, 0x3312 = Y0
    { 0x3315, IMX296_DX>>8 }, { 0x3314, IMX296_DX & 0xFF },   // hor. output width H,L  0x3315, 0x3314 = DX
    { 0x3317, IMX296_DY>>8 }, { 0x3316, IMX296_DY & 0xFF },   // ver. output height H,L 0x3317, 0x3316 = DY
  #endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX296 - color formats
static struct imx_datafmt imx296_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx296_color_fmts_size = ARRAY_SIZE(imx296_color_fmts);

// IMX296 - mono formats
static struct imx_datafmt imx296_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx296_mono_fmts_size = ARRAY_SIZE(imx296_mono_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX297                              */
/*----------------------------------------------------------------------*/
// IMX296,IMX297 sensor modes:
// 0 = 10bit free-run
// 1 = 10bit external trigger

/* In dB*10 */
#define IMX297_DIGITAL_GAIN_MIN     0
#define IMX297_DIGITAL_GAIN_MAX     480
#define IMX297_DIGITAL_GAIN_DEFAULT 20

/* In usec */
#define IMX297_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX297_DIGITAL_EXPOSURE_MAX     15110711
#define IMX297_DIGITAL_EXPOSURE_DEFAULT 2000

#define IMX297_DX  720
#define IMX297_DY  540

static const struct sensor_reg imx297_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x300A, 0x00},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx297_stop[] = {
    {0x300A, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};

// M.Engel: 0x4183,0x4182 = Vertical direction effective pixels (H,L) for IMX296, IMX297

#define X0 0
#define Y0 0
static const struct sensor_reg imx297_mode_720_540[] = {
#if 1
    { 0x3311, X0>>8 }, { 0x3310, X0 & 0xFF },                 // horizontal start H,L   0x3311, 0x3310 = X0
    { 0x3313, Y0>>8 }, { 0x3312, Y0 & 0xFF },                 // vertical start H,L     0x3313, 0x3312 = Y0
    { 0x3315, IMX297_DX>>8 }, { 0x3314, IMX297_DX & 0xFF },   // hor. output width H,L  0x3315, 0x3314 = DX
    { 0x3317, IMX297_DY>>8 }, { 0x3316, IMX297_DY & 0xFF },   // ver. output height H,L 0x3317, 0x3316 = DY
    { 0x4183, IMX297_DY>>8 }, { 0x4182, IMX297_DY & 0xFF },   //  0x4183,0x4182 = Vertical direction effective pixels (H,L) for IMX296, IMX297
#endif
    { 0x3300, 0x01 },
//    { 0x3316, 0x38 },  // 0x38 : 1080, 0x40 : 1088
//    { 0x3317, 0x04 },
    { 0xaf,   0x0B },
    {SENSOR_TABLE_END, 0x00}
};
#undef  X0
#undef  Y0

// IMX297 - color formats
static struct imx_datafmt imx297_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx297_color_fmts_size = ARRAY_SIZE(imx297_color_fmts);

// IMX297 - mono formats
static struct imx_datafmt imx297_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx297_mono_fmts_size = ARRAY_SIZE(imx297_mono_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX290                              */
/*----------------------------------------------------------------------*/
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

#define IMX290_DIGITAL_GAIN_MIN        0
#define IMX290_DIGITAL_GAIN_MAX        240
#define IMX290_DIGITAL_GAIN_DEFAULT    0

/* In usec */
#define IMX290_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX290_DIGITAL_EXPOSURE_MAX     7767184
#define IMX290_DIGITAL_EXPOSURE_DEFAULT 10000

#define IMX290_DX   1920
#define IMX290_DY   1080

static const struct sensor_reg imx290_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x3002, 0x00},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx290_stop[] = {
    {0x3002, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx290_mode_1920_1080[] = {
    { 0x3000, 0x1 },            // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby

    { 0x3473, IMX290_DX>>8 }, { 0x3472, IMX290_DX & 0xFF },   // X_OUT_SIZE[12:0] H,L = 0x3473,0x3472 = Horizontal (H) direction effective pixel width setting.
    { 0x3419, IMX290_DY>>8 }, { 0x3418, IMX290_DY & 0xFF },   // Y_OUT_SIZE[12:0] H,L = 0x3419,0x3418 = Vertical direction effective pixels

    { 0x3000, 0x0 },                // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby
    {SENSOR_TABLE_END, 0x00}
};

// IMX290 - color formats
static struct imx_datafmt imx290_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx290_color_fmts_size = ARRAY_SIZE(imx290_color_fmts);

// IMX290 - mono formats
static struct imx_datafmt imx290_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx290_mono_fmts_size = ARRAY_SIZE(imx290_mono_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX327                              */
/*----------------------------------------------------------------------*/
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

#define IMX327_DIGITAL_GAIN_MIN        0
#define IMX327_DIGITAL_GAIN_MAX        240
#define IMX327_DIGITAL_GAIN_DEFAULT    0

/* In usec */
#define IMX327_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX327_DIGITAL_EXPOSURE_MAX     7767184
#define IMX327_DIGITAL_EXPOSURE_DEFAULT 10000

#define IMX327_DX   1920
#define IMX327_DY   1080

static const struct sensor_reg imx327_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x3002, 0x00},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx327_stop[] = {
    {0x3002, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx327_mode_1920_1080[] = {
    { 0x3000, 0x1 },            // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby

    { 0x3473, IMX327_DX>>8 }, { 0x3472, IMX327_DX & 0xFF },   // X_OUT_SIZE[12:0] H,L = 0x3473,0x3472 = Horizontal (H) direction effective pixel width setting.
    { 0x3419, IMX327_DY>>8 }, { 0x3418, IMX327_DY & 0xFF },   // Y_OUT_SIZE[12:0] H,L = 0x3419,0x3418 = Vertical direction effective pixels

    { 0x3000, 0x0 },                // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby
    {SENSOR_TABLE_END, 0x00}
};

// IMX327 - color formats
static struct imx_datafmt imx327_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx327_color_fmts_size = ARRAY_SIZE(imx327_color_fmts);

// IMX327 - mono formats
static struct imx_datafmt imx327_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx327_mono_fmts_size = ARRAY_SIZE(imx327_mono_fmts);


/*----------------------------------------------------------------------*/
/*                                  IMX412                              */
/*----------------------------------------------------------------------*/
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

#define IMX412_DIGITAL_GAIN_MIN         0x100
#define IMX412_DIGITAL_GAIN_MAX         0xFFF
#define IMX412_DIGITAL_GAIN_DEFAULT     0x100

/* In usec */
#define IMX412_DIGITAL_EXPOSURE_MIN     22    // microseconds (us)
#define IMX412_DIGITAL_EXPOSURE_MAX     92693
#define IMX412_DIGITAL_EXPOSURE_DEFAULT 5000

#if 1
  #define IMX412_DX 4032
  #define IMX412_DY 3040
#else
  #define IMX412_DX 1920
  #define IMX412_DY 1080
#endif

static const struct sensor_reg imx412_start[] = {
    { 0x0100, 0x01 },
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx412_stop[] = {
    { 0x0100, 0x00 },
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx412_mode_4032_3040[] = {
#if 1
    { 0x034C, IMX412_DX>>8 }, { 0x034D, IMX412_DX & 0xFF },   // hor. output width H,L      0x034C,0x034D  = DX
    { 0x034E, IMX412_DY>>8 }, { 0x034F, IMX412_DY & 0xFF },   // ver. output height H,L     0x034E,0x034F  = DY
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX412 - color formats
static struct imx_datafmt imx412_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx412_color_fmts_size = ARRAY_SIZE(imx412_color_fmts);

// IMX412 - mono formats
static struct imx_datafmt imx412_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx412_mono_fmts_size = ARRAY_SIZE(imx412_mono_fmts);

/*----------------------------------------------------------------------*/
/*                                  IMX415                              */
/*----------------------------------------------------------------------*/
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

#define IMX415_DIGITAL_GAIN_MIN        0       // 0x00
#define IMX415_DIGITAL_GAIN_MAX        240     // 0xF0
#define IMX415_DIGITAL_GAIN_DEFAULT    0

/* In usec */
#define IMX415_DIGITAL_EXPOSURE_MIN     56          // microseconds (us)
#define IMX415_DIGITAL_EXPOSURE_MAX     2097152     // 92693
#define IMX415_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
#define IMX415_DX 3864
#define IMX415_DY 2192
#else
#define IMX415_DX 1920
#define IMX415_DY 1080
#endif

static const struct sensor_reg imx415_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x3002, 0x00},     /* mode select streaming on */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx415_stop[] = {
    {0x3002, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {SENSOR_TABLE_END, 0x00}
};

static const struct sensor_reg imx415_mode_3864_2192[] = {
#if 1
    { 0x3042, IMX415_DX & 0xFF }, { 0x3043, IMX415_DX>>8 },           // hor. output width H,L  0x3043,0x3042 = DX
    { 0x3046, (IMX415_DY*2) & 0xFF }, { 0x3047, (IMX415_DY*2)>>8 },   // ver. output height H,L 0x3047,0x3046 = DY
#endif
    {SENSOR_TABLE_END, 0x00}
};

// IMX415 - color formats
static struct imx_datafmt imx415_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SGBRG10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SGBRG10 = 'GB10' */
};
static int imx415_color_fmts_size = ARRAY_SIZE(imx415_color_fmts);

// IMX415 - mono formats
static struct imx_datafmt imx415_mono_fmts[] = {
    { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
};
static int imx415_mono_fmts_size = ARRAY_SIZE(imx415_mono_fmts);

#endif
