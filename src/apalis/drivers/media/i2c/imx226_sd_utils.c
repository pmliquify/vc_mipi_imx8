#include "imx226_sd_utils.h"
#include "imx226_utils.h"
#include "imx226_i2c.h"

#define TRACE printk("        TRACE [vc-mipi] imx226_sd_utils.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

int imx226_set_power(struct imx226_dev *sensor, bool on)
{
	int ret = 0;

	TRACE
	
	if (on) {
		ret = imx226_set_power_on(sensor);
		if (ret)
			return ret;

		ret = imx226_restore_mode(sensor);
		if (ret)
			goto power_off;
	}

	if (sensor->ep.bus_type == V4L2_MBUS_CSI2_DPHY)
		ret = imx226_set_power_mipi(sensor, on);
	else
		ret = imx226_set_power_dvp(sensor, on);
	if (ret)
		goto power_off;

	if (!on)
		imx226_set_power_off(sensor);

	return 0;

power_off:
	imx226_set_power_off(sensor);
	return ret;
}

int imx226_try_frame_interval(struct imx226_dev *sensor,
				     struct v4l2_fract *fi,
				     u32 width, u32 height)
{
	const struct imx226_mode_info *mode;
	enum imx226_frame_rate rate = imx226_08_FPS;
	int minfps, maxfps, best_fps, fps;
	int i;

	TRACE
	
	minfps = imx226_framerates[imx226_08_FPS];
	maxfps = imx226_framerates[imx226_60_FPS];

	if (fi->numerator == 0) {
		fi->denominator = maxfps;
		fi->numerator = 1;
		rate = imx226_60_FPS;
		goto find_mode;
	}

	fps = clamp_val(DIV_ROUND_CLOSEST(fi->denominator, fi->numerator),
			minfps, maxfps);

	best_fps = minfps;
	for (i = 0; i < ARRAY_SIZE(imx226_framerates); i++) {
		int curr_fps = imx226_framerates[i];

		if (abs(curr_fps - fps) < abs(best_fps - fps)) {
			best_fps = curr_fps;
			rate = i;
		}
	}

	fi->numerator = 1;
	fi->denominator = best_fps;

find_mode:
	mode = imx226_find_mode(sensor, rate, width, height, false);
	return mode ? rate : -EINVAL;
}

int imx226_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   enum imx226_frame_rate fr,
				   const struct imx226_mode_info **new_mode)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	const struct imx226_mode_info *mode;
	int i;

	TRACE
	
	mode = imx226_find_mode(sensor, fr, fmt->width, fmt->height, true);
	if (!mode)
		return -EINVAL;
	fmt->width = mode->hact;
	fmt->height = mode->vact;
	memset(fmt->reserved, 0, sizeof(fmt->reserved));

	if (new_mode)
		*new_mode = mode;

	for (i = 0; i < ARRAY_SIZE(imx226_formats); i++)
		if (imx226_formats[i].code == fmt->code)
			break;
	if (i >= ARRAY_SIZE(imx226_formats))
		i = 0;

	fmt->code = imx226_formats[i].code;
	fmt->colorspace = imx226_formats[i].colorspace;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);

	return 0;
}

int imx226_set_framefmt(struct imx226_dev *sensor,
			       struct v4l2_mbus_framefmt *format)
{
	int ret = 0;
	bool is_jpeg = false;
	u8 fmt, mux;

	TRACE
	
