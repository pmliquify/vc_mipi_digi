#ifndef _VC_MIPI_CAMERA_H
#define _VC_MIPI_CAMERA_H

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
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "vc_mipi_core.h"
#include "vc_mipi_modules.h"

// // Valid sensor resolutions
// struct vc_res {
//     int width;
//     int height;
// };

// /* Valid sensor resolutions */
// static struct vc_res vc_valid_res[] = {
//     [0] = {640, 480},
// //    [1] = {320, 240},
// //    [2] = {720, 480},
// //    [3] = {1280, 720},
// //    [4] = {1920, 1080},
// //    [5] = {2592, 1944},
// };

struct vc_camera {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep; 	// the parsed DT endpoint info
	struct mutex lock; 			// lock to protect all members below

	struct vc_desc desc;
	struct vc_ctrl ctrl;
	struct vc_state state;

	// int sen_clk;            		// sen_clk: default=54Mhz imx183=72Mhz
	// struct v4l2_captureparm streamcap;
	// struct v4l2_pix_format pix;
	// int color;              		// color flag: 0=no, 1=yes
};

static inline struct vc_camera *to_vc_camera(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vc_camera, sd);
}

#endif // _VC_MIPI_CAMERA_H