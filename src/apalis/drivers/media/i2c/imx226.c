// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 */
#include "imx226.h"
#include "vc_mipi_ctrl.h"

#define TRACE printk("        TRACE [vc-mipi] imx226.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

static int imx226_init_slave_id(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg;
	u8 buf[3];
	int ret;

	TRACE

	if (client->addr == IMX226_DEFAULT_SLAVE_ID)
		return 0;

	buf[0] = IMX226_REG_SLAVE_ID >> 8;
	buf[1] = IMX226_REG_SLAVE_ID & 0xff;
	buf[2] = client->addr << 1;

	msg.addr = IMX226_DEFAULT_SLAVE_ID;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: failed with %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int imx226_write_reg(struct imx226_dev *sensor, u16 reg, u8 val)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg;
	u8 buf[3];
	int ret;

	TRACE

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: error: reg=%x, val=%x\n",
			__func__, reg, val);
		return ret;
	}

	return 0;
}

static int imx226_read_reg(struct imx226_dev *sensor, u16 reg, u8 *val)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg[2];
	u8 buf[2];
	int ret;

	TRACE

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].addr = client->addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = 1;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: error: reg=%x\n",
			__func__, reg);
		return ret;
	}

	*val = buf[0];
	return 0;
}

static int imx226_read_reg16(struct imx226_dev *sensor, u16 reg, u16 *val)
{
	u8 hi, lo;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, reg, &hi);
	if (ret)
		return ret;
	ret = imx226_read_reg(sensor, reg + 1, &lo);
	if (ret)
		return ret;

	*val = ((u16)hi << 8) | (u16)lo;
	return 0;
}

static int imx226_write_reg16(struct imx226_dev *sensor, u16 reg, u16 val)
{
	int ret;

	TRACE

	ret = imx226_write_reg(sensor, reg, val >> 8);
	if (ret)
		return ret;

	return imx226_write_reg(sensor, reg + 1, val & 0xff);
}

static int imx226_mod_reg(struct imx226_dev *sensor, u16 reg,
			  u8 mask, u8 val)
{
	u8 readval;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, reg, &readval);
	if (ret)
		return ret;

	readval &= ~mask;
	val &= mask;
	val |= readval;

	return imx226_write_reg(sensor, reg, val);
}

/*
 * After trying the various combinations, reading various
 * documentations spread around the net, and from the various
 * feedback, the clock tree is probably as follows:
 *
 *   +--------------+
 *   |  Ext. Clock  |
 *   +-+------------+
 *     |  +----------+
 *     +->|   PLL1   | - reg 0x3036, for the multiplier
 *        +-+--------+ - reg 0x3037, bits 0-3 for the pre-divider
 *          |  +--------------+
 *          +->| System Clock |  - reg 0x3035, bits 4-7
 *             +-+------------+
 *               |  +--------------+
 *               +->| MIPI Divider | - reg 0x3035, bits 0-3
 *               |  +-+------------+
 *               |    +----------------> MIPI SCLK
 *               |    +  +-----+
 *               |    +->| / 2 |-------> MIPI BIT CLK
 *               |       +-----+
 *               |  +--------------+
 *               +->| PLL Root Div | - reg 0x3037, bit 4
 *                  +-+------------+
 *                    |  +---------+
 *                    +->| Bit Div | - reg 0x3034, bits 0-3
 *                       +-+-------+
 *                         |  +-------------+
 *                         +->| SCLK Div    | - reg 0x3108, bits 0-1
 *                         |  +-+-----------+
 *                         |    +---------------> SCLK
 *                         |  +-------------+
 *                         +->| SCLK 2X Div | - reg 0x3108, bits 2-3
 *                         |  +-+-----------+
 *                         |    +---------------> SCLK 2X
 *                         |  +-------------+
 *                         +->| PCLK Div    | - reg 0x3108, bits 4-5
 *                            ++------------+
 *                             +  +-----------+
 *                             +->|   P_DIV   | - reg 0x3035, bits 0-3
 *                                +-----+-----+
 *                                       +------------> PCLK
 *
 * This is deviating from the datasheet at least for the register
 * 0x3108, since it's said here that the PCLK would be clocked from
 * the PLL.
 *
 * There seems to be also (unverified) constraints:
 *  - the PLL pre-divider output rate should be in the 4-27MHz range
 *  - the PLL multiplier output rate should be in the 500-1000MHz range
 *  - PCLK >= SCLK * 2 in YUV, >= SCLK in Raw or JPEG
 *
 * In the two latter cases, these constraints are met since our
 * factors are hardcoded. If we were to change that, we would need to
 * take this into account. The only varying parts are the PLL
 * multiplier and the system clock divider, which are shared between
 * all these clocks so won't cause any issue.
 */

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 3 in the vendor kernels.
 */
#define imx226_PLL_PREDIV	3

#define imx226_PLL_MULT_MIN	4
#define imx226_PLL_MULT_MAX	252

/*
 * This is supposed to be ranging from 1 to 16, but the value is
 * always set to either 1 or 2 in the vendor kernels.
 */
#define imx226_SYSDIV_MIN	1
#define imx226_SYSDIV_MAX	16

/*
 * Hardcode these values for scaler and non-scaler modes.
 * FIXME: to be re-calcualted for 1 data lanes setups
 */
#define imx226_MIPI_DIV_PCLK	2
#define imx226_MIPI_DIV_SCLK	1

/*
 * This is supposed to be ranging from 1 to 2, but the value is always
 * set to 2 in the vendor kernels.
 */
#define imx226_PLL_ROOT_DIV			2
#define imx226_PLL_CTRL3_PLL_ROOT_DIV_2		BIT(4)

/*
 * We only supports 8-bit formats at the moment
 */
#define imx226_BIT_DIV				2
#define imx226_PLL_CTRL0_MIPI_MODE_8BIT		0x08

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 2 in the vendor kernels.
 */
#define imx226_SCLK_ROOT_DIV	2

/*
 * This is hardcoded so that the consistency is maintained between SCLK and
 * SCLK 2x.
 */
#define imx226_SCLK2X_ROOT_DIV (imx226_SCLK_ROOT_DIV / 2)

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 1 in the vendor kernels.
 */
#define imx226_PCLK_ROOT_DIV			1
#define imx226_PLL_SYS_ROOT_DIVIDER_BYPASS	0x00

static unsigned long imx226_compute_sys_clk(struct imx226_dev *sensor,
					    u8 pll_prediv, u8 pll_mult,
					    u8 sysdiv)
{
	unsigned long sysclk = sensor->xclk_freq / pll_prediv * pll_mult;

	TRACE
	
	/* PLL1 output cannot exceed 1GHz. */
	if (sysclk / 1000000 > 1000)
		return 0;

	return sysclk / sysdiv;
}

