#ifndef _VC_MIPI_CORE_H
#define _VC_MIPI_CORE_H

#define DEBUG

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>

#define FLAG_MODE_2_LANES       0x01
#define FLAG_MODE_4_LANES       0x02
#define FLAG_TRIGGER_IN         0x04
#define FLAG_FLASH_OUT          0x08

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

struct vc_range {
	__u32 min;
	__u32 max;
	__u32 default_val;
};

struct vc_settings {
	struct vc_range exposure;
	struct vc_range gain;
};

struct vc_framefmt {
	__u32 code;
	enum v4l2_colorspace colorspace;
};

struct vc_mode {
	__u32 code;
	__u8 flags;
	__u8 value;
};

struct vc_addr2 {
	__u32 l;
	__u32 m;
};

struct vc_addr3 {
	__u32 l;
	__u32 m;
	__u32 h;
};

struct vc_sen_addrs {
	struct vc_addr2 mode;
	struct vc_addr3 vmax;
	struct vc_addr3 expo;
	struct vc_addr2 gain;
	struct vc_addr2 o_width;
	struct vc_addr2 o_height;
};

struct vc_addrs {
	struct vc_sen_addrs sen;
};

struct vc_ctrl {
	int mod_id;
	int mod_i2c_addr;
	// Communication
	struct i2c_client *client_sen;
	struct i2c_client *client_mod;
	// Settings
	struct vc_settings set;
	// Modes & Frame Formats
	struct vc_mode *modes;
	int default_mode;
	struct vc_framefmt *fmts;
	int default_fmt;
	__u32 o_width;
	__u32 o_height;
	// Variable sensor programming
	struct vc_addrs addr;
	// Exposure
	__u32 sen_clk;
	__u32 expo_time_min2;
	__u32 expo_vmax;
	__u32 expo_toffset;
	__u32 expo_h1period;
	// Features
	__u8 flags;
};

struct vc_state {
	// Modes & Frame Formats
	struct vc_framefmt *fmt;
	__u32 width;
	__u32 height;
	// Features
	__u8 flags;
	// Status flags
	int power_on;
	int streaming;
};

// --- Helper functions for internal data structures
void vc_core_state_init(struct vc_ctrl *ctrl, struct vc_state *state);
struct vc_framefmt *vc_core_find_framefmt(struct vc_ctrl *ctrl, __u32 code);

// --- Functions for the VC MIPI Controller Module ----------------------------
int vc_mod_setup(struct vc_ctrl *ctrl, int mod_i2c_addr, struct vc_desc *desc);
int vc_mod_is_color_sensor(struct vc_desc *desc);
int vc_mod_set_power(struct vc_ctrl *ctrl, int on);
int vc_mod_set_mode(struct vc_ctrl *ctrl, struct vc_state *state);

// --- Functions for the VC MIPI Sensors --------------------------------------
int vc_sen_set_roi(struct vc_ctrl *ctrl, int width, int height);
int vc_sen_set_gain(struct vc_ctrl *ctrl, int value);
int vc_sen_start_stream(struct vc_ctrl *ctrl, struct vc_state *state);
int vc_sen_stop_stream(struct vc_ctrl *ctrl, struct vc_state *state);

// TODO: Cleaned up
int vc_sen_set_exposure_dirty(struct vc_ctrl *ctrl, struct vc_state *state, int value);

#endif // _VC_MIPI_CORE_H