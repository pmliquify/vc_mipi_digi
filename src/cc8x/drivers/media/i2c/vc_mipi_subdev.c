#include "vc_mipi_subdev.h"
#include "vc_mipi_camera.h"
#include <linux/delay.h>

// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_sd_s_power(struct v4l2_subdev *sd, int on)
{
	struct vc_camera *camera = to_vc_camera(sd);
	struct vc_ctrl *ctrl = &camera->ctrl;
	struct vc_state *state = &camera->state;
	struct i2c_client *client_mod = camera->ctrl.client_mod;
	struct device *dev_mod = &client_mod->dev;
	int ret;

	dev_dbg(dev_mod, "%s(): Set power: %s\n", __FUNCTION__, on ? "on" : "off");

	ret = vc_mod_set_power(client_mod, on);
	if (ret) 
		return ret;

	if (on) {
		dev_dbg(dev_mod, "%s(): Restore all controls ...\n", __FUNCTION__);
		// Restore controls. This function calls vc_sd_s_ctrl for all controls.
		ret = v4l2_ctrl_handler_setup(sd->ctrl_handler);
		if (ret) {
			dev_err(dev_mod, "%s: Failed to restore controls\n", __FUNCTION__);
			return ret;
		}
	}

	state->power_on = on;

	if (on) {
		int exposure = 10000;
		if (state->ext_trig) {
			ret = vc_mod_set_exposure(client_mod, exposure, ctrl->sen_clk);
		} else {
			ret = vc_sen_set_exposure(ctrl, exposure);
		}
		ret |= vc_sen_set_gain(ctrl, 0);
		ret |= vc_mod_set_mode(client_mod, state->mode);
	}

	return 0;
}

int vc_sd_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	return v4l2_queryctrl(sd->ctrl_handler, qc);
}

int vc_sd_try_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	return 0;
}

int vc_sd_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct vc_camera *camera = to_vc_camera(sd);
	struct vc_state *state = &camera->state;
	struct i2c_client *client_mod = camera->ctrl.client_mod;
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl;

	// Settings
	// 0=free run, 1=ext. trigger, 2=trigger self test
	int sen_clk = 54000000; // sen_clk default=54Mhz, imx183=72Mhz

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL)
		return -EINVAL;

	if (control->value < ctrl->minimum || control->value > ctrl->maximum) {
		dev_err(dev, "%s(): Control value '%s' %d exceeds allowed range (%lld - %lld)\n", __FUNCTION__,
			ctrl->name, control->value, ctrl->minimum, ctrl->maximum);
		return -EINVAL;
	}

	dev_dbg(dev, "%s(): Set control '%s' to value %d\n", __FUNCTION__, ctrl->name, control->value);

	// Store value in ctrl to get and restore ctrls later and after power off.
	v4l2_ctrl_s_ctrl(ctrl, control->value);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		if (state->ext_trig) {
			return vc_mod_set_exposure(client_mod, control->value, sen_clk);
		} else {
			return vc_sen_set_exposure(&camera->ctrl, control->value);
		}

	case V4L2_CID_GAIN:
		return vc_sen_set_gain(&camera->ctrl, control->value);
	}

	dev_warn(dev, "%s(): ctrl(id:0x%x, val:0x%x) is not handled\n", __FUNCTION__, ctrl->id, ctrl->val);
	return -EINVAL;
}

int vc_sd_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL)
		return -EINVAL;

	dev_dbg(dev, "%s(): Get control '%s' value %d\n", __FUNCTION__, ctrl->name, control->value);
	return v4l2_g_ctrl(sd->ctrl_handler, control);
}

__s32 vc_sd_get_ctrl_value(struct v4l2_subdev *sd, __u32 id)
{
	struct v4l2_control control;
	control.id = id;
	vc_sd_g_ctrl(sd, &control);
	// TODO: Errorhandling
	return control.value;
}