	switch (format->code) {
	case MEDIA_BUS_FMT_UYVY8_2X8:
		/* YUV422, UYVY */
		fmt = 0x3f;
		mux = IMX226_FMT_MUX_YUV422;
		break;
	case MEDIA_BUS_FMT_YUYV8_2X8:
		/* YUV422, YUYV */
		fmt = 0x30;
		mux = IMX226_FMT_MUX_YUV422;
		break;
	case MEDIA_BUS_FMT_RGB565_2X8_LE:
		/* RGB565 {g[2:0],b[4:0]},{r[4:0],g[5:3]} */
		fmt = 0x6F;
		mux = IMX226_FMT_MUX_RGB;
		break;
	case MEDIA_BUS_FMT_RGB565_2X8_BE:
		/* RGB565 {r[4:0],g[5:3]},{g[2:0],b[4:0]} */
		fmt = 0x61;
		mux = IMX226_FMT_MUX_RGB;
		break;
	case MEDIA_BUS_FMT_JPEG_1X8:
		/* YUV422, YUYV */
		fmt = 0x30;
		mux = IMX226_FMT_MUX_YUV422;
		is_jpeg = true;
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		/* Raw, BGBG... / GRGR... */
		fmt = 0x00;
		mux = IMX226_FMT_MUX_RAW_DPC;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		/* Raw bayer, GBGB... / RGRG... */
		fmt = 0x01;
		mux = IMX226_FMT_MUX_RAW_DPC;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		/* Raw bayer, GRGR... / BGBG... */
		fmt = 0x02;
		mux = IMX226_FMT_MUX_RAW_DPC;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		/* Raw bayer, RGRG... / GBGB... */
		fmt = 0x03;
		mux = IMX226_FMT_MUX_RAW_DPC;
		break;
	default:
		return -EINVAL;
	}

	/* FORMAT CONTROL00: YUV and RGB formatting */
	ret = imx226_write_reg(sensor, IMX226_REG_FORMAT_CONTROL00, fmt);
	if (ret)
		return ret;

	/* FORMAT MUX CONTROL: ISP YUV or RGB */
	ret = imx226_write_reg(sensor, IMX226_REG_ISP_FORMAT_MUX_CTRL, mux);
	if (ret)
		return ret;

	/*
	 * TIMING TC REG21:
	 * - [5]:	JPEG enable
	 */
	ret = imx226_mod_reg(sensor, IMX226_REG_TIMING_TC_REG21,
			     BIT(5), is_jpeg ? BIT(5) : 0);
	if (ret)
		return ret;

	/*
	 * SYSTEM RESET02:
	 * - [4]:	Reset JFIFO
	 * - [3]:	Reset SFIFO
	 * - [2]:	Reset JPEG
	 */
	ret = imx226_mod_reg(sensor, IMX226_REG_SYS_RESET02,
			     BIT(4) | BIT(3) | BIT(2),
			     is_jpeg ? 0 : (BIT(4) | BIT(3) | BIT(2)));
	if (ret)
		return ret;

	/*
	 * CLOCK ENABLE02:
	 * - [5]:	Enable JPEG 2x clock
	 * - [3]:	Enable JPEG clock
	 */
	return imx226_mod_reg(sensor, IMX226_REG_SYS_CLOCK_ENABLE02,
			      BIT(5) | BIT(3),
			      is_jpeg ? (BIT(5) | BIT(3)) : 0);
}

/*
 * Sensor Controls.
 */

int imx226_set_ctrl_hue(struct imx226_dev *sensor, int value)
{
	int ret;

	TRACE
	
	if (value) {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0,
				     BIT(0), BIT(0));
		if (ret)
			return ret;
		ret = imx226_write_reg16(sensor, IMX226_REG_SDE_CTRL1, value);
	} else {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0, BIT(0), 0);
	}

	return ret;
}

int imx226_set_ctrl_contrast(struct imx226_dev *sensor, int value)
{
	int ret;

	TRACE
	
	if (value) {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0,
				     BIT(2), BIT(2));
		if (ret)
			return ret;
		ret = imx226_write_reg(sensor, IMX226_REG_SDE_CTRL5,
				       value & 0xff);
	} else {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0, BIT(2), 0);
	}

	return ret;
}

int imx226_set_ctrl_saturation(struct imx226_dev *sensor, int value)
{
	int ret;

	TRACE
	
	if (value) {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0,
				     BIT(1), BIT(1));
		if (ret)
			return ret;
		ret = imx226_write_reg(sensor, IMX226_REG_SDE_CTRL3,
				       value & 0xff);
		if (ret)
			return ret;
		ret = imx226_write_reg(sensor, IMX226_REG_SDE_CTRL4,
				       value & 0xff);
	} else {
		ret = imx226_mod_reg(sensor, IMX226_REG_SDE_CTRL0, BIT(1), 0);
	}

	return ret;
}

