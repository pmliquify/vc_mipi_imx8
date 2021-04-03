#include "vc_mipi_param.h"
#include "vc_mipi_types.h"
#include "vc_mipi_modules.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_params.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

/*............. User control handler methods */
#define USER_CTRL_NONE        0   /* no user controls */
#define USER_CTRL_METHOD1     1   /* method 1 (old) */
#define USER_CTRL_METHOD2     2   /* method 2 (new) */

#define CTRL_HANDLER_METHOD   USER_CTRL_METHOD2

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

// static int sensor_mode = 0;         // VC MIPI sensor mode
static int flash_output  = 0;       // flash output enable

static struct imx_datafmt *imx_data_fmts = NULL;
static int                 imx_data_fmts_size = 0;

int imx_param_setup(struct vc_mipi_dev *sensor)
{
    // struct i2c_client *client = sensor->i2c_client;
    // struct device *dev = &client->dev;
    int ret = 0;

    TRACE

    switch(sensor->model) {
        case OV9281_MODEL_MONOCHROME:
        case OV9281_MODEL_COLOR:
            sensor->sen_pars.gain_min         = OV9281_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = OV9281_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = OV9281_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = OV9281_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = OV9281_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = OV9281_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = OV9281_DX;
            sensor->sen_pars.frame_dy         = OV9281_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)ov9281_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)ov9281_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)ov9281_mode_1280_800;

            if(sensor->model == OV9281_MODEL_MONOCHROME)
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

        case IMX183_MODEL_MONOCHROME:
        case IMX183_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX183_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX183_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX183_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX183_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX183_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX183_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX183_DX;
            sensor->sen_pars.frame_dy         = IMX183_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx183_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx183_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx183_mode_5440_3692;

            if(sensor->model == IMX183_MODEL_MONOCHROME)
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

        case IMX226_MODEL_MONOCHROME:
        case IMX226_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX226_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX226_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX226_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX226_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX226_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX226_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX226_DX;
            sensor->sen_pars.frame_dy         = IMX226_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx226_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx226_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx226_mode_3840_3040;

            if(sensor->model == IMX226_MODEL_MONOCHROME)
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

        case IMX250_MODEL_MONOCHROME:
        case IMX250_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX250_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX250_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX250_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX250_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX250_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX250_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX250_DX;
            sensor->sen_pars.frame_dy         = IMX250_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx250_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx250_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx250_mode_2432_2048;

            if(sensor->model == IMX250_MODEL_MONOCHROME)
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

        case IMX252_MODEL_MONOCHROME:
        case IMX252_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX252_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX252_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX252_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX252_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX252_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX252_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX252_DX;
            sensor->sen_pars.frame_dy         = IMX252_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx252_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx252_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx252_mode_2048_1536;

            if(sensor->model == IMX252_MODEL_MONOCHROME)
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

        case IMX273_MODEL_MONOCHROME:
        case IMX273_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX273_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX273_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX273_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX273_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX273_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX273_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX273_DX;
            sensor->sen_pars.frame_dy         = IMX273_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx273_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx273_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx273_mode_1440_1080;

            if(sensor->model == IMX273_MODEL_MONOCHROME)
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

        case IMX296_MODEL_MONOCHROME:
        case IMX296_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX296_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX296_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX296_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX296_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX296_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX296_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX296_DX;
            sensor->sen_pars.frame_dy         = IMX296_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx296_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx296_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx296_mode_1440_1080;

            if(sensor->model == IMX296_MODEL_MONOCHROME)
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

        case IMX297_MODEL_MONOCHROME:
        case IMX297_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX297_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX297_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX297_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX297_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX297_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX297_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX297_DX;
            sensor->sen_pars.frame_dy         = IMX297_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx297_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx297_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx297_mode_720_540;

            if(sensor->model == IMX297_MODEL_MONOCHROME)
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

        case IMX290_MODEL_MONOCHROME:
        case IMX290_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX290_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX290_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX290_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX290_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX290_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX290_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX290_DX;
            sensor->sen_pars.frame_dy         = IMX290_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx290_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx290_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx290_mode_1920_1080;

            if(sensor->model == IMX290_MODEL_MONOCHROME)
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

        case IMX327_MODEL_MONOCHROME:
        case IMX327_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX327_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX327_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX327_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX327_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX327_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX327_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX327_DX;
            sensor->sen_pars.frame_dy         = IMX327_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx327_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx327_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx327_mode_1920_1080;

            if(sensor->model == IMX327_MODEL_MONOCHROME)
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

        case IMX415_MODEL_MONOCHROME:
        case IMX415_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX415_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX415_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX415_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX415_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX415_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX415_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX415_DX;
            sensor->sen_pars.frame_dy         = IMX415_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx415_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx415_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx415_mode_3864_2192;

            if(sensor->model == IMX415_MODEL_MONOCHROME)
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

        case IMX412_MODEL_MONOCHROME:
        case IMX412_MODEL_COLOR:
            sensor->sen_pars.gain_min         = IMX412_DIGITAL_GAIN_MIN;
            sensor->sen_pars.gain_max         = IMX412_DIGITAL_GAIN_MAX;
            sensor->sen_pars.gain_default     = IMX412_DIGITAL_GAIN_DEFAULT;
            sensor->sen_pars.exposure_min     = IMX412_DIGITAL_EXPOSURE_MIN;
            sensor->sen_pars.exposure_max     = IMX412_DIGITAL_EXPOSURE_MAX;
            sensor->sen_pars.exposure_default = IMX412_DIGITAL_EXPOSURE_DEFAULT;
            sensor->sen_pars.frame_dx         = IMX412_DX;
            sensor->sen_pars.frame_dy         = IMX412_DY;

            sensor->sen_pars.sensor_start_table = (struct sensor_reg *)imx412_start;
            sensor->sen_pars.sensor_stop_table  = (struct sensor_reg *)imx412_stop;
            sensor->sen_pars.sensor_mode_table  = (struct sensor_reg *)imx412_mode_4032_3040;

            if(sensor->model == IMX412_MODEL_MONOCHROME)
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
    }

    sensor->pix.width  = sensor->sen_pars.frame_dx; // 640;
    sensor->pix.height = sensor->sen_pars.frame_dy; // 480;

    imx_valid_res[0].width  = sensor->sen_pars.frame_dx;
    imx_valid_res[0].height = sensor->sen_pars.frame_dy;

    sensor->digital_gain  = sensor->sen_pars.gain_default;
    sensor->exposure_time = sensor->sen_pars.exposure_default;

    sensor->sensor_ext_trig = 0;    // ext. trigger flag: 0=no, 1=yes

    if(sensor->model == IMX183_MODEL_MONOCHROME || sensor->model == IMX183_MODEL_COLOR)
    {
      sensor->sen_clk = 72000000;     // clock-frequency: default=54Mhz imx183=72Mhz
    }
    else
    {
      sensor->sen_clk = 54000000;     // clock-frequency: default=54Mhz imx183=72Mhz
    }

    sensor->flash_output = flash_output;
    sensor->streaming = false;