int vc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	// struct v4l2_subdev *sd = ctrl->priv;
	// struct v4l2_control control;
	// control.id = ctrl->id;
	// control.value = ctrl->val;
	// return vc_sd_s_ctrl(sd, &control);
	return 0;
}


// --- v4l2_subdev_video_ops ---------------------------------------------------

// int vc_sd_g_frame_interval(struct v4l2_subdev *sd,
// 				   struct v4l2_subdev_frame_interval *fi)
// {
// 	return 0;
// }

// int vc_sd_s_frame_interval(struct v4l2_subdev *sd,
// 				   struct v4l2_subdev_frame_interval *fi)
// {
// 	return 0;
// }

int vc_sd_set_mode(struct vc_camera *camera)
{
	struct v4l2_subdev *sd = &camera->sd;
	struct device *dev = sd->dev;
	struct vc_ctrl *ctrl = &camera->ctrl;
	struct vc_state *state = &camera->state;
	struct i2c_client *client_sen = camera->ctrl.client_sen;
	struct i2c_client *client_mod = camera->ctrl.client_mod;
	int ret = 0;
	int code = 0;
	int mode = 0;

	dev_dbg(dev, "%s(): Set sensor mode\n", __FUNCTION__);
	// Test

	/*............. Get pixel format */
	// TODO: mx6s_get_pix_fmt(&pix);
	// pix_fmt = V4L2_PIX_FMT_SRGGB8;
	code = state->fmt->code;

	switch(ctrl->mod_id) {
	case MOD_ID_IMX226:
		if (code == V4L2_PIX_FMT_GREY || code == V4L2_PIX_FMT_SRGGB8) {
			mode = 0; // 8-bit

		} else if (code == V4L2_PIX_FMT_Y10 || code == V4L2_PIX_FMT_SRGGB10) {
			mode = 1; // 10-bit

		} else if (code == V4L2_PIX_FMT_Y12 || code == V4L2_PIX_FMT_SRGGB12) {
			mode = 2; // 12-bit
		}
		if (ctrl->csi_lanes == 4) {
		 	mode += 6;
		}
		if (state->ext_trig) {
			mode += 3;
		}
		break;

	case MOD_ID_IMX327:
		mode = 0;
		if (ctrl->csi_lanes == 4) {
		 	mode = 1;
		}
		break;
	}

	// Change VC MIPI sensor mode
	if (state->mode != mode) {
		dev_dbg(dev, "%s(): Mode has to be changed.\n", __func__);

		// TODO: Check if it is realy necessary to reset the module.
		ret = vc_mod_reset_module(client_mod, mode);
		ret |= vc_sen_set_gain(&camera->ctrl, vc_sd_get_ctrl_value(sd, V4L2_CID_GAIN));
		ret |= vc_sen_set_exposure(&camera->ctrl, vc_sd_get_ctrl_value(sd, V4L2_CID_EXPOSURE));
		if (ret) {
			dev_err(dev, "%s(): Unable to set mode: 0x%02x (error=%d)\n", __func__, mode, ret);
			return ret;
		}
		dev_dbg(dev, "%s(): New mode: 0x%02x\n", __func__, mode);
		state->mode = mode;
	
	} else {
		dev_dbg(dev, "%s(): Current mode: 0x%02x\n", __func__, state->mode);
	}

	dev_dbg(dev, "%s(): Set sensor ROI\n", __func__);
	return vc_sen_write_table(client_sen, camera->ctrl.mode_table);
}

int vc_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vc_camera *camera = to_vc_camera(sd);
	struct vc_ctrl *ctrl = &camera->ctrl;
	struct vc_state *state = &camera->state;
	struct device *dev = sd->dev;
	int ret = 0;

	dev_dbg(dev, "%s(): Set streaming: %s\n", __FUNCTION__, enable ? "on" : "off");

	if (enable) {
		if (state->streaming == 1) {
			dev_warn(dev, "%s(): Sensor is already streaming!\n", __FUNCTION__);
			ret = vc_sen_stop_stream(ctrl, state);
		}

		ret = vc_sd_set_mode(camera);
		ret |= vc_sen_start_stream(ctrl, state);
		if (ret == 0)
			state->streaming = 1;

	} else {
		ret = vc_sen_stop_stream(ctrl, state);
		if (ret == 0)
			state->streaming = 0;
	}

	return ret;
}

