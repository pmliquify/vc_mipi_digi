
#include "vc_mipi_modules.h"
#include "vc_mipi_imx226.h"
#include "vc_mipi_imx327.h"

void vc_init_imx226_ctrl(struct vc_ctrl *ctrl)
{
	struct device *dev = &ctrl->client_sen->dev;

	dev_dbg(dev, "%s(): Initialising module control for IMX226\n", __FUNCTION__);

	ctrl->mod_id		= MOD_ID_IMX226;
	ctrl->mod_i2c_addr	= 0x10;

	ctrl->start_table 	= (struct vc_table_entry *)imx226_start;
	ctrl->stop_table 	= (struct vc_table_entry *)imx226_stop;
	ctrl->mode_table 	= (struct vc_table_entry *)imx226_mode_3840_3040;

	ctrl->fmts 		= imx226_color_fmts;
	ctrl->fmts_size		= imx226_color_fmts_size;
	ctrl->default_fmt	= 0;
	ctrl->o_width 		= IMX226_DX;
	ctrl->o_height 		= IMX226_DY;

	ctrl->sen_reg_vmax_l 	= IMX226_SEN_REG_VMAX_L;
	ctrl->sen_reg_vmax_m 	= IMX226_SEN_REG_VMAX_M;
	ctrl->sen_reg_vmax_h 	= IMX226_SEN_REG_VMAX_H;	
	ctrl->sen_reg_expo_l 	= IMX226_SEN_REG_EXPO_L;
	ctrl->sen_reg_expo_m 	= IMX226_SEN_REG_EXPO_M;
	ctrl->sen_reg_expo_h 	= IMX226_SEN_REG_EXPO_H;
	ctrl->sen_reg_gain_l 	= IMX226_SEN_REG_GAIN_L;
	ctrl->sen_reg_gain_m 	= IMX226_SEN_REG_GAIN_M;

	ctrl->expo_time_min1 	= IMX226_EXPO_TIME_MIN1;
	ctrl->expo_time_min2 	= IMX226_EXPO_TIME_MIN2;
	ctrl->expo_time_max 	= IMX226_EXPO_TIME_MAX;
	ctrl->expo_vmax 	= IMX226_EXPO_VMAX;
	ctrl->expo_toffset 	= IMX226_EXPO_TOFFSET;
	ctrl->expo_h1period 	= IMX226_EXPO_H1PERIOD;

	ctrl->csi_lanes		= 2;
	ctrl->default_mode 	= 0x00;			// reg2[7:0] = 0x00 :  8bit, 2 lanes, streaming
	ctrl->io_control 	= 1;
}

void vc_init_imx327_ctrl(struct vc_ctrl *ctrl)
{
	struct device *dev = &ctrl->client_sen->dev;

	dev_dbg(dev, "%s(): Initialising module control for IMX327\n", __FUNCTION__);
	
	ctrl->mod_id		= MOD_ID_IMX327;
	ctrl->mod_i2c_addr	= 0x10;

	ctrl->start_table 	= (struct vc_table_entry *)imx327_start;
	ctrl->stop_table 	= (struct vc_table_entry *)imx327_stop;
	ctrl->mode_table 	= (struct vc_table_entry *)imx327_mode_1920_1080;

	ctrl->fmts 		= imx327_color_fmts;
	ctrl->fmts_size		= imx327_color_fmts_size;
	ctrl->default_fmt	= 0;
	ctrl->o_width 		= IMX327_DX;
	ctrl->o_height 		= IMX327_DY;

	ctrl->sen_reg_vmax_l 	= IMX327_SEN_REG_VMAX_L;
	ctrl->sen_reg_vmax_m 	= IMX327_SEN_REG_VMAX_M;
	ctrl->sen_reg_vmax_h 	= IMX327_SEN_REG_VMAX_H;
	ctrl->sen_reg_expo_l 	= IMX327_SEN_REG_EXPO_L;
	ctrl->sen_reg_expo_m 	= IMX327_SEN_REG_EXPO_M;
	ctrl->sen_reg_expo_h 	= IMX327_SEN_REG_EXPO_H;
	ctrl->sen_reg_gain_l 	= IMX327_SEN_REG_GAIN_L;
	ctrl->sen_reg_gain_m 	= IMX327_SEN_REG_GAIN_M;
	
	ctrl->expo_time_min1 	= IMX327_EXPO_TIME_MIN1;
	ctrl->expo_time_min2 	= IMX327_EXPO_TIME_MIN2;
	ctrl->expo_time_max 	= IMX327_EXPO_TIME_MAX;
	ctrl->expo_vmax 	= IMX327_EXPO_VMAX;
	ctrl->expo_toffset 	= IMX327_EXPO_TOFFSET;
	ctrl->expo_h1period 	= IMX327_EXPO_H1PERIOD;
	
	ctrl->csi_lanes		= 2;
	ctrl->default_mode 	= 0x00;			// reg2[7:0] = 0x00 : 10bit streaming, 2 lanes
	ctrl->io_control 	= 0;
}

int vc_mod_ctrl_init(struct vc_ctrl* ctrl, struct vc_desc* desc)
{
	struct device *dev = &ctrl->client_sen->dev;

	switch(desc->mod_id) {
	case MOD_ID_IMX226: 
		vc_init_imx226_ctrl(ctrl); 
		break;
	case MOD_ID_IMX327: 
		vc_init_imx327_ctrl(ctrl); 
		break;
	default:
		dev_err(dev, "%s(): Detected Module not supported!\n", __FUNCTION__);
		return 1;
	}
	return 0;
}


// *** Code Snippets **********************************************************

// camera->model = IMX226_MODEL_COLOR;
// if(camera->model == IMX226_MODEL_MONOCHROME) {
// 	camera->vc_data_fmts      = imx226_mono_fmts;
// 	camera->vc_data_fmts_size = imx226_mono_fmts_size;

// } else {
// 	camera->vc_data_fmts      = imx226_color_fmts;
// 	camera->vc_data_fmts_size = imx226_color_fmts_size;
// }

// if(camera->model == IMX183_MODEL_MONOCHROME || camera->model == IMX183_MODEL_COLOR) {
// 	camera->sen_clk = 72000000;     // clock-frequency: default=54Mhz imx183=72Mhz
// } else {
// 	camera->sen_clk = 54000000;     // clock-frequency: default=54Mhz imx183=72Mhz
// }

// ****************************************************************************