static unsigned long imx226_calc_sys_clk(struct imx226_dev *sensor,
					 unsigned long rate,
					 u8 *pll_prediv, u8 *pll_mult,
					 u8 *sysdiv)
{
	unsigned long best = ~0;
	u8 best_sysdiv = 1, best_mult = 1;
	u8 _sysdiv, _pll_mult;

	TRACE

	for (_sysdiv = imx226_SYSDIV_MIN;
	     _sysdiv <= imx226_SYSDIV_MAX;
	     _sysdiv++) {
		for (_pll_mult = imx226_PLL_MULT_MIN;
		     _pll_mult <= imx226_PLL_MULT_MAX;
		     _pll_mult++) {
			unsigned long _rate;

			/*
			 * The PLL multiplier cannot be odd if above
			 * 127.
			 */
			if (_pll_mult > 127 && (_pll_mult % 2))
				continue;

			_rate = imx226_compute_sys_clk(sensor,
						       imx226_PLL_PREDIV,
						       _pll_mult, _sysdiv);

			/*
			 * We have reached the maximum allowed PLL1 output,
			 * increase sysdiv.
			 */
			if (!_rate)
				break;

			/*
			 * Prefer rates above the expected clock rate than
			 * below, even if that means being less precise.
			 */
			if (_rate < rate)
				continue;

			if (abs(rate - _rate) < abs(rate - best)) {
				best = _rate;
				best_sysdiv = _sysdiv;
				best_mult = _pll_mult;
			}

			if (_rate == rate)
				goto out;
		}
	}

out:
	*sysdiv = best_sysdiv;
	*pll_prediv = imx226_PLL_PREDIV;
	*pll_mult = best_mult;

	return best;
}

static int imx226_check_valid_mode(struct imx226_dev *sensor,
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

/*
 * imx226_set_mipi_pclk() - Calculate the clock tree configuration values
 *			    for the MIPI CSI-2 output.
 *
 * @rate: The requested bandwidth per lane in bytes per second.
 *	  'Bandwidth Per Lane' is calculated as:
 *	  bpl = HTOT * VTOT * FPS * bpp / num_lanes;
 *
 * This function use the requested bandwidth to calculate:
 * - sample_rate = bpl / (bpp / num_lanes);
 *	         = bpl / (PLL_RDIV * BIT_DIV * PCLK_DIV * MIPI_DIV / num_lanes);
 *
 * - mipi_sclk   = bpl / MIPI_DIV / 2; ( / 2 is for CSI-2 DDR)
 *
 * with these fixed parameters:
 *	PLL_RDIV	= 2;
 *	BIT_DIVIDER	= 2; (MIPI_BIT_MODE == 8 ? 2 : 2,5);
 *	PCLK_DIV	= 1;
 *
 * The MIPI clock generation differs for modes that use the scaler and modes
 * that do not. In case the scaler is in use, the MIPI_SCLK generates the MIPI
 * BIT CLk, and thus:
 *
 * - mipi_sclk = bpl / MIPI_DIV / 2;
 *   MIPI_DIV = 1;
 *
 * For modes that do not go through the scaler, the MIPI BIT CLOCK is generated
 * from the pixel clock, and thus:
 *
 * - sample_rate = bpl / (bpp / num_lanes);
 *	         = bpl / (2 * 2 * 1 * MIPI_DIV / num_lanes);
 *		 = bpl / (4 * MIPI_DIV / num_lanes);
 * - MIPI_DIV	 = bpp / (4 * num_lanes);
 *
 * FIXME: this have been tested with 16bpp and 2 lanes setup only.
 * MIPI_DIV is fixed to value 2, but it -might- be changed according to the
 * above formula for setups with 1 lane or image formats with different bpp.
 *
 * FIXME: this deviates from the sensor manual documentation which is quite
 * thin on the MIPI clock tree generation part.
 */
static int imx226_set_mipi_pclk(struct imx226_dev *sensor,
				unsigned long rate)
{
	const struct imx226_mode_info *mode = sensor->current_mode;
	u8 prediv, mult, sysdiv;
	u8 mipi_div;
	int ret;

	TRACE

	/*
	 * 1280x720 is reported to use 'SUBSAMPLING' only,
	 * but according to the sensor manual it goes through the
	 * scaler before subsampling.
	 */
	if (mode->dn_mode == SCALING ||
	   (mode->id == IMX226_MODE_720P_1280_720))
		mipi_div = imx226_MIPI_DIV_SCLK;
	else
		mipi_div = imx226_MIPI_DIV_PCLK;

	imx226_calc_sys_clk(sensor, rate, &prediv, &mult, &sysdiv);

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL0,
			     0x0f, imx226_PLL_CTRL0_MIPI_MODE_8BIT);

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL1,
			     0xff, sysdiv << 4 | mipi_div);
	if (ret)
		return ret;

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL2, 0xff, mult);
	if (ret)
		return ret;

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL3,
			     0x1f, imx226_PLL_CTRL3_PLL_ROOT_DIV_2 | prediv);
	if (ret)
		return ret;

	return imx226_mod_reg(sensor, IMX226_REG_SYS_ROOT_DIVIDER,
			      0x30, imx226_PLL_SYS_ROOT_DIVIDER_BYPASS);
}

static unsigned long imx226_calc_pclk(struct imx226_dev *sensor,
				      unsigned long rate,
				      u8 *pll_prediv, u8 *pll_mult, u8 *sysdiv,
				      u8 *pll_rdiv, u8 *bit_div, u8 *pclk_div)
{
	unsigned long _rate = rate * imx226_PLL_ROOT_DIV * imx226_BIT_DIV *
				imx226_PCLK_ROOT_DIV;

	TRACE

	_rate = imx226_calc_sys_clk(sensor, _rate, pll_prediv, pll_mult,
				    sysdiv);
	*pll_rdiv = imx226_PLL_ROOT_DIV;
	*bit_div = imx226_BIT_DIV;
	*pclk_div = imx226_PCLK_ROOT_DIV;

	return _rate / *pll_rdiv / *bit_div / *pclk_div;
}

static int imx226_set_dvp_pclk(struct imx226_dev *sensor, unsigned long rate)
{
	u8 prediv, mult, sysdiv, pll_rdiv, bit_div, pclk_div;
	int ret;

	TRACE

	imx226_calc_pclk(sensor, rate, &prediv, &mult, &sysdiv, &pll_rdiv,
			 &bit_div, &pclk_div);

	if (bit_div == 2)
		bit_div = 0xA;

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL0,
			     0x0f, bit_div);
	if (ret)
		return ret;

	/*
	 * We need to set sysdiv according to the clock, and to clear
	 * the MIPI divider.
	 */
	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL1,
			     0xff, sysdiv << 4);
	if (ret)
		return ret;

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL2,
			     0xff, mult);
	if (ret)
		return ret;

	ret = imx226_mod_reg(sensor, IMX226_REG_SC_PLL_CTRL3,
			     0x1f, prediv | ((pll_rdiv - 1) << 4));
	if (ret)
		return ret;

	return imx226_mod_reg(sensor, IMX226_REG_SYS_ROOT_DIVIDER, 0x30,
			      (ilog2(pclk_div) << 4));
}

/* set JPEG framing sizes */
static int imx226_set_jpeg_timings(struct imx226_dev *sensor,
				   const struct imx226_mode_info *mode)
{
	int ret;

	TRACE

	/*
	 * compression mode 3 timing
	 *
	 * Data is transmitted with programmable width (VFIFO_HSIZE).
	 * No padding done. Last line may have less data. Varying
	 * number of lines per frame, depending on amount of data.
	 */
	ret = imx226_mod_reg(sensor, IMX226_REG_JPG_MODE_SELECT, 0x7, 0x3);
	if (ret < 0)
		return ret;

	ret = imx226_write_reg16(sensor, IMX226_REG_VFIFO_HSIZE, mode->hact);
	if (ret < 0)
		return ret;

	return imx226_write_reg16(sensor, IMX226_REG_VFIFO_VSIZE, mode->vact);
}

