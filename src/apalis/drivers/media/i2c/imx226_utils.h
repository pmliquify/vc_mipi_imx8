#ifndef _IMX226_UTILS_H
#define _IMX226_UTILS_H

#include "imx226.h"

unsigned long imx226_compute_sys_clk(struct imx226_dev *sensor, u8 pll_prediv, u8 pll_mult, u8 sysdiv);
unsigned long imx226_calc_sys_clk(struct imx226_dev *sensor, unsigned long rate, u8 *pll_prediv, u8 *pll_mult, u8 *sysdiv);
int imx226_set_mipi_pclk(struct imx226_dev *sensor, unsigned long rate);
unsigned long imx226_calc_pclk(struct imx226_dev *sensor, unsigned long rate, u8 *pll_prediv, u8 *pll_mult, u8 *sysdiv, u8 *pll_rdiv, u8 *bit_div, u8 *pclk_div);
int imx226_set_dvp_pclk(struct imx226_dev *sensor, unsigned long rate);
int imx226_set_jpeg_timings(struct imx226_dev *sensor, const struct imx226_mode_info *mode);
int imx226_set_timings(struct imx226_dev *sensor, const struct imx226_mode_info *mode);
int imx226_load_regs(struct imx226_dev *sensor, const struct imx226_mode_info *mode);
int imx226_set_autoexposure(struct imx226_dev *sensor, bool on);
int imx226_set_exposure(struct imx226_dev *sensor, u32 exposure);
int imx226_set_gain(struct imx226_dev *sensor, int gain);
int imx226_set_autogain(struct imx226_dev *sensor, bool on);
int imx226_get_sysclk(struct imx226_dev *sensor);
int imx226_set_night_mode(struct imx226_dev *sensor);
int imx226_get_hts(struct imx226_dev *sensor);
int imx226_get_vts(struct imx226_dev *sensor);
int imx226_set_vts(struct imx226_dev *sensor, int vts);
int imx226_get_light_freq(struct imx226_dev *sensor);
int imx226_set_bandingfilter(struct imx226_dev *sensor);
int imx226_set_ae_target(struct imx226_dev *sensor, int target);
int imx226_get_binning(struct imx226_dev *sensor);
int imx226_set_binning(struct imx226_dev *sensor, bool enable);
int imx226_set_virtual_channel(struct imx226_dev *sensor);
int imx226_set_mode_exposure_calc(struct imx226_dev *sensor, const struct imx226_mode_info *mode);
int imx226_set_mode_direct(struct imx226_dev *sensor, const struct imx226_mode_info *mode);
int imx226_restore_mode(struct imx226_dev *sensor);
void imx226_power(struct imx226_dev *sensor, bool enable);
void imx226_reset(struct imx226_dev *sensor);
int imx226_set_power_on(struct imx226_dev *sensor);
void imx226_set_power_off(struct imx226_dev *sensor);
int imx226_set_power_mipi(struct imx226_dev *sensor, bool on);
int imx226_set_power_dvp(struct imx226_dev *sensor, bool on);

#endif // _IMX226_UTILS_H