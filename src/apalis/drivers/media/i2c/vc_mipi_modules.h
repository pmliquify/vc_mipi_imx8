#ifndef _VC_MIPI_MOPDULES_H
#define _VC_MIPI_MOPDULES_H

#include "vc_mipi_core.h"

// /*............. Sensor models */
// enum sen_model {
//     SEN_MODEL_UNKNOWN = 0,
//     OV7251_MODEL_MONOCHROME,
//     OV7251_MODEL_COLOR,
// //    OV9281_MODEL,
//     OV9281_MODEL_MONOCHROME,
//     OV9281_MODEL_COLOR,

//     IMX183_MODEL_MONOCHROME,
//     IMX183_MODEL_COLOR,
//     IMX226_MODEL_MONOCHROME,
//     IMX226_MODEL_COLOR,
//     IMX250_MODEL_MONOCHROME,
//     IMX250_MODEL_COLOR,
//     IMX252_MODEL_MONOCHROME,
//     IMX252_MODEL_COLOR,
//     IMX273_MODEL_MONOCHROME,
//     IMX273_MODEL_COLOR,

//     IMX290_MODEL_MONOCHROME,
//     IMX290_MODEL_COLOR,
//     IMX296_MODEL_MONOCHROME,
//     IMX296_MODEL_COLOR,
//     IMX297_MODEL_MONOCHROME,
//     IMX297_MODEL_COLOR,

//     IMX327C_MODEL,
//     IMX327_MODEL_MONOCHROME,
//     IMX327_MODEL_COLOR,
//     IMX412_MODEL_MONOCHROME,
//     IMX412_MODEL_COLOR,
//     IMX415_MODEL_MONOCHROME,
//     IMX415_MODEL_COLOR,
// };

/*----------------------------------------------------------------------*/
/*   IMX226                                                             */
/*----------------------------------------------------------------------*/
#define SENSOR_TABLE_END        0xffff

// /* In dB*10 */
// #define IMX226_DIGITAL_GAIN_MIN        0x00
// #define IMX226_DIGITAL_GAIN_MAX        0x7A5     // 1957
// #define IMX226_DIGITAL_GAIN_DEFAULT    0x10

// /* In usec */
// #define IMX226_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
// #define IMX226_DIGITAL_EXPOSURE_MAX     10000000
// #define IMX226_DIGITAL_EXPOSURE_DEFAULT 10000

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
// static struct vc_datafmt imx226_mono_fmts[] = {
//     { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
//     { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
//     { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
//     { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// // use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
// };
// static int imx226_mono_fmts_size = ARRAY_SIZE(imx226_mono_fmts);

// IMX226 - color formats
// static struct vc_datafmt imx226_color_fmts[] = {
//     { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
//     { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
//     { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
// };
// static int imx226_color_fmts_size = ARRAY_SIZE(imx226_color_fmts);
/*----------------------------------------------------------------------*/

#endif // _VC_MIPI_MOPDULES_H