/* download imx226 settings to sensor through i2c */
static int imx226_set_timings(struct imx226_dev *sensor,
			      const struct imx226_mode_info *mode)
{
	int ret;

	TRACE

	if (sensor->fmt.code == MEDIA_BUS_FMT_JPEG_1X8) {
		ret = imx226_set_jpeg_timings(sensor, mode);
		if (ret < 0)
			return ret;
	}

	ret = imx226_write_reg16(sensor, IMX226_REG_TIMING_DVPHO, mode->hact);
	if (ret < 0)
		return ret;

	ret = imx226_write_reg16(sensor, IMX226_REG_TIMING_DVPVO, mode->vact);
	if (ret < 0)
		return ret;

	ret = imx226_write_reg16(sensor, IMX226_REG_TIMING_HTS, mode->htot);
	if (ret < 0)
		return ret;

	return imx226_write_reg16(sensor, IMX226_REG_TIMING_VTS, mode->vtot);
}

static int imx226_load_regs(struct imx226_dev *sensor,
			    const struct imx226_mode_info *mode)
{
	const struct reg_value *regs = mode->reg_data;
	unsigned int i;
	u32 delay_ms;
	u16 reg_addr;
	u8 mask, val;
	int ret = 0;

	TRACE

	for (i = 0; i < mode->reg_data_size; ++i, ++regs) {
		delay_ms = regs->delay_ms;
		reg_addr = regs->reg_addr;
		val = regs->val;
		mask = regs->mask;

		/* remain in power down mode for DVP */
		if (regs->reg_addr == IMX226_REG_SYS_CTRL0 &&
		    val == IMX226_REG_SYS_CTRL0_SW_PWUP &&
		    sensor->ep.bus_type != V4L2_MBUS_CSI2_DPHY)
			continue;

		if (mask)
			ret = imx226_mod_reg(sensor, reg_addr, mask, val);
		else
			ret = imx226_write_reg(sensor, reg_addr, val);
		if (ret)
			break;

		if (delay_ms)
			usleep_range(1000 * delay_ms, 1000 * delay_ms + 100);
	}

	return imx226_set_timings(sensor, mode);
}

static int imx226_set_autoexposure(struct imx226_dev *sensor, bool on)
{
	TRACE
	
	return imx226_mod_reg(sensor, IMX226_REG_AEC_PK_MANUAL,
			      BIT(0), on ? 0 : BIT(0));
}

/* read exposure, in number of line periods */
static int imx226_get_exposure(struct imx226_dev *sensor)
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

/* write exposure, given number of line periods */
static int imx226_set_exposure(struct imx226_dev *sensor, u32 exposure)
{
	int ret;

	TRACE

	exposure <<= 4;

	ret = imx226_write_reg(sensor,
			       IMX226_REG_AEC_PK_EXPOSURE_LO,
			       exposure & 0xff);
	if (ret)
		return ret;
	ret = imx226_write_reg(sensor,
			       IMX226_REG_AEC_PK_EXPOSURE_MED,
			       (exposure >> 8) & 0xff);
	if (ret)
		return ret;
	return imx226_write_reg(sensor,
				IMX226_REG_AEC_PK_EXPOSURE_HI,
				(exposure >> 16) & 0x0f);
}

static int imx226_get_gain(struct imx226_dev *sensor)
{
	u16 gain;
	int ret;

	TRACE

	ret = imx226_read_reg16(sensor, IMX226_REG_AEC_PK_REAL_GAIN, &gain);
	if (ret)
		return ret;

	return gain & 0x3ff;
}

static int imx226_set_gain(struct imx226_dev *sensor, int gain)
{
	TRACE
	
	return imx226_write_reg16(sensor, IMX226_REG_AEC_PK_REAL_GAIN,
				  (u16)gain & 0x3ff);
}

static int imx226_set_autogain(struct imx226_dev *sensor, bool on)
{
	TRACE
	
	return imx226_mod_reg(sensor, IMX226_REG_AEC_PK_MANUAL,
			      BIT(1), on ? 0 : BIT(1));
}

static int imx226_set_stream_dvp(struct imx226_dev *sensor, bool on)
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

static int imx226_set_stream_mipi(struct imx226_dev *sensor, bool on)
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

static int imx226_get_sysclk(struct imx226_dev *sensor)
{
	/* calculate sysclk */
	u32 xvclk = sensor->xclk_freq / 10000;
	u32 multiplier, prediv, VCO, sysdiv, pll_rdiv;
	u32 sclk_rdiv_map[] = {1, 2, 4, 8};
	u32 bit_div2x = 1, sclk_rdiv, sysclk;
	u8 temp1, temp2;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, IMX226_REG_SC_PLL_CTRL0, &temp1);
	if (ret)
		return ret;
	temp2 = temp1 & 0x0f;
	if (temp2 == 8 || temp2 == 10)
		bit_div2x = temp2 / 2;

	ret = imx226_read_reg(sensor, IMX226_REG_SC_PLL_CTRL1, &temp1);
	if (ret)
		return ret;
	sysdiv = temp1 >> 4;
	if (sysdiv == 0)
		sysdiv = 16;

	ret = imx226_read_reg(sensor, IMX226_REG_SC_PLL_CTRL2, &temp1);
	if (ret)
		return ret;
	multiplier = temp1;

	ret = imx226_read_reg(sensor, IMX226_REG_SC_PLL_CTRL3, &temp1);
	if (ret)
		return ret;
	prediv = temp1 & 0x0f;
	pll_rdiv = ((temp1 >> 4) & 0x01) + 1;

	ret = imx226_read_reg(sensor, IMX226_REG_SYS_ROOT_DIVIDER, &temp1);
	if (ret)
		return ret;
	temp2 = temp1 & 0x03;
	sclk_rdiv = sclk_rdiv_map[temp2];

	if (!prediv || !sysdiv || !pll_rdiv || !bit_div2x)
		return -EINVAL;

	VCO = xvclk * multiplier / prediv;

	sysclk = VCO / sysdiv / pll_rdiv * 2 / bit_div2x / sclk_rdiv;

	return sysclk;
}

static int imx226_set_night_mode(struct imx226_dev *sensor)
{	
	 /* read HTS from register settings */
	u8 mode;
	int ret;

	TRACE

	ret = imx226_read_reg(sensor, IMX226_REG_AEC_CTRL00, &mode);
	if (ret)
		return ret;
	mode &= 0xfb;
	return imx226_write_reg(sensor, IMX226_REG_AEC_CTRL00, mode);
}

static int imx226_get_hts(struct imx226_dev *sensor)
{
	/* read HTS from register settings */
	u16 hts;
	int ret;

	TRACE

	ret = imx226_read_reg16(sensor, IMX226_REG_TIMING_HTS, &hts);
	if (ret)
		return ret;
	return hts;
}

static int imx226_get_vts(struct imx226_dev *sensor)
{
	u16 vts;
	int ret;

	TRACE

	ret = imx226_read_reg16(sensor, IMX226_REG_TIMING_VTS, &vts);
	if (ret)
		return ret;
	return vts;
}

