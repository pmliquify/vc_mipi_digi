#define DEBUG

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "vc_mipi_core.h"


struct vc_camera {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	// the parsed DT endpoint info

	struct vc_cam cam;
};

static inline struct vc_camera *to_vc_camera(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_camera, sd);
}

static inline struct vc_cam *to_vc_cam(struct v4l2_subdev *sd)
{
	struct vc_camera *camera = to_vc_camera(sd);
	return &camera->cam;
}


// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_sd_s_power(struct v4l2_subdev *sd, int on)
{
	struct vc_cam *cam = to_vc_cam(sd);
	return vc_mod_set_power(cam, on);
}

int vc_sd_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	struct device *dev = sd->dev;
	int ret;
	ret = v4l2_queryctrl(sd->ctrl_handler, qc);
	dev_dbg(dev, "%s() Query control (id: 0x%08x, name: %s)\n", __FUNCTION__, qc->id, qc->name);
	return ret;
}

int vc_sd_queryextctrl(struct v4l2_subdev *sd, struct v4l2_query_ext_ctrl *qec)
{
	struct device *dev = sd->dev;
	dev_dbg(dev, "%s()\n", __FUNCTION__);
	return 0;
}

int vc_sd_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct device *dev = sd->dev;
	dev_dbg(dev, "%s()\n", __FUNCTION__);
	return 0;
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

int vc_sd_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL) {
		dev_err(dev, "%s(): Control with id 0x%08x not defined!\n", __FUNCTION__, control->id);
		return -EINVAL;
	}

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
		return vc_sen_set_exposure_dirty(cam, control->value);

	case V4L2_CID_GAIN:
		return vc_sen_set_gain(cam, control->value);
	}

	dev_warn(dev, "%s(): ctrl(id:0x%x, val:0x%x) is not handled\n", __FUNCTION__, ctrl->id, ctrl->val);
	return -EINVAL;
}

int vc_g_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	struct device *dev = sd->dev;
	struct v4l2_control control;
	int ret;

	dev_dbg(dev, "%s()\n", __FUNCTION__);

	if(ctrls->count != 1 && ctrls->controls == NULL)
		return -EINVAL;

	control.id = ctrls->controls->id;
	ret = vc_sd_g_ctrl(sd, &control);
	if (ret)
		return -EINVAL;

	ctrls->count = 1;
	ctrls->controls->value = control.value;
	return 0;
}

int vc_s_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	struct device *dev = sd->dev;
	struct v4l2_control control;

	dev_dbg(dev, "%s()\n", __FUNCTION__);

	if(ctrls->count != 1 && ctrls->controls == NULL)
		return -EINVAL;

	control.id = ctrls->controls->id;
	control.value = ctrls->controls->value;
	return vc_sd_s_ctrl(sd, &control);
}

int vc_sd_try_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ctrls)
{
	struct device *dev = sd->dev;
	dev_dbg(dev, "%s()\n", __FUNCTION__);
	return 0;
}


// __s32 vc_sd_get_ctrl_value(struct v4l2_subdev *sd, __u32 id)
// {
// 	struct v4l2_control control;
// 	control.id = id;
// 	vc_sd_g_ctrl(sd, &control);
// 	// TODO: Errorhandling
// 	return control.value;
// }

// --- v4l2_ctrl_ops ---------------------------------------------------

int vc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd;
	struct v4l2_control control;
	
	if(ctrl->priv != NULL) {
		sd = (struct v4l2_subdev *)ctrl->priv;
	}
	
	control.id = ctrl->id;
	control.value = ctrl->val;
	
	return vc_sd_s_ctrl(sd, &control);
}


// --- v4l2_subdev_video_ops ---------------------------------------------------

int vc_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct vc_ctrl *ctrl = &cam->ctrl;
	struct vc_state *state = &cam->state;
	struct device *dev = sd->dev;
	int ret = 0;

	dev_dbg(dev, "%s(): Set streaming: %s\n", __FUNCTION__, enable ? "on" : "off");

	if (enable) {
		if (state->streaming == 1) {
			dev_warn(dev, "%s(): Sensor is already streaming!\n", __FUNCTION__);
			ret = vc_sen_stop_stream(cam);
		}

		ret  = vc_mod_set_mode(cam);
		// ret |= vc_sen_set_exposure_dirty(cam, ctrl->set.exposure.default_val);
		// ret |= vc_sen_set_gain(cam, ctrl->set.gain.default_val);
		// ret |= vc_sen_set_roi(cam, ctrl->o_frame.width, ctrl->o_frame.height);
		ret |= vc_sen_set_exposure_dirty(cam, ctrl->set.exposure.default_val);
		ret |= vc_sen_set_gain(cam, ctrl->set.gain.default_val);
		ret |= vc_sen_set_roi(cam, ctrl->o_frame.width, ctrl->o_frame.height);

		ret |= vc_sen_start_stream(cam);
		if (ret == 0)
			state->streaming = 1;

	} else {
		ret = vc_sen_stop_stream(cam);
		if (ret == 0)
			state->streaming = 0;
	}

	return ret;
}

