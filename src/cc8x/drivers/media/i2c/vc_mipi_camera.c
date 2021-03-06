#include "vc_mipi_core.h"

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

struct vc_device {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	// the parsed DT endpoint info

	struct vc_cam cam;
};

static inline struct vc_device *to_vc_device(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_device, sd);
}

static inline struct vc_cam *to_vc_cam(struct v4l2_subdev *sd)
{
	struct vc_device *device = to_vc_device(sd);
	return &device->cam;
}


// --- v4l2_subdev_core_ops ---------------------------------------------------

int vc_sd_s_power(struct v4l2_subdev *sd, int on)
{
	struct vc_cam *cam = to_vc_cam(sd);
	return vc_mod_set_power(cam, on);
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

int vc_sd_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	struct vc_cam *cam = to_vc_cam(sd);
	struct device *dev = sd->dev;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, control->id);
	if (ctrl == NULL) {
		dev_err(dev, "%s(): Control with id: 0x%08x not defined!\n", __FUNCTION__, control->id);
		return -EINVAL;
	}

	if (control->value < ctrl->minimum || control->value > ctrl->maximum) {
		dev_err(dev, "%s(): Control '%s' with value: %d exceeds allowed range (%lld - %lld)\n", __FUNCTION__,
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

	dev_warn(dev, "%s(): Control with id: 0x%08x is not handled\n", __FUNCTION__, ctrl->id);
	return -EINVAL;
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
		ret |= vc_sen_set_exposure_dirty(cam, vc_sd_get_ctrl_value(sd, V4L2_CID_EXPOSURE));
		ret |= vc_sen_set_gain(cam, vc_sd_get_ctrl_value(sd, V4L2_CID_GAIN));
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
	.g_ctrl = vc_sd_g_ctrl,
	.s_ctrl = vc_sd_s_ctrl,
};

const struct v4l2_subdev_video_ops vc_video_ops = {
	.s_stream = vc_sd_s_stream,
};

const struct v4l2_subdev_pad_ops vc_pad_ops = {
	.get_fmt = vc_sd_get_fmt,
	.set_fmt = vc_sd_set_fmt,
};

const struct v4l2_subdev_ops vc_subdev_ops = {
	.core = &vc_core_ops,
	.video = &vc_video_ops,
	.pad = &vc_pad_ops,
};

struct v4l2_ctrl_config ctrl_config_gain = {
	.id = V4L2_CID_GAIN,
	.name = "Gain",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.step = 1,
};

struct v4l2_ctrl_config ctrl_config_exposure = {
	.id = V4L2_CID_EXPOSURE,
	.name = "Exposure", 
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_SLIDER,
	.step = 1,
};

int vc_sd_init(struct vc_device *device)
{
	struct v4l2_subdev *sd = &device->sd;
	struct i2c_client *client = device->cam.ctrl.client_sen;
	struct device *dev = &client->dev;
	struct v4l2_ctrl_handler *ctrl_hdl;
	struct v4l2_ctrl *ctrl;
	int ret;

	// Initializes the subdevice
	v4l2_i2c_subdev_init(sd, client, &vc_subdev_ops);

	// Initializes the ctrls
	ctrl_hdl = devm_kzalloc(dev, sizeof(*ctrl_hdl), GFP_KERNEL);
	ret = v4l2_ctrl_handler_init(ctrl_hdl, 2);
	if (ret) {
		dev_err(dev, "%s(): Failed to init control handler\n", __FUNCTION__);
		return ret;
	}

	if(device->cam.ctrl.exposure.enabled) {
		ctrl_config_exposure.min = device->cam.ctrl.exposure.min;
		ctrl_config_exposure.max = device->cam.ctrl.exposure.max;
		ctrl_config_exposure.def = device->cam.ctrl.exposure.default_val;
		ctrl = v4l2_ctrl_new_custom(ctrl_hdl, &ctrl_config_exposure, NULL);
		if (ctrl == NULL) {
			dev_err(dev, "%s(): Failed to init %s ctrl\n", __FUNCTION__, ctrl_config_exposure.name);
		}
		ctrl->priv = (void*)sd;
	}

	if(device->cam.ctrl.gain.enabled) {
		ctrl_config_gain.min = device->cam.ctrl.gain.min;
		ctrl_config_gain.max = device->cam.ctrl.gain.max;
		ctrl_config_gain.def = device->cam.ctrl.gain.default_val;
		ctrl = v4l2_ctrl_new_custom(ctrl_hdl, &ctrl_config_gain, NULL);
		if (ctrl == NULL) {
			dev_err(dev, "%s(): Failed to init %s ctrl\n", __FUNCTION__, ctrl_config_gain.name);
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
	struct vc_device *device;
	int ret;

	device = devm_kzalloc(dev, sizeof(*device), GFP_KERNEL);
	if (!device)
		return -ENOMEM;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		dev_err(dev, "%s(): Endpoint node not found\n", __FUNCTION__);
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &device->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(dev, "%s(): Could not parse endpoint\n", __FUNCTION__);
		return ret;
	}

	ret = vc_core_init(&device->cam, client);
	if (ret) 
		goto free_ctrls;

	ret = vc_sd_init(device);
	if (ret)
		goto free_ctrls;

	device->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	device->pad.flags = MEDIA_PAD_FL_SOURCE;
	device->sd.entity.ops = &vc_sd_media_ops;
	device->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&device->sd.entity, 1, &device->pad);
	if (ret)
		return ret;

	ret = v4l2_async_register_subdev_sensor_common(&device->sd);
	if (ret)
		goto free_ctrls;

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(device->sd.ctrl_handler);
	media_entity_cleanup(&device->sd.entity);
	return ret;
}

static int vc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vc_device *device = to_vc_device(sd);

	v4l2_async_unregister_subdev(&device->sd);
	media_entity_cleanup(&device->sd.entity);
	v4l2_ctrl_handler_free(device->sd.ctrl_handler);

	return 0;
}

static const struct i2c_device_id vc_id[] = {
	{ "vc-mipi-cam", 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, vc_id);

static const struct of_device_id vc_dt_ids[] = { 
	{ .compatible = "vc,vc_mipi" }, 
	{ /* sentinel */ } 
};
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

MODULE_DESCRIPTION("VC MIPI Camera Driver");
MODULE_VERSION("0.3.0");
MODULE_LICENSE("GPL");