static int imx226_set_vts(struct imx226_dev *sensor, int vts)
{
	TRACE
	
	return imx226_write_reg16(sensor, IMX226_REG_TIMING_VTS, vts);
}

static int imx226_get_light_freq(struct imx226_dev *sensor)
{
	/* get banding filter value */
	int ret, light_freq = 0;
	u8 temp, temp1;

	TRACE

	ret = imx226_read_reg(sensor, IMX226_REG_HZ5060_CTRL01, &temp);
	if (ret)
		return ret;

	if (temp & 0x80) {
		/* manual */
		ret = imx226_read_reg(sensor, IMX226_REG_HZ5060_CTRL00,
				      &temp1);
		if (ret)
			return ret;
		if (temp1 & 0x04) {
			/* 50Hz */
			light_freq = 50;
		} else {
			/* 60Hz */
			light_freq = 60;
		}
	} else {
		/* auto */
		ret = imx226_read_reg(sensor, IMX226_REG_SIGMADELTA_CTRL0C,
				      &temp1);
		if (ret)
			return ret;

		if (temp1 & 0x01) {
			/* 50Hz */
			light_freq = 50;
		} else {
			/* 60Hz */
		}
	}

	return light_freq;
}

static int imx226_set_bandingfilter(struct imx226_dev *sensor)
{
	u32 band_step60, max_band60, band_step50, max_band50, prev_vts;
	int ret;

	TRACE

	/* read preview PCLK */
	ret = imx226_get_sysclk(sensor);
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -EINVAL;
	sensor->prev_sysclk = ret;
	/* read preview HTS */
	ret = imx226_get_hts(sensor);
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -EINVAL;
	sensor->prev_hts = ret;

	/* read preview VTS */
	ret = imx226_get_vts(sensor);
	if (ret < 0)
		return ret;
	prev_vts = ret;

	/* calculate banding filter */
	/* 60Hz */
	band_step60 = sensor->prev_sysclk * 100 / sensor->prev_hts * 100 / 120;
	ret = imx226_write_reg16(sensor, IMX226_REG_AEC_B60_STEP, band_step60);
	if (ret)
		return ret;
	if (!band_step60)
		return -EINVAL;
	max_band60 = (int)((prev_vts - 4) / band_step60);
	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL0D, max_band60);
	if (ret)
		return ret;

	/* 50Hz */
	band_step50 = sensor->prev_sysclk * 100 / sensor->prev_hts;
	ret = imx226_write_reg16(sensor, IMX226_REG_AEC_B50_STEP, band_step50);
	if (ret)
		return ret;
	if (!band_step50)
		return -EINVAL;
	max_band50 = (int)((prev_vts - 4) / band_step50);
	return imx226_write_reg(sensor, IMX226_REG_AEC_CTRL0E, max_band50);
}

static int imx226_set_ae_target(struct imx226_dev *sensor, int target)
{
	/* stable in high */
	u32 fast_high, fast_low;
	int ret;

	TRACE
	
	sensor->ae_low = target * 23 / 25;	/* 0.92 */
	sensor->ae_high = target * 27 / 25;	/* 1.08 */

	fast_high = sensor->ae_high << 1;
	if (fast_high > 255)
		fast_high = 255;

	fast_low = sensor->ae_low >> 1;

	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL0F, sensor->ae_high);
	if (ret)
		return ret;
	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL10, sensor->ae_low);
	if (ret)
		return ret;
	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL1B, sensor->ae_high);
	if (ret)
		return ret;
	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL1E, sensor->ae_low);
	if (ret)
		return ret;
	ret = imx226_write_reg(sensor, IMX226_REG_AEC_CTRL11, fast_high);
	if (ret)
		return ret;
	return imx226_write_reg(sensor, IMX226_REG_AEC_CTRL1F, fast_low);
}

static int imx226_get_binning(struct imx226_dev *sensor)
{
	u8 temp;
	int ret;

	TRACE
	
	ret = imx226_read_reg(sensor, IMX226_REG_TIMING_TC_REG21, &temp);
	if (ret)
		return ret;

	return temp & BIT(0);
}

static int imx226_set_binning(struct imx226_dev *sensor, bool enable)
{
	int ret;

	TRACE
	
	/*
	 * TIMING TC REG21:
	 * - [0]:	Horizontal binning enable
	 */
	ret = imx226_mod_reg(sensor, IMX226_REG_TIMING_TC_REG21,
			     BIT(0), enable ? BIT(0) : 0);
	if (ret)
		return ret;
	/*
	 * TIMING TC REG20:
	 * - [0]:	Undocumented, but hardcoded init sequences
	 *		are always setting REG21/REG20 bit 0 to same value...
	 */
	return imx226_mod_reg(sensor, IMX226_REG_TIMING_TC_REG20,
			      BIT(0), enable ? BIT(0) : 0);
}

static int imx226_set_virtual_channel(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	u8 temp, channel = virtual_channel;
	int ret;

	TRACE
	
	if (channel > 3) {
		dev_err(&client->dev,
			"[vc-mipi imx226] %s: wrong virtual_channel parameter, expected (0..3), got %d\n",
			__func__, channel);
		return -EINVAL;
	}

	ret = imx226_read_reg(sensor, IMX226_REG_DEBUG_MODE, &temp);
	if (ret)
		return ret;
	temp &= ~(3 << 6);
	temp |= (channel << 6);
	return imx226_write_reg(sensor, IMX226_REG_DEBUG_MODE, temp);
}

static const struct imx226_mode_info *
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

/*
 * sensor changes between scaling and subsampling, go through
 * exposure calculation
 */
