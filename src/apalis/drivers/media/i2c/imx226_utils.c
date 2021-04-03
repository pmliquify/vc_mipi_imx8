#include "imx226_sd_utils.h"
#include "imx226_utils.h"
#include "imx226_i2c.h"

#define TRACE printk("        TRACE [vc-mipi] imx226_utils.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

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

unsigned long imx226_compute_sys_clk(struct imx226_dev *sensor,
					    u8 pll_prediv, u8 pll_mult,
					    u8 sysdiv)
{
	unsigned long sysclk = sensor->xclk_freq / pll_prediv * pll_mult;

	//TRACE
	
	/* PLL1 output cannot exceed 1GHz. */
	if (sysclk / 1000000 > 1000)
		return 0;

	return sysclk / sysdiv;
}

unsigned long imx226_calc_sys_clk(struct imx226_dev *sensor,
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
int imx226_set_mipi_pclk(struct imx226_dev *sensor,
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

unsigned long imx226_calc_pclk(struct imx226_dev *sensor,
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

int imx226_set_dvp_pclk(struct imx226_dev *sensor, unsigned long rate)
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
int imx226_set_jpeg_timings(struct imx226_dev *sensor,
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
int imx226_set_timings(struct imx226_dev *sensor,
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

int imx226_load_regs(struct imx226_dev *sensor,
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

int imx226_set_autoexposure(struct imx226_dev *sensor, bool on)
{
	TRACE
	
	return imx226_mod_reg(sensor, IMX226_REG_AEC_PK_MANUAL,
			      BIT(0), on ? 0 : BIT(0));
}

/* write exposure, given number of line periods */
int imx226_set_exposure(struct imx226_dev *sensor, u32 exposure)
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

int imx226_set_gain(struct imx226_dev *sensor, int gain)
{
	TRACE
	
	return imx226_write_reg16(sensor, IMX226_REG_AEC_PK_REAL_GAIN,
				  (u16)gain & 0x3ff);
}

int imx226_set_autogain(struct imx226_dev *sensor, bool on)
{
	TRACE
	
	return imx226_mod_reg(sensor, IMX226_REG_AEC_PK_MANUAL,
			      BIT(1), on ? 0 : BIT(1));
}

int imx226_get_sysclk(struct imx226_dev *sensor)
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

int imx226_set_night_mode(struct imx226_dev *sensor)
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

int imx226_get_hts(struct imx226_dev *sensor)
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

int imx226_get_vts(struct imx226_dev *sensor)
{
	u16 vts;
	int ret;

	TRACE

	ret = imx226_read_reg16(sensor, IMX226_REG_TIMING_VTS, &vts);
	if (ret)
		return ret;
	return vts;
}

int imx226_set_vts(struct imx226_dev *sensor, int vts)
{
	TRACE
	
	return imx226_write_reg16(sensor, IMX226_REG_TIMING_VTS, vts);
}

int imx226_get_light_freq(struct imx226_dev *sensor)
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

int imx226_set_bandingfilter(struct imx226_dev *sensor)
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

int imx226_set_ae_target(struct imx226_dev *sensor, int target)
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

int imx226_get_binning(struct imx226_dev *sensor)
{
	u8 temp;
	int ret;

	TRACE
	
	ret = imx226_read_reg(sensor, IMX226_REG_TIMING_TC_REG21, &temp);
	if (ret)
		return ret;

	return temp & BIT(0);
}

int imx226_set_binning(struct imx226_dev *sensor, bool enable)
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

int imx226_set_virtual_channel(struct imx226_dev *sensor)
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

/*
 * sensor changes between scaling and subsampling, go through
 * exposure calculation
 */
int imx226_set_mode_exposure_calc(struct imx226_dev *sensor,
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
int imx226_set_mode_direct(struct imx226_dev *sensor,
				  const struct imx226_mode_info *mode)
{
	TRACE
	
	if (!mode->reg_data)
		return -EINVAL;

	/* Write capture setting */
	return imx226_load_regs(sensor, mode);
}



int imx226_set_framefmt(struct imx226_dev *sensor,
			       struct v4l2_mbus_framefmt *format);

/* restore the last set video mode after chip power-on */
int imx226_restore_mode(struct imx226_dev *sensor)
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

void imx226_power(struct imx226_dev *sensor, bool enable)
{
	TRACE
	
	gpiod_set_value_cansleep(sensor->pwdn_gpio, enable ? 0 : 1);
}

void imx226_reset(struct imx226_dev *sensor)
{
	//TRACE
	
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

int imx226_set_power_on(struct imx226_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret;

	//TRACE
	
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

void imx226_set_power_off(struct imx226_dev *sensor)
{
	TRACE
	
	imx226_power(sensor, false);
	regulator_bulk_disable(IMX226_NUM_SUPPLIES, sensor->supplies);
	clk_disable_unprepare(sensor->xclk);
}

int imx226_set_power_mipi(struct imx226_dev *sensor, bool on)
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

int imx226_set_power_dvp(struct imx226_dev *sensor, bool on)
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