// --- v4l2_subdev_pad_ops ---------------------------------------------------

// int vc_sd_enum_mbus_code(struct v4l2_subdev *sd,
// 				struct v4l2_subdev_pad_config *cfg,
// 				struct v4l2_subdev_mbus_code_enum *code)
// {
// 	struct vc_camera *camera = to_vc_camera(sd);

//     	if (code->pad || code->index >= camera->vc_data_fmts_size)
//         	return -EINVAL;

//     	code->code = 			camera->vc_data_fmts[code->index].code;

// 	return 0;
// }

int vc_sd_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_camera *camera = to_vc_camera(sd);
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct device *dev = sd->dev;

	mf->code = camera->state.fmt->code;
	// mf->width = camera->state.width;
	// mf->height = camera->state.height;
	mf->width = camera->ctrl.o_width;
	mf->height = camera->ctrl.o_height;

	dev_dbg(dev, "%s(): v4l2_mbus_framefmt <- (code: 0x%04x, width: %u, height: %u)\n", __FUNCTION__, mf->code,
		mf->width, mf->height);

	// struct vc_camera *camera = to_vc_camera(sd);
	// struct v4l2_mbus_framefmt *mf = &format->format;

	// if (format->pad) {
	// 	return -EINVAL;
	// }

	// mf->code = 			camera->fmt->code;
	// mf->colorspace  = 		camera->fmt->colorspace;
	// mf->field = V4L2_FIELD_NONE;
	// mf->width = 			camera->pix.width;
	// mf->height = 			camera->pix.height;

	return 0;
}

// static const struct vc_datafmt *vc_find_datafmt(struct v4l2_subdev *sd, __u32 code)
// {
// 	struct vc_camera *camera = to_vc_camera(sd);
// 	struct device* dev = sd->dev;
// 	struct vc_datafmt *data_fmt;
// 	int i;

// 	dev_info(dev, "[vc-mipi camera] %s: Try to find vc_datafmt with code: 0x%08x\n", __FUNCTION__, code);

// 	for (i = 0; i < camera->vc_data_fmts_size; i++) {
// 		if (camera->vc_data_fmts[i].code == code) {
// 			data_fmt = &camera->vc_data_fmts[i];
// 			dev_info(dev, "[vc-mipi camera] %s: Found vc_datafmt with colorspace: %u\n", __FUNCTION__, data_fmt->colorspace);
// 			return data_fmt;
// 		}
// 	}

// 	dev_info(dev, "[vc-mipi camera] %s: No vc_datafmt found!\n", __FUNCTION__);

// 	return NULL;
// }

// static int get_capturemode(struct v4l2_subdev *sd, int width, int height)
// {
// 	int i;

// 	for (i = 0; i < ARRAY_SIZE(vc_valid_res); i++) {
// 		if ((vc_valid_res[i].width == width) && (vc_valid_res[i].height == height)) {
// 		return i;
// 		}
// 	}
// 	return -1;
// }