static int imx226_set_mode_exposure_calc(struct imx226_dev *sensor,
					 const struct imx226_mode_info *mode)
{
	u32 prev_shutter, prev_gain16;
	u32 cap_shutter, cap_gain16;
	u32 cap_sysclk, cap_hts, cap_vts;
	u32 light_freq, cap_bandfilt, cap_maxband;
	u32 cap_gain16_shutter;
	u8 average;
	int ret;

	TRACE
	
	if (!mode->reg_data)
		return -EINVAL;

	/* read preview shutter */
	ret = imx226_get_exposure(sensor);
	if (ret < 0)
		return ret;
	prev_shutter = ret;
	ret = imx226_get_binning(sensor);
	if (ret < 0)
		return ret;
	if (ret && mode->id != IMX226_MODE_720P_1280_720 &&
	    mode->id != IMX226_MODE_1080P_1920_1080)
		prev_shutter *= 2;

	/* read preview gain */
	ret = imx226_get_gain(sensor);
	if (ret < 0)
		return ret;
	prev_gain16 = ret;

	/* get average */
	ret = imx226_read_reg(sensor, IMX226_REG_AVG_READOUT, &average);
	if (ret)
		return ret;

	/* turn off night mode for capture */
	ret = imx226_set_night_mode(sensor);
	if (ret < 0)
		return ret;

	/* Write capture setting */
	ret = imx226_load_regs(sensor, mode);
	if (ret < 0)
		return ret;

	/* read capture VTS */
	ret = imx226_get_vts(sensor);
	if (ret < 0)
		return ret;
	cap_vts = ret;
	ret = imx226_get_hts(sensor);
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -EINVAL;
	cap_hts = ret;

	ret = imx226_get_sysclk(sensor);
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -EINVAL;
	cap_sysclk = ret;

	/* calculate capture banding filter */
	ret = imx226_get_light_freq(sensor);
	if (ret < 0)
		return ret;
	light_freq = ret;

	if (light_freq == 60) {
		/* 60Hz */
		cap_bandfilt = cap_sysclk * 100 / cap_hts * 100 / 120;
	} else {
		/* 50Hz */
		cap_bandfilt = cap_sysclk * 100 / cap_hts;
	}

	if (!sensor->prev_sysclk) {
		ret = imx226_get_sysclk(sensor);
		if (ret < 0)
			return ret;
		if (ret == 0)
			return -EINVAL;
		sensor->prev_sysclk = ret;
	}

	if (!cap_bandfilt)
		return -EINVAL;

	cap_maxband = (int)((cap_vts - 4) / cap_bandfilt);

	/* calculate capture shutter/gain16 */
	if (average > sensor->ae_low && average < sensor->ae_high) {
		/* in stable range */
		cap_gain16_shutter =
			prev_gain16 * prev_shutter *
			cap_sysclk / sensor->prev_sysclk *
			sensor->prev_hts / cap_hts *
			sensor->ae_target / average;
	} else {
		cap_gain16_shutter =
			prev_gain16 * prev_shutter *
			cap_sysclk / sensor->prev_sysclk *
			sensor->prev_hts / cap_hts;
	}

	/* gain to shutter */
	if (cap_gain16_shutter < (cap_bandfilt * 16)) {
		/* shutter < 1/100 */
		cap_shutter = cap_gain16_shutter / 16;
		if (cap_shutter < 1)
			cap_shutter = 1;

		cap_gain16 = cap_gain16_shutter / cap_shutter;
		if (cap_gain16 < 16)
			cap_gain16 = 16;
	} else {
		if (cap_gain16_shutter > (cap_bandfilt * cap_maxband * 16)) {
			/* exposure reach max */
			cap_shutter = cap_bandfilt * cap_maxband;
			if (!cap_shutter)
				return -EINVAL;

			cap_gain16 = cap_gain16_shutter / cap_shutter;
		} else {
			/* 1/100 < (cap_shutter = n/100) =< max */
			cap_shutter =
				((int)(cap_gain16_shutter / 16 / cap_bandfilt))
				* cap_bandfilt;
			if (!cap_shutter)
				return -EINVAL;

			cap_gain16 = cap_gain16_shutter / cap_shutter;
		}
	}

	/* set capture gain */
	ret = imx226_set_gain(sensor, cap_gain16);
	if (ret)
		return ret;

	/* write capture shutter */
	if (cap_shutter > (cap_vts - 4)) {
		cap_vts = cap_shutter + 4;
		ret = imx226_set_vts(sensor, cap_vts);
		if (ret < 0)
			return ret;
	}

	/* set exposure */
	return imx226_set_exposure(sensor, cap_shutter);
}

/*
 * if sensor changes inside scaling or subsampling
 * change mode directly
 */
static int imx226_set_mode_direct(struct imx226_dev *sensor,
				  const struct imx226_mode_info *mode)
{
	TRACE
	
	if (!mode->reg_data)
		return -EINVAL;

	/* Write capture setting */
	return imx226_load_regs(sensor, mode);
}

static int imx226_set_mode(struct imx226_dev *sensor)
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

static int imx226_set_framefmt(struct imx226_dev *sensor,
			       struct v4l2_mbus_framefmt *format);

/* restore the last set video mode after chip power-on */
static int imx226_restore_mode(struct imx226_dev *sensor)
{
	int ret;

	TRACE
	
	/* first load the initial register values */
	ret = imx226_load_regs(sensor, &imx226_mode_init_data);
	if (ret < 0)
		return ret;
	sensor->last_mode = &imx226_mode_init_data;

	ret = imx226_mod_reg(sensor, IMX226_REG_SYS_ROOT_DIVIDER, 0x3f,
			     (ilog2(imx226_SCLK2X_ROOT_DIV) << 2) |
			     ilog2(imx226_SCLK_ROOT_DIV));
	if (ret)
		return ret;

	/* now restore the last capture mode */
	ret = imx226_set_mode(sensor);
	if (ret < 0)
		return ret;

	return imx226_set_framefmt(sensor, &sensor->fmt);
}

static void imx226_power(struct imx226_dev *sensor, bool enable)
{
	TRACE
	
	gpiod_set_value_cansleep(sensor->pwdn_gpio, enable ? 0 : 1);
}

static void imx226_reset(struct imx226_dev *sensor)
{
	TRACE
	
	if (!sensor->reset_gpio)
		return;

	gpiod_set_value_cansleep(sensor->reset_gpio, 0);

	/* camera power cycle */
	imx226_power(sensor, false);
	usleep_range(5000, 10000);
	imx226_power(sensor, true);
	usleep_range(5000, 10000);

	gpiod_set_value_cansleep(sensor->reset_gpio, 1);
	usleep_range(1000, 2000);

	gpiod_set_value_cansleep(sensor->reset_gpio, 0);
	usleep_range(20000, 25000);
}

static int imx226_set_power_on(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret;

	TRACE
	
	ret = clk_prepare_enable(sensor->xclk);
	if (ret) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: failed to enable clock\n",
			__func__);
		return ret;
	}

	ret = regulator_bulk_enable(IMX226_NUM_SUPPLIES,
				    sensor->supplies);
	if (ret) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: failed to enable regulators\n",
			__func__);
		goto xclk_off;
	}

	imx226_reset(sensor);
	imx226_power(sensor, true);

	ret = imx226_init_slave_id(sensor);
	if (ret)
		goto power_off;

	return 0;

power_off:
	imx226_power(sensor, false);
	regulator_bulk_disable(IMX226_NUM_SUPPLIES, sensor->supplies);
xclk_off:
	clk_disable_unprepare(sensor->xclk);
	return ret;
}

static void imx226_set_power_off(struct imx226_dev *sensor)
{
	TRACE
	
	imx226_power(sensor, false);
	regulator_bulk_disable(IMX226_NUM_SUPPLIES, sensor->supplies);
	clk_disable_unprepare(sensor->xclk);
}

static int imx226_set_power_mipi(struct imx226_dev *sensor, bool on)
{
	int ret;

	TRACE
	
	if (!on) {
		/* Reset MIPI bus settings to their default values. */
		imx226_write_reg(sensor, IMX226_REG_IO_MIPI_CTRL00, 0x58);
		imx226_write_reg(sensor, IMX226_REG_MIPI_CTRL00, 0x04);
		imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT00, 0x00);
		return 0;
	}

	/*
	 * Power up MIPI HS Tx and LS Rx; 2 data lanes mode
	 *
	 * 0x300e = 0x40
	 * [7:5] = 010	: 2 data lanes mode (see FIXME note in
	 *		  "imx226_set_stream_mipi()")
	 * [4] = 0	: Power up MIPI HS Tx
	 * [3] = 0	: Power up MIPI LS Rx
	 * [2] = 0	: MIPI interface disabled
	 */
	ret = imx226_write_reg(sensor, IMX226_REG_IO_MIPI_CTRL00, 0x40);
	if (ret)
		return ret;

	/*
	 * Gate clock and set LP11 in 'no packets mode' (idle)
	 *
	 * 0x4800 = 0x24
	 * [5] = 1	: Gate clock when 'no packets'
	 * [2] = 1	: MIPI bus in LP11 when 'no packets'
	 */
	ret = imx226_write_reg(sensor, IMX226_REG_MIPI_CTRL00, 0x24);
	if (ret)
		return ret;

	/*
	 * Set data lanes and clock in LP11 when 'sleeping'
	 *
	 * 0x3019 = 0x70
	 * [6] = 1	: MIPI data lane 2 in LP11 when 'sleeping'
	 * [5] = 1	: MIPI data lane 1 in LP11 when 'sleeping'
	 * [4] = 1	: MIPI clock lane in LP11 when 'sleeping'
	 */
	ret = imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT00, 0x70);
	if (ret)
		return ret;

	/* Give lanes some time to coax into LP11 state. */
	usleep_range(500, 1000);

	return 0;
}