int imx226_set_ctrl_white_balance(struct imx226_dev *sensor, int awb)
{
	int ret;

	TRACE
	
	ret = imx226_mod_reg(sensor, IMX226_REG_AWB_MANUAL_CTRL,
			     BIT(0), awb ? 0 : 1);
	if (ret)
		return ret;

	if (!awb) {
		u16 red = (u16)sensor->ctrls.red_balance->val;
		u16 blue = (u16)sensor->ctrls.blue_balance->val;

		ret = imx226_write_reg16(sensor, IMX226_REG_AWB_R_GAIN, red);
		if (ret)
			return ret;
		ret = imx226_write_reg16(sensor, IMX226_REG_AWB_B_GAIN, blue);
	}

	return ret;
}

int imx226_set_ctrl_exposure(struct imx226_dev *sensor,
				    enum v4l2_exposure_auto_type auto_exposure)
{
	struct imx226_ctrls *ctrls = &sensor->ctrls;
	bool auto_exp = (auto_exposure == V4L2_EXPOSURE_AUTO);
	int ret = 0;

	TRACE
	
	if (ctrls->auto_exp->is_new) {
		ret = imx226_set_autoexposure(sensor, auto_exp);
		if (ret)
			return ret;
	}

	if (!auto_exp && ctrls->exposure->is_new) {
		u16 max_exp;

		ret = imx226_read_reg16(sensor, IMX226_REG_AEC_PK_VTS,
					&max_exp);
		if (ret)
			return ret;
		ret = imx226_get_vts(sensor);
		if (ret < 0)
			return ret;
		max_exp += ret;
		ret = 0;

		if (ctrls->exposure->val < max_exp)
			ret = imx226_set_exposure(sensor, ctrls->exposure->val);
	}

	return ret;
}

int imx226_set_ctrl_gain(struct imx226_dev *sensor, bool auto_gain)
{
	struct imx226_ctrls *ctrls = &sensor->ctrls;
	int ret = 0;

	TRACE
	
	if (ctrls->auto_gain->is_new) {
		ret = imx226_set_autogain(sensor, auto_gain);
		if (ret)
			return ret;
	}

	if (!auto_gain && ctrls->gain->is_new)
		ret = imx226_set_gain(sensor, ctrls->gain->val);

	return ret;
}

#define imx226_TEST_ENABLE		BIT(7)
#define imx226_TEST_ROLLING		BIT(6)	/* rolling horizontal bar */
#define imx226_TEST_TRANSPARENT		BIT(5)
#define imx226_TEST_SQUARE_BW		BIT(4)	/* black & white squares */
#define imx226_TEST_BAR_STANDARD	(0 << 2)
#define imx226_TEST_BAR_VERT_CHANGE_1	(1 << 2)
#define imx226_TEST_BAR_HOR_CHANGE	(2 << 2)
#define imx226_TEST_BAR_VERT_CHANGE_2	(3 << 2)
#define imx226_TEST_BAR			(0 << 0)
#define imx226_TEST_RANDOM		(1 << 0)
#define imx226_TEST_SQUARE		(2 << 0)
#define imx226_TEST_BLACK		(3 << 0)


const u8 test_pattern_val[] = {
	0,
	imx226_TEST_ENABLE | imx226_TEST_BAR_VERT_CHANGE_1 |
		imx226_TEST_BAR,
	imx226_TEST_ENABLE | imx226_TEST_ROLLING |
		imx226_TEST_BAR_VERT_CHANGE_1 | imx226_TEST_BAR,
	imx226_TEST_ENABLE | imx226_TEST_SQUARE,
	imx226_TEST_ENABLE | imx226_TEST_ROLLING | imx226_TEST_SQUARE,
};

int imx226_set_ctrl_test_pattern(struct imx226_dev *sensor, int value)
{
	TRACE
	
	return imx226_write_reg(sensor, IMX226_REG_PRE_ISP_TEST_SET1,
				test_pattern_val[value]);
}