int vc_sd_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_camera *camera = to_vc_camera(sd);
	struct vc_ctrl *ctrl = &camera->ctrl;
	struct vc_state *state = &camera->state;
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct device *dev = sd->dev;
	int i;
	// const struct vc_datafmt *fmt = vc_find_datafmt(sd, mf->code);
	// int capturemode;

	dev_dbg(dev, "%s(): v4l2_mbus_framefmt -> (code: 0x%04x, width: %u, height: %u)\n", __FUNCTION__, mf->code,
		mf->width, mf->height);

	state->fmt = NULL;
	for (i = 0; i < ctrl->fmts_size; i++) {
		struct vc_framefmt *fmt = &ctrl->fmts[i];
		dev_dbg(dev, "%s(): Checking format: 0x%04x", __FUNCTION__, fmt->code);
		if(mf->code == fmt->code) {
			state->fmt = fmt;
		}
	}
	if (state->fmt == NULL) {
		struct vc_framefmt *fmt = &ctrl->fmts[ctrl->default_fmt];
		dev_err(dev, "%s(): Format 0x%04x not supported! (Set default format: 0x%04x)\n", __FUNCTION__, mf->code, fmt->code);
		state->fmt = fmt;
	}

	// if (mf->width > camera->ctrl.o_width) {
	// 	camera->state.width = camera->ctrl.o_width;
	// } else if (mf->width < 0) {
	// 	camera->state.width = 0;
	// } else {
	// 	camera->state.width = mf->width;
	// }
	// dev_dbg(dev, "%s(): Set width: %u\n", __FUNCTION__, camera->state.width);
	
	// if (mf->height > camera->ctrl.o_height) {
	// 	camera->state.height = camera->ctrl.o_height;
	// } else if (mf->height < 0) {
	// 	camera->state.height = 0;
	// } else {
	// 	camera->state.height = mf->height;
	// }
	// dev_dbg(dev, "%s(): Set height: %u\n", __FUNCTION__, camera->state.height);

	return 0;

	// if (!fmt) {
	// 	mf->code = 		camera->vc_data_fmts[0].code;
	// 	mf->colorspace = 	camera->vc_data_fmts[0].colorspace;
	// 	fmt = 		       &camera->vc_data_fmts[0];
	// }

	// mf->field = V4L2_FIELD_NONE;
	// if (format->which == V4L2_SUBDEV_FORMAT_TRY) {
	// 	return 0;
	// }

	// camera->fmt = fmt;

	// capturemode = get_capturemode(sd, mf->width, mf->height);
	// if (capturemode >= 0) {
	// 	camera->streamcap.capturemode 	= capturemode;
	// 	camera->pix.width 		= mf->width;
	// 	camera->pix.height 		= mf->height;
	// 	return 0;
	// }

	// dev_err(dev, "[vc-mipi camera] %s: width,height=%d,%d\n", __FUNCTION__, mf->width, mf->height);
	// dev_err(dev, "[vc-mipi camera] %s: get_capturemode() failed: capturemode=%d\n", __FUNCTION__, capturemode);
	// dev_err(dev, "[vc-mipi camera] %s: Set format failed code=%d, colorspace=%d\n", __FUNCTION__, camera->fmt->code, camera->fmt->colorspace);
	// return -EINVAL;
}

// int vc_sd_enum_frame_size(struct v4l2_subdev *sd,
// 				struct v4l2_subdev_pad_config *cfg,
// 				struct v4l2_subdev_frame_size_enum *fse)
// {
// 	struct vc_camera *camera = to_vc_camera(sd);

// 	if (fse->index > 0)
// 		return -EINVAL;

// 	fse->max_width = 			camera->ctrl.frame_dx;
// 	fse->min_width = 			camera->ctrl.frame_dx;
// 	fse->max_height = 			camera->ctrl.frame_dy;
// 	fse->min_height = 			camera->ctrl.frame_dy;

// 	return 0;
// }

// int vc_sd_enum_frame_interval(struct v4l2_subdev *sd,
// 				struct v4l2_subdev_pad_config *cfg,
// 				struct v4l2_subdev_frame_interval_enum *fie)
// {
// 	return 0;
// }

// *** Initialisation *********************************************************

const struct v4l2_subdev_core_ops vc_core_ops = {
	.s_power = vc_sd_s_power,
// vvv Toradex Ixora Apalis ***************************************************
	// .queryctrl = vc_sd_queryctrl,
	// .try_ext_ctrls = vc_sd_try_ext_ctrls,
	// .s_ctrl = vc_sd_s_ctrl,
	// .g_ctrl = vc_sd_g_ctrl,
// ^^^ ************************************************************************
};