// --- v4l2_subdev_pad_ops ---------------------------------------------------

int vc_sd_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct vc_frame* frame;

	mf->code = vc_core_get_format(cam);
	frame = vc_core_get_frame(cam);
	mf->width = frame->width;
	mf->height = frame->height;
	// mf->reserved[1] = 30;

	return 0;
}

int vc_sd_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *format)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct v4l2_mbus_framefmt *mf = &format->format;

	vc_core_set_format(cam, mf->code);
	vc_core_set_frame(cam, mf->width, mf->height);
	
	return 0;
}



// *** Initialisation *********************************************************

const struct v4l2_subdev_core_ops vc_core_ops = {
	.s_power = vc_sd_s_power,
// vvv Toradex Ixora Apalis ***************************************************
	.queryctrl = vc_sd_queryctrl,
	.queryextctrl = vc_sd_queryextctrl,
	.querymenu = vc_sd_querymenu,
	.g_ctrl = vc_sd_g_ctrl,
	.s_ctrl = vc_sd_s_ctrl,
	.g_ext_ctrls = vc_g_ext_ctrls,
    	.s_ext_ctrls = vc_s_ext_ctrls,
	.try_ext_ctrls = vc_sd_try_ext_ctrls,
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
		.name = "Gain2", // Do not change the name field for the controls!
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
		.name = "Exposure2", // Do not change the name field for the controls!
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
		ctrl_config_list[i].ops = &vc_ctrl_ops;
		ctrl = v4l2_ctrl_new_custom(ctrl_hdl, &ctrl_config_list[i], NULL);
		if (ctrl == NULL) {
			dev_err(dev, "%s(): Failed to init %s ctrl\n", __FUNCTION__,
				ctrl_config_list[i].name);
			continue;
		}
		ctrl->priv = (void*)sd;
	}

	if (ctrl_hdl->error) {
		ret = ctrl_hdl->error;
		dev_err(dev, "%s(): Control init failed (%d)\n", __FUNCTION__, ret);
		goto error;
	}

	sd->ctrl_handler = ctrl_hdl;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdl);
	return ret;
}

static int vc_link_setup(struct media_entity *entity, const struct media_pad *local, const struct media_pad *remote,
			 __u32 flags)
{
	return 0;
}

static const struct media_entity_operations vc_sd_media_ops = {
	.link_setup = vc_link_setup,
};

static int vc_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *endpoint;
	struct vc_camera *camera;
	int ret;

	camera = devm_kzalloc(dev, sizeof(*camera), GFP_KERNEL);
	if (!camera)
		return -ENOMEM;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		dev_err(dev, "[vc-mipi vc_mipi] endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &camera->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "[vc-mipi vc_mipi] Could not parse endpoint\n");
		return ret;
	}

	ret = vc_sd_init(&camera->sd, client);
	if (ret)
		goto free_ctrls;

	camera->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	camera->pad.flags = MEDIA_PAD_FL_SOURCE;
	camera->sd.entity.ops = &vc_sd_media_ops;
	camera->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&camera->sd.entity, 1, &camera->pad);
	if (ret)
		return ret;

	ret = vc_core_init(&camera->cam, client);
	if (ret) 
		goto free_ctrls;

	ret = v4l2_async_register_subdev_sensor_common(&camera->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(camera->sd.ctrl_handler);
	media_entity_cleanup(&camera->sd.entity);
	return ret;
}

static int vc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_camera *camera = to_vc_camera(sd);

	v4l2_async_unregister_subdev(&camera->sd);
	media_entity_cleanup(&camera->sd.entity);
	v4l2_ctrl_handler_free(camera->sd.ctrl_handler);

	return 0;
}

static const struct i2c_device_id vc_id[] = {
	{ "vc-mipi-cam", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, vc_id);

static const struct of_device_id vc_dt_ids[] = { { .compatible = "vc,vc_mipi" }, { /* sentinel */ } };
MODULE_DEVICE_TABLE(of, vc_dt_ids);

static struct i2c_driver vc_i2c_driver = {
	.driver = {
		.name  = "vc-mipi-cam",
		.of_match_table	= vc_dt_ids,
	},
	.id_table = vc_id,
	.probe_new = vc_probe,
	.remove   = vc_remove,
};

module_i2c_driver(vc_i2c_driver);

MODULE_DESCRIPTION("IMX226 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");