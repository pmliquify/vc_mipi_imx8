#include "vc_mipi_sensor.h"
#include "vc_mipi_i2c.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_sensor.c --->  %s : %d", __FUNCTION__, __LINE__);
// #define TRACE

#define HIGH_BYTE(value) (__u8)((value >> 8) & 0xff)
#define LOW_BYTE(value)  (__u8)(value & 0xff)

#define IMX226_SEN_REG_EXPO_H	0x0000
#define IMX226_SEN_REG_EXPO_M	0x000C
#define IMX226_SEN_REG_EXPO_L	0x000B
#define IMX226_SEN_REG_GAIN_H	0x000A
#define IMX226_SEN_REG_GAIN_L	0x0009

#define EXPOSURE_TIME_MIN_226  	160		// expMin0
#define EXPOSURE_TIME_MIN2_226 	74480		// expMin1
#define EXPOSURE_TIME_MAX_226  	10000000	// expMax
#define NRLINES_226            	3694		// nrLines
#define TOFFSET_226  		47563  		// tOffset (U32)(2.903 * 16384.0)
#define H1PERIOD_226 		327680 		// h1Period 20.00us => (U32)(20.000 * 16384.0)
#define VMAX_226     		3728		// vMax


int vc_mipi_sen_set_exposure(struct i2c_client *client, int value)
{
        TRACE

        // // exposure = 5 + ((unsigned long long)(priv->exposure_time * 16384) - tOffset)/h1Period;
        // u64 divresult;
        // u32 divisor ,remainder;
        // divresult = ((unsigned long long)priv->exposure_time * 16384) - tOffset;
        // divisor   = h1Period;
        // remainder = (u32)(do_div(divresult,divisor)); // caution: division result value at dividend!
        // exposure = 5 + (u32)divresult;

        // dev_info(&client->dev, "[vc-mipi driver] VMAX = %d \n",exposure);

        // if(sen_reg(priv, EXPOSURE_MIDDLE))
        //     ret  = vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_MIDDLE), 0x00);
        // if(sen_reg(priv, EXPOSURE_LOW))
        //     ret |= vc_mipi_common_reg_write(client, sen_reg(priv, EXPOSURE_LOW), 0x04);

        // ret |= vc_mipi_common_reg_write(client, 0x7006, (exposure >> 16) & 0x07);
        // ret |= vc_mipi_common_reg_write(client, 0x7005, (exposure >>  8) & 0xff);
        // ret |= vc_mipi_common_reg_write(client, 0x7004,  exposure        & 0xff);

        return 0;
}

int vc_mipi_sen_set_gain(struct i2c_client *client, int value)
{
	struct device *dev = &client->dev;
	int ret = 0;

	// TRACE

	ret = i2c_write_reg(client, IMX226_SEN_REG_GAIN_H, HIGH_BYTE(value));
        ret |= i2c_write_reg(client, IMX226_SEN_REG_GAIN_L, LOW_BYTE(value));
        if (ret) 
                dev_err(dev, "[vc-mipi sensor] %s: Couldn't set 'Gain' (error=%d)\n", __FUNCTION__, ret);

	return ret;
}