int imx226_set_ctrl_light_freq(struct imx226_dev *sensor, int value)
{
	int ret;

	TRACE
	
	ret = imx226_mod_reg(sensor, IMX226_REG_HZ5060_CTRL01, BIT(7),
			     (value == V4L2_CID_POWER_LINE_FREQUENCY_AUTO) ?
			     0 : BIT(7));
	if (ret)
		return ret;

	return imx226_mod_reg(sensor, IMX226_REG_HZ5060_CTRL00, BIT(2),
			      (value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ) ?
			      BIT(2) : 0);
}

int imx226_set_ctrl_hflip(struct imx226_dev *sensor, int value)
{
	TRACE
	
	/*
	 * If sensor is mounted upside down, mirror logic is inversed.
	 *
	 * Sensor is a BSI (Back Side Illuminated) one,
	 * so image captured is physically mirrored.
	 * This is why mirror logic is inversed in
	 * order to cancel this mirror effect.
	 */

	/*
	 * TIMING TC REG21:
	 * - [2]:	ISP mirror
	 * - [1]:	Sensor mirror
	 */
	return imx226_mod_reg(sensor, IMX226_REG_TIMING_TC_REG21,
			      BIT(2) | BIT(1),
			      (!(value ^ sensor->upside_down)) ?
			      (BIT(2) | BIT(1)) : 0);
}

int imx226_set_ctrl_vflip(struct imx226_dev *sensor, int value)
{
	TRACE
	
	/* If sensor is mounted upside down, flip logic is inversed */

	/*
	 * TIMING TC REG20:
	 * - [2]:	ISP vflip
	 * - [1]:	Sensor vflip
	 */
	return imx226_mod_reg(sensor, IMX226_REG_TIMING_TC_REG20,
			      BIT(2) | BIT(1),
			      (value ^ sensor->upside_down) ?
			      (BIT(2) | BIT(1)) : 0);
}

const struct imx226_mode_info *
imx226_find_mode(struct imx226_dev *sensor, enum imx226_frame_rate fr,
		 int width, int height, bool nearest)
{
	const struct imx226_mode_info *mode;

	TRACE
	
	mode = v4l2_find_nearest_size(imx226_mode_data,
				      ARRAY_SIZE(imx226_mode_data),
				      hact, vact,
				      width, height);

	if (!mode ||
	    (!nearest && (mode->hact != width || mode->vact != height)))
		return NULL;

	return mode;
}

int imx226_set_stream_mipi(struct imx226_dev *sensor, bool on)
{
	int ret;

	TRACE

	/*
	 * Enable/disable the MIPI interface
	 *
	 * 0x300e = on ? 0x45 : 0x40
	 *
	 * FIXME: the sensor manual (version 2.03) reports
	 * [7:5] = 000  : 1 data lane mode
	 * [7:5] = 001  : 2 data lanes mode
	 * But this settings do not work, while the following ones
	 * have been validated for 2 data lanes mode.
	 *
	 * [7:5] = 010	: 2 data lanes mode
	 * [4] = 0	: Power up MIPI HS Tx
	 * [3] = 0	: Power up MIPI LS Rx
	 * [2] = 1/0	: MIPI interface enable/disable
	 * [1:0] = 01/00: FIXME: 'debug'
	 */
	ret = imx226_write_reg(sensor, IMX226_REG_IO_MIPI_CTRL00,
			       on ? 0x45 : 0x40);
	if (ret)
		return ret;

	ret = imx226_write_reg(sensor, IMX226_REG_FRAME_CTRL01,
				on ? 0x00 : 0x0f);
	if (ret)
		return ret;

	ret = imx226_write_reg(sensor, IMX226_REG_SYS_CTRL0,
				on ? 0x02 : 0x42);
	if (ret)
		return ret;

	msleep(100);
	return ret;
}

