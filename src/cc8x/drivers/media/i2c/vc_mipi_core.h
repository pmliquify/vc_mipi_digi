#ifndef _VC_MIPI_CORE_H
#define _VC_MIPI_CORE_H

#define DEBUG

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>

#define REG_TABLE_END 0xffff

struct vc_desc {
	__u8 magic[12];
	__u8 manuf[32];
	__u16 manuf_id;
	__u8 sen_manuf[8];
	__u8 sen_type[16];
	__u16 mod_id;
	__u16 mod_rev;
	__u16 chip_id_high;
	__u16 chip_id_low;
	__u16 chip_rev;
	__u8 regs[50];
	__u16 nr_modes;
	__u16 bytes_per_mode;
	__u8 mode1[16];
	__u8 mode2[16];
	__u8 mode3[16];
	__u8 mode4[16];
};

struct vc_table_entry {
	__u16 addr;
	__u8 val;
};

struct vc_framefmt {
    __u32 code;
    enum v4l2_colorspace colorspace;
};

struct vc_ctrl {
	int mod_id;
	int mod_i2c_addr;
	// Communication
	struct i2c_client *client_sen;
	struct i2c_client *client_mod;
	// Fixed sensor programming
	struct vc_table_entry *start_table;
	struct vc_table_entry *stop_table;
	struct vc_table_entry *mode_table;
	// Frame
	struct vc_framefmt *fmts;
	int fmts_size;
	int default_fmt;
	__u32 o_width;
	__u32 o_height;
	// Variable sensor programming
	__u32 sen_clk;
	__u32 sen_reg_vmax_l;
	__u32 sen_reg_vmax_m;
	__u32 sen_reg_vmax_h;
	__u32 sen_reg_expo_l;
	__u32 sen_reg_expo_m;
	__u32 sen_reg_expo_h;
	__u32 sen_reg_gain_l;
	__u32 sen_reg_gain_m;
	// Exposure
	__u32 expo_time_min1;
	__u32 expo_time_min2;
	__u32 expo_time_max;
	__u32 expo_vmax;
	__u32 expo_toffset;
	__u32 expo_h1period;
	// Features
	int default_mode;
	int csi_lanes;
	int io_control;
};

struct vc_state {
	// Frame and Format
	struct vc_framefmt *fmt;
	__u32 width;
	__u32 height;
	int mode; // sensor mode
	// IO flags
	int flash_output; // flash output enable
	int ext_trig; // ext. trigger flag: 0=no, 1=yes
	// Status flags
	int power_on;
	int streaming;
};

// --- Helper Functions for the VC MIPI Controller Module -----------------------------------------

struct i2c_client *vc_mod_setup(struct i2c_client *client_sen, __u8 addr_mod, struct vc_desc *desc);
int vc_mod_is_color_sensor(struct vc_desc *desc);
void vc_mod_state_init(struct vc_ctrl *ctrl, struct vc_state *state);
int vc_mod_set_power(struct i2c_client *client, int on);
int vc_mod_set_mode(struct i2c_client *client, int mode);
int vc_mod_reset_module(struct i2c_client *client_mod, int mode);
int vc_mod_set_exposure(struct i2c_client *client_mod, __u32 value, __u32 sen_clk);

// --- Helper Functions for the VC MIPI Sensors ---------------------------------------------------

int vc_sen_write_table(struct i2c_client *client, struct vc_table_entry table[]);
int vc_sen_set_exposure(struct vc_ctrl *ctrl, int value);
int vc_sen_set_gain(struct vc_ctrl *ctrl, int value);
int vc_sen_start_stream(struct vc_ctrl *ctrl, struct vc_state *state);
int vc_sen_stop_stream(struct vc_ctrl *ctrl, struct vc_state *state);

#endif // _VC_MIPI_CORE_H