const struct v4l2_subdev_video_ops vc_video_ops = {
	// .g_frame_interval = vc_sd_g_frame_interval,
	// .s_frame_interval = vc_sd_s_frame_interval,
	.s_stream = vc_sd_s_stream,
};

const struct v4l2_subdev_pad_ops vc_pad_ops = {
	// .enum_mbus_code = vc_sd_enum_mbus_code,
	.get_fmt = vc_sd_get_fmt,
	.set_fmt = vc_sd_set_fmt,
	// .enum_frame_size = vc_sd_enum_frame_size,
	// .enum_frame_interval = vc_sd_enum_frame_interval,
};

const struct v4l2_subdev_ops vc_subdev_ops = {
	.core = &vc_core_ops,
	.video = &vc_video_ops,
	.pad = &vc_pad_ops,
};

// imx8-mipi-csi2.c don't uses the ctrl ops. We have to handle all ops thru the subdev.
const struct v4l2_ctrl_ops vc_ctrl_ops = {
	// .queryctrl = vc_queryctrl,
	// .try_ext_ctrls = vc_try_ext_ctrls,
	// .s_ctrl = vc_s_ctrl,
	// .g_ctrl = vc_g_ctrl,
// vvv Toradex Dahlia Verdin **************************************************
	// .s_ctrl = vc_s_ctrl,
// ^^^ ************************************************************************
// vvv Digi ConnectCore8X Dev Kit *********************************************
	.s_ctrl = vc_s_ctrl,
// ^^^ ************************************************************************
};

struct v4l2_ctrl_config ctrl_config_list[] = {
	{
		// Leads to a hangup while booting on the DIGI ConnectCore8X Dev Kit
		// .ops = &vc_ctrl_ops,
		.id = V4L2_CID_GAIN,
		.name = "Gain", // Do not change the name field for the controls!
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = 0,
		.max = 0xfff,
		.def = 0,
		.step = 1,
	},
	{
		// Leads to a hangup while booting on the DIGI ConnectCore8X Dev Kit
		// .ops = &vc_ctrl_ops,
		.id = V4L2_CID_EXPOSURE,
		.name = "Exposure", // Do not change the name field for the controls!
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = 0,
		.max = 16000000,
		.def = 0,
		.step = 1,
	},
};

int vc_sd_init(struct v4l2_subdev *sd, struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct v4l2_ctrl_handler *ctrl_hdl;
	struct v4l2_ctrl *ctrl;
	int num_ctrls = ARRAY_SIZE(ctrl_config_list);
	int ret, i;

	// Initializes the subdevice
	v4l2_i2c_subdev_init(sd, client, &vc_subdev_ops);

	// Initializes the ctrls
	ctrl_hdl = devm_kzalloc(dev, sizeof(*ctrl_hdl), GFP_KERNEL);
	ret = v4l2_ctrl_handler_init(ctrl_hdl, 2);
	if (ret) {
		dev_err(dev, "%s(): Failed to init control handler\n", __FUNCTION__);
		return ret;
	}

	for (i = 0; i < num_ctrls; i++) {
		// Leads to a hangup while booting on the DIGI ConnectCore8X Dev Kit
		// when v4l2_ctrl_new_custom(..., sd) is set.
		// ctrl_config_list[i].ops = &vc_ctrl_ops;
		ctrl = v4l2_ctrl_new_custom(ctrl_hdl, &ctrl_config_list[i], NULL);
		if (ctrl == NULL) {
			dev_err(dev, "%s(): Failed to init %s ctrl\n", __FUNCTION__,
				ctrl_config_list[i].name);
			continue;
		}
	}

	if (ctrl_hdl->error) {
		ret = ctrl_hdl->error;
		dev_err(dev, "%s(): control init failed (%d)\n", __FUNCTION__, ret);
		goto error;
	}

	sd->ctrl_handler = ctrl_hdl;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdl);
	return ret;
}