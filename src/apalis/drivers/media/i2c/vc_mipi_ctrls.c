#include "vc_mipi_ctrls.h"
#include "vc_mipi_driver.h"

#define TRACE printk("        TRACE [vc-mipi] vc_mipi_ctrls.c --->  %s : %d", __FUNCTION__, __LINE__);
//#define TRACE

// --- V4L2 Controls ------------------------------------------------------

// enum vc_rom_table_sreg_names {
//     MODEL_ID_HIGH = 0,
//     MODEL_ID_LOW,
//     CHIP_REV,
//     IDLE,
//     H_START_HIGH,
//     H_START_LOW,
//     V_START_HIGH,
//     V_START_LOW,
//     H_SIZE_HIGH,
//     H_SIZE_LOW,
//     V_SIZE_HIGH,
//     V_SIZE_LOW,
//     H_OUTPUT_HIGH,
//     H_OUTPUT_LOW,
//     V_OUTPUT_HIGH,
//     V_OUTPUT_LOW,
//     EXPOSURE_HIGH,
//     EXPOSURE_MIDDLE,
//     EXPOSURE_LOW,
//     GAIN_HIGH,
//     GAIN_LOW,
//     RESERVED1,
//     RESERVED2,
//     RESERVED3,
//     RESERVED4,
//     RESERVED5,
//     RESERVED6,
//     RESERVED7,
// };

// #define sen_reg(sensor, addr) (*((u16 *)(&sensor->rom_table.regs[(addr)*2])))

// struct vc_mipi_dev *i2c_to_vc_mipi_dev(const struct i2c_client *client)
// {
//     return container_of(i2c_get_clientdata(client), struct vc_mipi_dev, sd);
// }

int imx_set_gain(struct i2c_client *client)
{
	// struct vc_mipi_dev *sensor = i2c_to_vc_mipi_dev(client);   
	// struct device *dev = &sensor->i2c_client->dev;
	int ret = 0;

	TRACE

	// if(sensor->digital_gain < sensor->sen_pars.gain_min)  sensor->digital_gain = sensor->sen_pars.gain_min;
	// if(sensor->digital_gain > sensor->sen_pars.gain_max)  sensor->digital_gain = sensor->sen_pars.gain_max;

	// if(sen_reg(sensor, GAIN_HIGH))
	// 	ret  = reg_write(client, sen_reg(sensor, GAIN_HIGH), (sensor->digital_gain >> 8) & 0xff);
	// if(sen_reg(sensor, GAIN_LOW))
        // 	ret |= reg_write(client, sen_reg(sensor, GAIN_LOW), sensor->digital_gain & 0xff);
	// if(ret)
	// 	dev_err(dev, "[vc-mipi driver] %s: error=%d\n", __func__, ret);

	return ret;
}

int vc_mipi_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vc_mipi_driver *driver = container_of(ctrl->handler, struct vc_mipi_driver, ctrls.handler);
	struct i2c_client *client = driver->module.client_sen;
	struct device* dev = &client->dev;
	int ret = 0;

	TRACE

        dev_err(dev, "[vc-mipi driver] %s: ctrl(id:0x%x, name:%s, val:0x%x)\n", __func__, ctrl->id, ctrl->name, ctrl->val);	

    	switch (ctrl->id)  {
	case V4L2_CID_GAIN:
		dev_err(dev, "[vc-mipi driver] %s: Set %s=%d\n", __func__, ctrl->name, ctrl->val);
		// sensor->digital_gain = ctrl->val;
        	// ret = imx_set_gain(client);
        	break;

	case V4L2_CID_EXPOSURE:
		dev_err(dev, "[vc-mipi driver] %s: Set %s=%d\n", __func__, ctrl->name, ctrl->val);
        	// sensor->exposure_time = ctrl->val;
        	// ret = imx_set_exposure(client);
        	break;
	default:
        	dev_err(dev, "[vc-mipi driver] %s: ctrl(id:0x%x,val:0x%x) is not handled\n", __func__, ctrl->id, ctrl->val);
        	ret = -EINVAL;
        	break;
    }

    return ret;
}

int vc_mipi_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	TRACE

	// switch (ctrl->id) {
	// case V4L2_CID_AUTOGAIN:
	// 	break;

	// case V4L2_CID_EXPOSURE_AUTO: 
	// 	break;
	// }

	return 0;
}


const struct v4l2_ctrl_ops vc_mipi_ctrl_ops = {
	.s_ctrl = vc_mipi_set_ctrl,
	.g_volatile_ctrl = vc_mipi_g_volatile_ctrl,
};

struct v4l2_ctrl_config ctrl_config_list[] =
{
{
     	.ops = &vc_mipi_ctrl_ops,
	.id = V4L2_CID_GAIN,
	.name = "Gain",			// Do not change the name field for the controls!
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.min = 0,
	.max = 0xfff,
	.def = 0,
	.step = 1,
},
{
	.ops = &vc_mipi_ctrl_ops,
	.id = V4L2_CID_EXPOSURE,
	.name = "Exposure",		// Do not change the name field for the controls!
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.min = 0,
	.max = 16000000,
	.def = 0,
	.step = 1,
},
};

int vc_mipi_ctrls_init(struct v4l2_subdev *sd, struct vc_mipi_ctrls *ctrls, struct device *dev)
{
	const struct v4l2_ctrl_ops *ops = &vc_mipi_ctrl_ops;
    	struct v4l2_ctrl_handler *handler = &ctrls->handler;
    	int ret;

	TRACE
	
    	ret = v4l2_ctrl_handler_init(handler, 1);
    	if (ret) {
		dev_err(dev, "[vc-mipi driver] %s: Failed to init control handler\n",  __func__);
		return ret;
	}

	// ctrl_config_list[0].ops = &vc_mipi_ctrl_ops;
	// ctrls->gain = v4l2_ctrl_new_custom(handler, &ctrl_config_list[0], NULL);
	// if (ctrls->gain == NULL) {
	// 	dev_err(dev, "[vc-mipi driver] %s: Failed to init %s ctrl\n",  __func__, ctrl_config_list[0].name);
        // }
	ctrls->gain = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 0, 0xfff, 1, 0);

	// ctrl_config_list[1].ops = &vc_mipi_ctrl_ops;
	// ctrls->exposure = v4l2_ctrl_new_custom(handler, &ctrl_config_list[1], NULL);
	// if (ctrls->gain == NULL) {
	// 	dev_err(dev, "[vc-mipi driver] %s: Failed to init %s ctrl\n",  __func__, ctrl_config_list[1].name);
        // }
	ctrls->exposure = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0, 16000000, 1, 0);

	if (handler->error) {
		ret = handler->error;
		dev_err(dev, "[vc-mipi driver] %s: control init failed (%d)\n", __func__, ret);
		goto error;
	}

	sd->ctrl_handler = handler;

	return 0;

error:
	v4l2_ctrl_handler_free(handler);
	return ret;
}