static int imx226_set_power_dvp(struct imx226_dev *sensor, bool on)
{
	int ret;

	TRACE
	
	if (!on) {
		/* Reset settings to their default values. */
		imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT_ENABLE01, 0x00);
		imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT_ENABLE02, 0x00);
		return 0;
	}

	/*
	 * enable VSYNC/HREF/PCLK DVP control lines
	 * & D[9:6] DVP data lines
	 *
	 * PAD OUTPUT ENABLE 01
	 * - 6:		VSYNC output enable
	 * - 5:		HREF output enable
	 * - 4:		PCLK output enable
	 * - [3:0]:	D[9:6] output enable
	 */
	ret = imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT_ENABLE01, 0x7f);
	if (ret)
		return ret;

	/*
	 * enable D[5:0] DVP data lines
	 *
	 * PAD OUTPUT ENABLE 02
	 * - [7:2]:	D[5:0] output enable
	 */
	return imx226_write_reg(sensor, IMX226_REG_PAD_OUTPUT_ENABLE02, 0xfc);
}

static int imx226_set_power(struct imx226_dev *sensor, bool on)
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

/* --------------- Subdev Operations --------------- */

static int imx226_s_power(struct v4l2_subdev *sd, int on)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	int ret = 0;

	TRACE
	
	mutex_lock(&sensor->lock);

	/*
	 * If the power count is modified from 0 to != 0 or from != 0 to 0,
	 * update the power state.
	 */
	if (sensor->power_count == !on) {
		ret = imx226_set_power(sensor, !!on);
		if (ret)
			goto out;
	}

	/* Update the power count. */
	sensor->power_count += on ? 1 : -1;
	WARN_ON(sensor->power_count < 0);
out:
	mutex_unlock(&sensor->lock);

	if (on && !ret && sensor->power_count == 1) {
		/* restore controls */
		ret = v4l2_ctrl_handler_setup(&sensor->ctrls.handler);
	}

	return ret;
}

static int imx226_try_frame_interval(struct imx226_dev *sensor,
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

static int imx226_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	struct v4l2_mbus_framefmt *fmt;

	TRACE
	
	if (format->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt = v4l2_subdev_get_try_format(&sensor->sd, cfg,
						 format->pad);
	else
		fmt = &sensor->fmt;

	fmt->reserved[1] = (sensor->current_fr == imx226_30_FPS) ? 30 : 15;
	format->format = *fmt;

	mutex_unlock(&sensor->lock);
	return 0;
}

static int imx226_try_fmt_internal(struct v4l2_subdev *sd,
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

static int imx226_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	const struct imx226_mode_info *new_mode;
	struct v4l2_mbus_framefmt *mbus_fmt = &format->format;
	struct v4l2_mbus_framefmt *fmt;
	int ret;

	TRACE
	
	if (format->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);

	if (sensor->streaming) {
		ret = -EBUSY;
		goto out;
	}

	ret = imx226_try_fmt_internal(sd, mbus_fmt,
				      sensor->current_fr, &new_mode);
	if (ret)
		goto out;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt = v4l2_subdev_get_try_format(sd, cfg, 0);
	else
		fmt = &sensor->fmt;

	*fmt = *mbus_fmt;

	if (new_mode != sensor->current_mode) {
		sensor->current_mode = new_mode;
		sensor->pending_mode_change = true;
	}
	if (mbus_fmt->code != sensor->fmt.code)
		sensor->pending_fmt_change = true;

	if (sensor->pending_mode_change || sensor->pending_fmt_change)
		sensor->fmt = *mbus_fmt;
out:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int imx226_set_framefmt(struct imx226_dev *sensor,
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

static int imx226_set_ctrl_hue(struct imx226_dev *sensor, int value)
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

static int imx226_set_ctrl_contrast(struct imx226_dev *sensor, int value)
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

static int imx226_set_ctrl_saturation(struct imx226_dev *sensor, int value)
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

static int imx226_set_ctrl_white_balance(struct imx226_dev *sensor, int awb)
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

static int imx226_set_ctrl_exposure(struct imx226_dev *sensor,
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

static int imx226_set_ctrl_gain(struct imx226_dev *sensor, bool auto_gain)
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

static const char * const test_pattern_menu[] = {
	"Disabled",
	"Color bars",
	"Color bars w/ rolling bar",
	"Color squares",
	"Color squares w/ rolling bar",
};

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

static const u8 test_pattern_val[] = {
	0,
	imx226_TEST_ENABLE | imx226_TEST_BAR_VERT_CHANGE_1 |
		imx226_TEST_BAR,
	imx226_TEST_ENABLE | imx226_TEST_ROLLING |
		imx226_TEST_BAR_VERT_CHANGE_1 | imx226_TEST_BAR,
	imx226_TEST_ENABLE | imx226_TEST_SQUARE,
	imx226_TEST_ENABLE | imx226_TEST_ROLLING | imx226_TEST_SQUARE,
};

static int imx226_set_ctrl_test_pattern(struct imx226_dev *sensor, int value)
{
	TRACE
	
	return imx226_write_reg(sensor, IMX226_REG_PRE_ISP_TEST_SET1,
				test_pattern_val[value]);
}

static int imx226_set_ctrl_light_freq(struct imx226_dev *sensor, int value)
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

static int imx226_set_ctrl_hflip(struct imx226_dev *sensor, int value)
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

static int imx226_set_ctrl_vflip(struct imx226_dev *sensor, int value)
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

static int imx226_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct imx226_dev *sensor = to_imx226_dev(sd);
	int val;

	TRACE
	
	/* v4l2_ctrl_lock() locks our own mutex */

	switch (ctrl->id) {
	case V4L2_CID_AUTOGAIN:
		val = imx226_get_gain(sensor);
		if (val < 0)
			return val;
		sensor->ctrls.gain->val = val;
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		val = imx226_get_exposure(sensor);
		if (val < 0)
			return val;
		sensor->ctrls.exposure->val = val;
		break;
	}

	return 0;
}

static int imx226_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct imx226_dev *sensor = to_imx226_dev(sd);
	int ret;

	TRACE
	
	/* v4l2_ctrl_lock() locks our own mutex */

	/*
	 * If the device is not powered up by the host driver do
	 * not apply any controls to H/W at this time. Instead
	 * the controls will be restored right after power-up.
	 */
	if (sensor->power_count == 0)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_AUTOGAIN:
		ret = imx226_set_ctrl_gain(sensor, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		ret = imx226_set_ctrl_exposure(sensor, ctrl->val);
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		ret = imx226_set_ctrl_white_balance(sensor, ctrl->val);
		break;
	case V4L2_CID_HUE:
		ret = imx226_set_ctrl_hue(sensor, ctrl->val);
		break;
	case V4L2_CID_CONTRAST:
		ret = imx226_set_ctrl_contrast(sensor, ctrl->val);
		break;
	case V4L2_CID_SATURATION:
		ret = imx226_set_ctrl_saturation(sensor, ctrl->val);
		break;
	case V4L2_CID_TEST_PATTERN:
		ret = imx226_set_ctrl_test_pattern(sensor, ctrl->val);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		ret = imx226_set_ctrl_light_freq(sensor, ctrl->val);
		break;
	case V4L2_CID_HFLIP:
		ret = imx226_set_ctrl_hflip(sensor, ctrl->val);
		break;
	case V4L2_CID_VFLIP:
		ret = imx226_set_ctrl_vflip(sensor, ctrl->val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops imx226_ctrl_ops = {
	.g_volatile_ctrl = imx226_g_volatile_ctrl,
	.s_ctrl = imx226_s_ctrl,
};

static int imx226_init_controls(struct imx226_dev *sensor)
{
	const struct v4l2_ctrl_ops *ops = &imx226_ctrl_ops;
	struct imx226_ctrls *ctrls = &sensor->ctrls;
	struct v4l2_ctrl_handler *hdl = &ctrls->handler;
	int ret;

	TRACE
	
	v4l2_ctrl_handler_init(hdl, 32);

	/* we can use our own mutex for the ctrl lock */
	hdl->lock = &sensor->lock;

	/* Auto/manual white balance */
	ctrls->auto_wb = v4l2_ctrl_new_std(hdl, ops,
					   V4L2_CID_AUTO_WHITE_BALANCE,
					   0, 1, 1, 1);
	ctrls->blue_balance = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_BLUE_BALANCE,
						0, 4095, 1, 0);
	ctrls->red_balance = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_RED_BALANCE,
					       0, 4095, 1, 0);
	/* Auto/manual exposure */
	ctrls->auto_exp = v4l2_ctrl_new_std_menu(hdl, ops,
						 V4L2_CID_EXPOSURE_AUTO,
						 V4L2_EXPOSURE_MANUAL, 0,
						 V4L2_EXPOSURE_AUTO);
	ctrls->exposure = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_EXPOSURE,
					    0, 65535, 1, 0);
	/* Auto/manual gain */
	ctrls->auto_gain = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_AUTOGAIN,
					     0, 1, 1, 1);
	ctrls->gain = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_GAIN,
					0, 1023, 1, 0);

	ctrls->saturation = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_SATURATION,
					      0, 255, 1, 64);
	ctrls->hue = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_HUE,
				       0, 359, 1, 0);
	ctrls->contrast = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_CONTRAST,
					    0, 255, 1, 0);
	ctrls->test_pattern =
		v4l2_ctrl_new_std_menu_items(hdl, ops, V4L2_CID_TEST_PATTERN,
					     ARRAY_SIZE(test_pattern_menu) - 1,
					     0, 0, test_pattern_menu);
	ctrls->hflip = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_HFLIP,
					 0, 1, 1, 0);
	ctrls->vflip = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_VFLIP,
					 0, 1, 1, 0);

	ctrls->light_freq =
		v4l2_ctrl_new_std_menu(hdl, ops,
				       V4L2_CID_POWER_LINE_FREQUENCY,
				       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
				       V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	if (hdl->error) {
		ret = hdl->error;
		goto free_ctrls;
	}

	ctrls->gain->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrls->exposure->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_auto_cluster(3, &ctrls->auto_wb, 0, false);
	v4l2_ctrl_auto_cluster(2, &ctrls->auto_gain, 0, true);
	v4l2_ctrl_auto_cluster(2, &ctrls->auto_exp, 1, true);

	sensor->sd.ctrl_handler = hdl;
	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(hdl);
	return ret;
}