int imx226_set_stream_dvp(struct imx226_dev *sensor, bool on)
{
	int ret;
	unsigned int flags = sensor->ep.bus.parallel.flags;
	u8 pclk_pol = 0;
	u8 hsync_pol = 0;
	u8 vsync_pol = 0;

	TRACE

	/*
	 * Note about parallel port configuration.
	 *
	 * When configured in parallel mode, the imx226 will
	 * output 10 bits data on DVP data lines [9:0].
	 * If only 8 bits data are wanted, the 8 bits data lines
	 * of the camera interface must be physically connected
	 * on the DVP data lines [9:2].
	 *
	 * Control lines polarity can be configured through
	 * devicetree endpoint control lines properties.
	 * If no endpoint control lines properties are set,
	 * polarity will be as below:
	 * - VSYNC:	active high
	 * - HREF:	active low
	 * - PCLK:	active low
	 */

	if (on) {
		/*
		 * configure parallel port control lines polarity
		 *
		 * POLARITY CTRL0
		 * - [5]:	PCLK polarity (0: active low, 1: active high)
		 * - [1]:	HREF polarity (0: active low, 1: active high)
		 * - [0]:	VSYNC polarity (mismatch here between
		 *		datasheet and hardware, 0 is active high
		 *		and 1 is active low...)
		 */
		if (flags & V4L2_MBUS_PCLK_SAMPLE_RISING)
			pclk_pol = 1;
		if (flags & V4L2_MBUS_HSYNC_ACTIVE_HIGH)
			hsync_pol = 1;
		if (flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)
			vsync_pol = 1;

		ret = imx226_write_reg(sensor,
				       IMX226_REG_POLARITY_CTRL00,
				       (pclk_pol << 5) |
				       (hsync_pol << 1) |
				       vsync_pol);

		if (ret)
			return ret;
	}

	/*
	 * powerdown MIPI TX/RX PHY & disable MIPI
	 *
	 * MIPI CONTROL 00
	 * 4:	 PWDN PHY TX
	 * 3:	 PWDN PHY RX
	 * 2:	 MIPI enable
	 */
	ret = imx226_write_reg(sensor,
			       IMX226_REG_IO_MIPI_CTRL00, on ? 0x58 : 0);
	if (ret)
		return ret;

	return imx226_write_reg(sensor, IMX226_REG_SYS_CTRL0, on ?
				IMX226_REG_SYS_CTRL0_SW_PWUP :
				IMX226_REG_SYS_CTRL0_SW_PWDN);
}

int imx226_get_gain(struct imx226_dev *sensor)
{
	u16 gain;
	int ret;

	TRACE

	ret = imx226_read_reg16(sensor, IMX226_REG_AEC_PK_REAL_GAIN, &gain);
	if (ret)
		return ret;

	return gain & 0x3ff;
}

/* read exposure, in number of line periods */
int imx226_get_exposure(struct imx226_dev *sensor)
{
	int exp, ret;
	u8 temp;

	TRACE

	ret = imx226_read_reg(sensor, IMX226_REG_AEC_PK_EXPOSURE_HI, &temp);
	if (ret)
		return ret;
	exp = ((int)temp & 0x0f) << 16;
	ret = imx226_read_reg(sensor, IMX226_REG_AEC_PK_EXPOSURE_MED, &temp);
	if (ret)
		return ret;
	exp |= ((int)temp << 8);
	ret = imx226_read_reg(sensor, IMX226_REG_AEC_PK_EXPOSURE_LO, &temp);
	if (ret)
		return ret;
	exp |= (int)temp;

	return exp >> 4;
}