//    sensor->num_lanes = csi_num_lanes();

  return ret;

// /*............. Set user control ranges */
// #if CTRL_HANDLER_METHOD == USER_CTRL_METHOD2
//     ret = mx6s_set_ctrl_range (
//             V4L2_CID_GAIN,                  /* int id,             [in] control id            */
//             sensor->sen_pars.gain_min,        /* int minimum,        [in] minimum control value */
//             sensor->sen_pars.gain_max,        /* int maximum,        [in] minimum control value */
//             sensor->sen_pars.gain_default,    /* int default_value,  [in] default control value */
//             1);                             /* int step );         [in] control's step value  */
//     if(ret)
//     {
//       dev_err(dev, "[vc-mipi driver] %s: mx6s_set_ctrl_range() gain error=%d\n", __func__, ret);
//       goto done;
//     }

//     ret = mx6s_set_ctrl_range (
//             V4L2_CID_EXPOSURE,              /* int id,             [in] control id            */
//             sensor->sen_pars.exposure_min,    /* int minimum,        [in] minimum control value */
//             sensor->sen_pars.exposure_max,    /* int maximum,        [in] minimum control value */
//             sensor->sen_pars.exposure_default,/* int default_value,  [in] default control value */
//             1);                             /* int step );         [in] control's step value  */
//     if (ret) {
//       dev_err(dev, "[vc-mipi driver] %s: mx6s_set_ctrl_range() exp error=%d\n", __func__, ret);
//       goto done;
//     }
// #endif

// done:
//     return ret;
}