static int imx226_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	TRACE
	
	if (fse->pad != 0)
		return -EINVAL;
	if (fse->index >= IMX226_NUM_MODES)
		return -EINVAL;

	fse->min_width =
		imx226_mode_data[fse->index].hact;
	fse->max_width = fse->min_width;
	fse->min_height =
		imx226_mode_data[fse->index].vact;
	fse->max_height = fse->min_height;

	return 0;
}

static int imx226_enum_frame_interval(
	struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_frame_interval_enum *fie)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	int i, j, count;

	TRACE
	
	if (fie->pad != 0)
		return -EINVAL;
	if (fie->index >= IMX226_NUM_FRAMERATES)
		return -EINVAL;

	if (fie->width == 0 || fie->height == 0 || fie->code == 0) {
		pr_warn("[vc-mipi imx226] Please assign pixel format, width and height.\n");
		return -EINVAL;
	}

	fie->interval.numerator = 1;

	count = 0;
	for (i = 0; i < IMX226_NUM_FRAMERATES; i++) {
		for (j = 0; j < IMX226_NUM_MODES; j++) {
			if (fie->width  == imx226_mode_data[j].hact &&
			    fie->height == imx226_mode_data[j].vact &&
			    !imx226_check_valid_mode(sensor, &imx226_mode_data[j], i))
				count++;

			if (fie->index == (count - 1)) {
				fie->interval.denominator = imx226_framerates[i];
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int imx226_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);

	TRACE
	
	mutex_lock(&sensor->lock);
	fi->interval = sensor->frame_interval;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int imx226_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	const struct imx226_mode_info *mode;
	int frame_rate, ret = 0;

	TRACE
	
	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);

	if (sensor->streaming) {
		ret = -EBUSY;
		goto out;
	}

	mode = sensor->current_mode;

	frame_rate = imx226_try_frame_interval(sensor, &fi->interval,
					       mode->hact, mode->vact);
	if (frame_rate < 0) {
		/* Always return a valid frame interval value */
		fi->interval = sensor->frame_interval;
		goto out;
	}

	mode = imx226_find_mode(sensor, frame_rate, mode->hact,
				mode->vact, true);
	if (!mode) {
		ret = -EINVAL;
		goto out;
	}

	if (mode != sensor->current_mode ||
	    frame_rate != sensor->current_fr) {
		sensor->current_fr = frame_rate;
		sensor->frame_interval = fi->interval;
		sensor->current_mode = mode;
		sensor->pending_mode_change = true;
	}
out:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int imx226_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	TRACE
	
	if (code->pad != 0)
		return -EINVAL;
	if (code->index >= ARRAY_SIZE(imx226_formats))
		return -EINVAL;

	code->code = imx226_formats[code->index].code;
	return 0;
}