int imx226_set_mode(struct imx226_dev *sensor)
{
	const struct imx226_mode_info *mode = sensor->current_mode;
	const struct imx226_mode_info *orig_mode = sensor->last_mode;
	enum imx226_downsize_mode dn_mode, orig_dn_mode;
	bool auto_gain = sensor->ctrls.auto_gain->val == 1;
	bool auto_exp =  sensor->ctrls.auto_exp->val == V4L2_EXPOSURE_AUTO;
	unsigned long rate;
	int ret;

	TRACE
	
	dn_mode = mode->dn_mode;
	orig_dn_mode = orig_mode->dn_mode;

	/* auto gain and exposure must be turned off when changing modes */
	if (auto_gain) {
		ret = imx226_set_autogain(sensor, false);
		if (ret)
			return ret;
	}

	if (auto_exp) {
		ret = imx226_set_autoexposure(sensor, false);
		if (ret)
			goto restore_auto_gain;
	}

	/*
	 * All the formats we support have 16 bits per pixel, seems to require
	 * the same rate than YUV, so we can just use 16 bpp all the time.
	 */
	rate = mode->vtot * mode->htot * 16;
	rate *= imx226_framerates[sensor->current_fr];
	if (sensor->ep.bus_type == V4L2_MBUS_CSI2_DPHY) {
		rate = rate / sensor->ep.bus.mipi_csi2.num_data_lanes;
		ret = imx226_set_mipi_pclk(sensor, rate);
	} else {
		rate = rate / sensor->ep.bus.parallel.bus_width;
		ret = imx226_set_dvp_pclk(sensor, rate);
	}

	if (ret < 0)
		return 0;

	if ((dn_mode == SUBSAMPLING && orig_dn_mode == SCALING) ||
	    (dn_mode == SCALING && orig_dn_mode == SUBSAMPLING)) {
		/*
		 * change between subsampling and scaling
		 * go through exposure calculation
		 */
		ret = imx226_set_mode_exposure_calc(sensor, mode);
	} else {
		/*
		 * change inside subsampling or scaling
		 * download firmware directly
		 */
		ret = imx226_set_mode_direct(sensor, mode);
	}
	if (ret < 0)
		goto restore_auto_exp_gain;

	/* restore auto gain and exposure */
	if (auto_gain)
		imx226_set_autogain(sensor, true);
	if (auto_exp)
		imx226_set_autoexposure(sensor, true);

	ret = imx226_set_binning(sensor, dn_mode != SCALING);
	if (ret < 0)
		return ret;
	ret = imx226_set_ae_target(sensor, sensor->ae_target);
	if (ret < 0)
		return ret;
	ret = imx226_get_light_freq(sensor);
	if (ret < 0)
		return ret;
	ret = imx226_set_bandingfilter(sensor);
	if (ret < 0)
		return ret;
	ret = imx226_set_virtual_channel(sensor);
	if (ret < 0)
		return ret;

	sensor->pending_mode_change = false;
	sensor->last_mode = mode;

	return 0;

restore_auto_exp_gain:
	if (auto_exp)
		imx226_set_autoexposure(sensor, true);
restore_auto_gain:
	if (auto_gain)
		imx226_set_autogain(sensor, true);

	return ret;
}

int imx226_check_valid_mode(struct imx226_dev *sensor,
				   const struct imx226_mode_info *mode,
				   enum imx226_frame_rate rate)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret = 0;

	TRACE

	switch (mode->id) {
	case IMX226_MODE_QCIF_176_144:
	case IMX226_MODE_QVGA_320_240:
	case IMX226_MODE_NTSC_720_480:
	case IMX226_MODE_PAL_720_576 :
	case IMX226_MODE_XGA_1024_768:
	case IMX226_MODE_720P_1280_720:
		if ((rate != imx226_15_FPS) &&
		    (rate != imx226_30_FPS))
			ret = -EINVAL;
		break;
	case IMX226_MODE_VGA_640_480:
		if ((rate != imx226_15_FPS) &&
		    (rate != imx226_30_FPS))
			ret = -EINVAL;
		break;
	case IMX226_MODE_1080P_1920_1080:
		if (sensor->ep.bus_type == V4L2_MBUS_CSI2_DPHY) {
			if ((rate != imx226_15_FPS) &&
			    (rate != imx226_30_FPS))
				ret = -EINVAL;
		 } else {
			if ((rate != imx226_15_FPS))
				ret = -EINVAL;
		 }
		break;
	case IMX226_MODE_QSXGA_2592_1944:
		if (rate != imx226_08_FPS)
			ret = -EINVAL;
		break;
	default:
		dev_err(&client->dev, "[vc-mipi imx226] Invalid mode (%d)\n", mode->id);
		ret = -EINVAL;
	}

	return ret;
}