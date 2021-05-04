/* SPDX-License-Identifier: GPL-2.0 */
/*
 * V4L2 Capture ISI subdev for i.MX8QXP/QM platform
 *
 * ISI is a Image Sensor Interface of i.MX8QXP/QM platform, which
 * used to process image from camera sensor to memory or DC
 *
 * Copyright (c) 2019 NXP Semiconductor
 *
 */

#ifndef __MXC_COMMON_H__
#define __MXC_COMMON_H__

#define ISI_OF_NODE_NAME	"isi"
#define MIPI_CSI2_OF_NODE_NAME  "csi"
#define PARALLEL_OF_NODE_NAME   "pcsi"

#define MXC_ISI_MAX_DEVS	8
#define MXC_ISI_NUM_PORTS	1
#define MXC_MIPI_CSI2_MAX_DEVS	2
#define MXC_MAX_SENSORS		3

/* ISI PADS */
#define MXC_ISI_SD_PAD_SINK_MIPI0		0
#define MXC_ISI_SD_PAD_SINK_MIPI1		1

#define MXC_ISI_SD_PAD_SINK_DC0			2
#define MXC_ISI_SD_PAD_SINK_DC1			3
#define MXC_ISI_SD_PAD_SINK_HDMI		4
#define MXC_ISI_SD_PAD_SINK_MEM			5
#define MXC_ISI_SD_PAD_SOURCE_MEM		6
#define MXC_ISI_SD_PAD_SOURCE_DC0		7
#define MXC_ISI_SD_PAD_SOURCE_DC1		8
#define MXC_ISI_SD_PAD_SINK_PARALLEL_CSI	9
#define MXC_ISI_SD_PADS_NUM			10

/* MIPI CSI PADS */
#define MXC_MIPI_CSI2_PAD_SINK			0
#define MXC_MIPI_CSI2_PAD_SOURCE		1

/* Parallel CSI PADS */
#define MXC_PARALLEL_CSI_PAD_SOURCE		0
#define MXC_PARALLEL_CSI_PAD_SINK		1
#define MXC_PARALLEL_CSI_PADS_NUM		2

#define ISI_2K		2048

enum {
	IN_PORT,
	OUT_PORT,
	MAX_PORTS,
};

enum isi_input_interface {
	ISI_INPUT_INTERFACE_DC0 = 0,
	ISI_INPUT_INTERFACE_DC1,
	ISI_INPUT_INTERFACE_MIPI0_CSI2,
	ISI_INPUT_INTERFACE_MIPI1_CSI2,
	ISI_INPUT_INTERFACE_HDMI,
	ISI_INPUT_INTERFACE_MEM,
	ISI_INPUT_INTERFACE_PARALLEL_CSI,
	ISI_INPUT_INTERFACE_MAX,
};

enum isi_output_interface {
	ISI_OUTPUT_INTERFACE_DC0 = 0,
	ISI_OUTPUT_INTERFACE_DC1,
	ISI_OUTPUT_INTERFACE_MEM,
	ISI_OUTPUT_INTERFACE_MAX,
};

enum mxc_isi_buf_id {
	MXC_ISI_BUF1 = 0x0,
	MXC_ISI_BUF2,
};

#endif /* MXC_ISI_CORE_H_ */