static int imx226_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx226_dev *sensor = to_imx226_dev(sd);
	struct i2c_client *client = sensor->i2c_client;
	int ret = 0;

	TRACE
	
	mutex_lock(&sensor->lock);

	if (sensor->streaming == !enable) {
		ret = imx226_check_valid_mode(sensor,
					      sensor->current_mode,
					      sensor->current_fr);
		if (ret) {
			dev_err(&client->dev, "[vc-mipi imx226] Not support WxH@fps=%dx%d@%d\n",
				sensor->current_mode->hact,
				sensor->current_mode->vact,
				imx226_framerates[sensor->current_fr]);
			goto out;
		}

		if (enable && sensor->pending_mode_change) {
			ret = imx226_set_mode(sensor);
			if (ret)
				goto out;
		}

		if (enable && sensor->pending_fmt_change) {
			ret = imx226_set_framefmt(sensor, &sensor->fmt);
			if (ret)
				goto out;
			sensor->pending_fmt_change = false;
		}

		if (sensor->ep.bus_type == V4L2_MBUS_CSI2_DPHY)
			ret = imx226_set_stream_mipi(sensor, enable);
		else
			ret = imx226_set_stream_dvp(sensor, enable);

		if (!ret)
			sensor->streaming = enable;
	}
out:
	mutex_unlock(&sensor->lock);
	return ret;
}

static const struct v4l2_subdev_core_ops imx226_core_ops = {
	.s_power = imx226_s_power,
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops imx226_video_ops = {
	.g_frame_interval = imx226_g_frame_interval,
	.s_frame_interval = imx226_s_frame_interval,
	.s_stream = imx226_s_stream,
};

static const struct v4l2_subdev_pad_ops imx226_pad_ops = {
	.enum_mbus_code = imx226_enum_mbus_code,
	.get_fmt = imx226_get_fmt,
	.set_fmt = imx226_set_fmt,
	.enum_frame_size = imx226_enum_frame_size,
	.enum_frame_interval = imx226_enum_frame_interval,
};

static const struct v4l2_subdev_ops imx226_subdev_ops = {
	.core = &imx226_core_ops,
	.video = &imx226_video_ops,
	.pad = &imx226_pad_ops,
};

static int imx226_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	TRACE
	
	return 0;
}

static const struct media_entity_operations imx226_sd_media_ops = {
	.link_setup = imx226_link_setup,
};

static int imx226_get_regulators(struct imx226_dev *sensor)
{
	int i;

	TRACE
	
	for (i = 0; i < IMX226_NUM_SUPPLIES; i++)
		sensor->supplies[i].supply = imx226_supply_name[i];

	return devm_regulator_bulk_get(&sensor->i2c_client->dev,
				       IMX226_NUM_SUPPLIES,
				       sensor->supplies);
}

static int imx226_check_chip_id(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret = 0;
	u16 chip_id;

	TRACE
	
	ret = imx226_set_power_on(sensor);
	if (ret)
		return ret;

	ret = imx226_read_reg16(sensor, IMX226_REG_CHIP_ID, &chip_id);
	if (ret) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: failed to read chip identifier\n",
			__func__);
		goto power_off;
	}

	if (chip_id != 0x5640) {
		dev_err(&client->dev, "[vc-mipi imx226] %s: wrong chip identifier, expected 0x5640, got 0x%x\n",
			__func__, chip_id);
		ret = -ENXIO;
	}

power_off:
	imx226_set_power_off(sensor);
	return ret;
}

static int imx226_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *endpoint;
	struct imx226_dev *sensor;
	struct v4l2_mbus_framefmt *fmt;
	u32 rotation;
	int ret;

	TRACE

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->i2c_client = client;

	/*
	 * default init sequence initialize sensor to
	 * YUV422 UYVY VGA@30fps
	 */
	fmt = &sensor->fmt;
	fmt->code = MEDIA_BUS_FMT_UYVY8_2X8;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
	fmt->width = 640;
	fmt->height = 480;
	fmt->field = V4L2_FIELD_NONE;
	sensor->frame_interval.numerator = 1;
	sensor->frame_interval.denominator = imx226_framerates[imx226_30_FPS];
	sensor->current_fr = imx226_30_FPS;
	sensor->current_mode =
		&imx226_mode_data[IMX226_MODE_VGA_640_480];
	sensor->last_mode = sensor->current_mode;

	sensor->ae_target = 52;

	/* optional indication of physical rotation of sensor */
	ret = fwnode_property_read_u32(dev_fwnode(&client->dev), "rotation",
				       &rotation);
	if (!ret) {
		switch (rotation) {
		case 180:
			sensor->upside_down = true;
			/* fall through */
		case 0:
			break;
		default:
			dev_warn(dev, "[vc-mipi imx226] %u degrees rotation is not supported, ignoring...\n",
				 rotation);
		}
	}

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(&client->dev),
						  NULL);
	if (!endpoint) {
		dev_err(dev, "[vc-mipi imx226] endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &sensor->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "[vc-mipi imx226] Could not parse endpoint\n");
		return ret;
	}

	/* get system clock (xclk) */
	sensor->xclk = devm_clk_get(dev, "xclk");
	if (IS_ERR(sensor->xclk)) {
		dev_err(dev, "[vc-mipi imx226] failed to get xclk\n");
		return PTR_ERR(sensor->xclk);
	}

	sensor->xclk_freq = clk_get_rate(sensor->xclk);
	if (sensor->xclk_freq < IMX226_XCLK_MIN ||
	    sensor->xclk_freq > IMX226_XCLK_MAX) {
		dev_err(dev, "[vc-mipi imx226] xclk frequency out of range: %d Hz\n",
			sensor->xclk_freq);
		return -EINVAL;
	}

	/* request optional power down pin */
	sensor->pwdn_gpio = devm_gpiod_get_optional(dev, "powerdown",
						    GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->pwdn_gpio))
		return PTR_ERR(sensor->pwdn_gpio);

	/* request optional reset pin */
	sensor->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->reset_gpio))
		return PTR_ERR(sensor->reset_gpio);

	v4l2_i2c_subdev_init(&sensor->sd, client, &imx226_subdev_ops);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.ops = &imx226_sd_media_ops;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		return ret;

	ret = imx226_get_regulators(sensor);
	if (ret)
		return ret;

	mutex_init(&sensor->lock);

	//========================================
	ret = vc_mipi_ctrl_setup(sensor);
	if(ret) {
		goto entity_cleanup;
	}
	//========================================

	ret = imx226_check_chip_id(sensor);
	if (ret)
		goto entity_cleanup;

	ret = imx226_init_controls(sensor);
	if (ret)
		goto entity_cleanup;

	ret = v4l2_async_register_subdev_sensor_common(&sensor->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(&sensor->ctrls.handler);
entity_cleanup:
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	return ret;
}

static int imx226_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx226_dev *sensor = to_imx226_dev(sd);

	TRACE
	
	v4l2_async_unregister_subdev(&sensor->sd);
	media_entity_cleanup(&sensor->sd.entity);
	v4l2_ctrl_handler_free(&sensor->ctrls.handler);
	mutex_destroy(&sensor->lock);

	return 0;
}

static const struct i2c_device_id imx226_id[] = {
	{"imx226", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, imx226_id);

static const struct of_device_id imx226_dt_ids[] = {
	{ .compatible = "vc,imx226" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx226_dt_ids);

static struct i2c_driver imx226_i2c_driver = {
	.driver = {
		.name  = "imx226",
		.of_match_table	= imx226_dt_ids,
	},
	.id_table = imx226_id,
	.probe_new = imx226_probe,
	.remove   = imx226_remove,
};

module_i2c_driver(imx226_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");
