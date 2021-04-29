#include "vc_mipi_core.h"
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include "vc_mipi_modules.h"

#define MOD_REG_RESET           0x0100 // register  0 [0x0100]: reset and init register (R/W)
#define MOD_REG_STATUS          0x0101 // register  1 [0x0101]: status (R)
#define MOD_REG_MODE            0x0102 // register  2 [0x0102]: initialisation mode (R/W)
#define MOD_REG_IOCTRL          0x0103 // register  3 [0x0103]: input/output control (R/W)
#define MOD_REG_MOD_ADDR        0x0104 // register  4 [0x0104]: module i2c address (R/W, default: 0x10)
#define MOD_REG_SEN_ADDR        0x0105 // register  5 [0x0105]: sensor i2c address (R/W, default: 0x1A)
#define MOD_REG_OUTPUT          0x0106 // register  6 [0x0106]: output signal override register (R/W, default: 0x00)
#define MOD_REG_INPUT           0x0107 // register  7 [0x0107]: input signal status register (R)
#define MOD_REG_EXTTRIG         0x0108 // register  8 [0x0108]: external trigger enable (R/W, default: 0x00)
#define MOD_REG_EXPO_L          0x0109 // register  9 [0x0109]: exposure LSB (R/W, default: 0x10)
#define MOD_REG_EXPO_M          0x010A // register 10 [0x010A]: exposure 	   (R/W, default: 0x27)
#define MOD_REG_EXPO_H          0x010B // register 11 [0x010B]: exposure     (R/W, default: 0x00)
#define MOD_REG_EXPO_U          0x010C // register 12 [0x010C]: exposure MSB (R/W, default: 0x00)
#define MOD_REG_RETRIG_L        0x010D // register 13 [0x010D]: retrigger LSB (R/W, default: 0x40)
#define MOD_REG_RETRIG_M        0x010E // register 14 [0x010E]: retrigger     (R/W, default: 0x2D)
#define MOD_REG_RETRIG_H        0x010F // register 15 [0x010F]: retrigger     (R/W, default: 0x29)
#define MOD_REG_RETRIG_U        0x0110 // register 16 [0x0110]: retrigger MSB (R/W, default: 0x00)

#define REG_RESET_PWR_UP        0x00
#define REG_RESET_SENSOR        0x01   // reg0[0] = 0 sensor reset the sensor is held in reset when this bit is 1
#define REG_RESET_PWR_DOWN      0x02   // reg0[1] = 0 power down power for the sensor is switched off

#define REG_STATUS_NO_COM       0x00   // reg1[7:0] = 0x00 default, no communication with sensor possible
#define REG_STATUS_READY        0x80   // reg1[7:0] = 0x80 sensor ready after successful initialization sequence
#define REG_STATUS_ERROR        0x01   // reg1[7:0] = 0x01 internal error during initialization

#define REG_EXTTRIG_ENABLE      0x01
#define REG_EXTTRIG_DISABLE     0x00

#define REG_IOCTRL_ENABLE       0x01
#define REG_IOCTRL_DISABLE      0x00

// ------------------------------------------------------------------------------------------------
//  Helper Functions for I2C Communication

#define U_BYTE(value) (__u8)((value >> 24) & 0xff)
#define H_BYTE(value) (__u8)((value >> 16) & 0xff)
#define M_BYTE(value) (__u8)((value >> 8) & 0xff)
#define L_BYTE(value) (__u8)(value & 0xff)

int i2c_read_reg(struct i2c_client *client, const __u16 addr)
{
	__u8 buf[2] = { addr >> 8, addr & 0xff };
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = buf,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(&client->dev, "%s(): Reading register %x from %x failed\n", __FUNCTION__, addr, client->addr);
		return ret;
	}

	return buf[0];
}

int i2c_write_reg(struct i2c_client *client, const __u16 addr, const __u8 data)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	__u8 tx[3];
	int ret;

	msg.addr = client->addr;
	msg.buf = tx;
	msg.len = 3;
	msg.flags = 0;
	tx[0] = addr >> 8;
	tx[1] = addr & 0xff;
	tx[2] = data;
	ret = i2c_transfer(adap, &msg, 1);
	mdelay(2);

	return ret == 1 ? 0 : -EIO;
}

int vc_sen_write_table(struct i2c_client *client, struct vc_table_entry table[])
{
	struct device *dev = &client->dev;
	const struct vc_table_entry *reg;
	int ret;

	dev_dbg(dev, "%s(): Write sensor register table\n", __FUNCTION__);

	for (reg = table; reg->addr != REG_TABLE_END; reg++) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, reg->addr, reg->val);
		ret = i2c_write_reg(client, reg->addr, reg->val);
		if (ret < 0) {
			dev_err(dev, "%s(): error=%d\n", __FUNCTION__, ret);
			return ret;
		}
	}

	return 0;
}


// ------------------------------------------------------------------------------------------------
//  Helper Functions for debugging

void vc_dump_reg_value(struct device *dev, int addr, int reg)
{
	int sval = 0; // short 2-byte value
	if (addr & 1) { // odd addr
		sval |= (int)reg << 8;
		dev_dbg(dev, "%s(): addr=0x%04x reg=0x%04x\n", __FUNCTION__, addr + 0x1000 - 1, sval);
	}
}

void vc_dump_hw_desc(struct device *dev, struct vc_desc *desc)
{
	dev_info(dev, "VC MIPI Module - Hardware Descriptor\n");
	dev_info(dev, "[ MAGIC  ] [ %s ]\n", desc->magic);
	dev_info(dev, "[ MANUF. ] [ %s ] [ MID=0x%04x ]\n", desc->manuf, desc->manuf_id);
	dev_info(dev, "[ SENSOR ] [ %s %s ]\n", desc->sen_manuf, desc->sen_type);
	dev_info(dev, "[ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n", desc->mod_id, desc->mod_rev);
	dev_info(dev, "[ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n", desc->nr_modes, desc->bytes_per_mode);
}

// ------------------------------------------------------------------------------------------------
//  Helper Functions for the VC MIPI Controller Module

struct i2c_client *vc_mod_get_client(struct i2c_adapter *adapter, __u8 addr)
{
	struct i2c_board_info info = {
		I2C_BOARD_INFO("i2c", addr),
	};
	unsigned short addr_list[2] = { addr, I2C_CLIENT_END };
	return i2c_new_probed_device(adapter, &info, addr_list, NULL);
}

int vc_mod_set_power(struct i2c_client *client, int on)
{
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Set module power: %s\n", __FUNCTION__, on ? "up" : "down");

	ret = i2c_write_reg(client, MOD_REG_RESET, on ? REG_RESET_PWR_UP : REG_RESET_PWR_DOWN);
	if (ret)
		dev_err(dev, "%s(): Unable to power %s the module (error: %d)\n", __FUNCTION__,
			(on == REG_RESET_PWR_UP) ? "up" : "down", ret);

	return ret;
}

int vc_mod_get_status(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int ret;

	ret = i2c_read_reg(client, MOD_REG_STATUS);
	if (ret < 0)
		dev_err(dev, "%s(): Unable to get module status (error: %d)\n", __FUNCTION__, ret);
	else
		dev_dbg(dev, "%s(): Get module status: 0x%02x\n", __FUNCTION__, ret);

	return ret;
}

int vc_mod_set_ext_trig(struct i2c_client *client, int enable)
{
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Set external trigger: %s\n", __FUNCTION__, enable ? "on" : "off");

	ret = i2c_write_reg(client, MOD_REG_EXTTRIG, enable);
	if (ret)
		dev_err(dev, "%s(): Unable to set external trigger (error: %d)\n", __FUNCTION__, ret);

	return ret;
}

int vc_mod_set_flash_output(struct i2c_client *client, int enable)
{
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Set flash output: %s\n", __FUNCTION__, enable ? "on" : "off");

	ret = i2c_write_reg(client, MOD_REG_IOCTRL, enable);
	if (ret)
		dev_err(dev, "%s(): Unable to set flash output (error: %d)\n", __FUNCTION__, ret);

	return ret;
}

int vc_mod_wait_until_module_is_ready(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int status;
	int try;

	dev_dbg(dev, "%s(): Wait until module is ready\n", __FUNCTION__);

	status = REG_STATUS_NO_COM;
	try = 0;
	while (status == REG_STATUS_NO_COM && try < 10) {
		mdelay(100);
		status = vc_mod_get_status(client);
		try++;
	}
	if (status == REG_STATUS_ERROR) {
		dev_err(dev, "%s(): Internal Error!", __func__);
		return -EIO;
	}

	dev_dbg(dev, "%s(): Module is ready!\n", __FUNCTION__);
	return 0;
}

int vc_mod_set_mode(struct i2c_client *client, int mode)
{
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Set module mode: 0x%02x\n", __FUNCTION__, mode);

	ret = i2c_write_reg(client, MOD_REG_MODE, mode);
	if (ret)
		dev_err(dev, "%s(): Unable to set module mode (error: %d)\n", __FUNCTION__, ret);

	return ret;
}

int vc_mod_reset_module(struct i2c_client *client, int mode)
{
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Reset the module!\n", __FUNCTION__);

	// TODO: Check if it really necessary to set mode in power down state.
	ret = vc_mod_set_power(client, 0);
	ret |= vc_mod_set_mode(client, mode);
	ret |= vc_mod_set_power(client, 1);
	ret |= vc_mod_wait_until_module_is_ready(client);

	return ret;
}

struct i2c_client *vc_mod_setup(struct i2c_client *client_sen, __u8 addr_mod, struct vc_desc *desc)
{
	struct i2c_adapter *adapter = client_sen->adapter;
	struct device *dev_sen = &client_sen->dev;
	struct i2c_client *client_mod;
	struct device *dev_mod;
	int addr, reg;

	dev_dbg(dev_sen, "%s(): Setup the module\n", __FUNCTION__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev_sen, "%s(): I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n", __FUNCTION__);
		return 0;
	}

	client_mod = vc_mod_get_client(adapter, addr_mod);
	if (client_mod == 0) {
		dev_err(dev_sen, "%s(): Unable to get module I2C client for address 0x%02x\n", __FUNCTION__, addr_mod);
		return 0;
	}

	dev_mod = &client_mod->dev;
	for (addr = 0; addr < sizeof(*desc); addr++) {
		reg = i2c_read_reg(client_mod, addr + 0x1000);
		if (reg < 0) {
			i2c_unregister_device(client_mod);
			return 0;
		}
		*((char *)(desc) + addr) = (char)reg;

		//vc_dump_reg_value(dev, addr, reg);
	}

	vc_dump_hw_desc(dev_mod, desc);

	dev_info(dev_mod, "VC MIPI Sensor succesfully initialized.");
	return client_mod;
}

int vc_mod_is_color_sensor(struct vc_desc *desc)
{
	if (desc->sen_type) {
		__u32 len = strnlen(desc->sen_type, 16);
		if (len > 0 && len < 17) {
			return *(desc->sen_type + len - 1) == 'C';
		}
	}
	return 0;
}

void vc_mod_state_init(struct vc_ctrl *ctrl, struct vc_state *state)
{
	state->fmt = &ctrl->fmts[ctrl->default_fmt];
	state->width = ctrl->o_width;
	state->height = ctrl->o_height;
	state->mode = ctrl->default_mode;
	state->flash_output = 0;
	state->ext_trig = 0;
	state->streaming = 0;
}

int vc_mod_set_exposure(struct i2c_client *client, __u32 value, __u32 sen_clk)
{
	struct device *dev = &client->dev;
	int ret;

	__u32 exposure = (value * (sen_clk / 1000000));

	dev_dbg(dev, "%s(): Set module exposure = 0x%08x (%u)\n", __FUNCTION__, exposure, exposure);

	ret = i2c_write_reg(client, MOD_REG_EXPO_U, U_BYTE(exposure));
	ret |= i2c_write_reg(client, MOD_REG_EXPO_H, H_BYTE(exposure));
	ret |= i2c_write_reg(client, MOD_REG_EXPO_M, M_BYTE(exposure));
	ret |= i2c_write_reg(client, MOD_REG_EXPO_L, L_BYTE(exposure));
	return ret;
}


// ------------------------------------------------------------------------------------------------
//  Helper Functions for the VC MIPI Sensors

__u32 vc_sen_read_vmax(struct vc_ctrl *ctrl)
{
	struct i2c_client *client = ctrl->client_sen;
	struct device *dev = &client->dev;
	int reg;
	__u32 vmax = 0;

	reg = i2c_read_reg(client, ctrl->sen_reg_vmax_h);
	if (reg)
		vmax = reg;
	reg = i2c_read_reg(client, ctrl->sen_reg_vmax_m);
	if (reg)
		vmax = (vmax << 8) | reg;
	reg = i2c_read_reg(client, ctrl->sen_reg_vmax_l);
	if (reg)
		vmax = (vmax << 8) | reg;

	dev_dbg(dev, "%s(): Read sensor VMAX: 0x%08x (%d)\n", __FUNCTION__, vmax, vmax);

	return vmax;
}

int vc_sen_write_vmax(struct vc_ctrl *ctrl, __u32 vmax)
{
	struct i2c_client *client = ctrl->client_sen;
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Write sensor VMAX: 0x%08x (%d)\n", __FUNCTION__, vmax, vmax);

	if (ctrl->sen_reg_vmax_l) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_vmax_l, L_BYTE(vmax));
		ret = i2c_write_reg(client, ctrl->sen_reg_vmax_l, L_BYTE(vmax));
	}
	if (ctrl->sen_reg_vmax_m) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_vmax_m, M_BYTE(vmax));
		ret |= i2c_write_reg(client, ctrl->sen_reg_vmax_m, M_BYTE(vmax));
	}
	if (ctrl->sen_reg_vmax_h) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_vmax_h, H_BYTE(vmax));
		ret |= i2c_write_reg(client, ctrl->sen_reg_vmax_h, H_BYTE(vmax));
	}
	
	return ret;
}

int vc_sen_write_exposure(struct vc_ctrl *ctrl, __u32 exposure)
{
	struct i2c_client *client = ctrl->client_sen;
	struct device *dev = &client->dev;
	int ret;

	dev_dbg(dev, "%s(): Write sensor exposure: 0x%08x (%d)\n", __FUNCTION__, exposure, exposure);

	if (ctrl->sen_reg_expo_l) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_expo_l, L_BYTE(exposure));
		ret = i2c_write_reg(client, ctrl->sen_reg_expo_l, L_BYTE(exposure));
	}
	if (ctrl->sen_reg_expo_m) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_expo_m, M_BYTE(exposure));
		ret |= i2c_write_reg(client, ctrl->sen_reg_expo_m, M_BYTE(exposure));
	}
	if (ctrl->sen_reg_expo_h) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_expo_h, H_BYTE(exposure));
		ret |= i2c_write_reg(client, ctrl->sen_reg_expo_h, H_BYTE(exposure));
	}

	return ret;
}

int vc_sen_set_exposure(struct vc_ctrl *ctrl, int value)
{
	struct i2c_client *client = ctrl->client_sen;
	struct device *dev = &client->dev;
	__u32 exposure = 0;
	__u32 vmax = 0;
	int ret = 0;

	dev_dbg(dev, "%s(): Set sensor exposure: %u us\n", __FUNCTION__, value);
	dev_dbg(dev, "%s(): Checking Limits Min1: %u, Min2: %u, Max: %u us\n", __FUNCTION__,
		ctrl->expo_time_min1, ctrl->expo_time_min2, ctrl->expo_time_max);

	// TODO: It is assumed, that the exposure value is valid => remove clamping.
	if (value < ctrl->expo_time_min1)
		value = ctrl->expo_time_min1;
	if (value > ctrl->expo_time_max)
		value = ctrl->expo_time_max;

	if (value < ctrl->expo_time_min2) {
		dev_dbg(dev, "%s(): Set exposure by method: < Min2\n", __FUNCTION__);
		switch (ctrl->mod_id) {
		case MOD_ID_IMX226:
			{
				// TODO: Find out which version is correct.
				// __u32 vmax = vc_sen_read_vmax(client);
				vmax = ctrl->expo_vmax;

				// exposure = (NumberOfLines - exp_time / 1Hperiod + toffset / 1Hperiod )
				// shutter = {VMAX - SHR}*HMAX + 209(157) clocks
				exposure = (vmax - ((__u32)(value)*16384 - ctrl->expo_toffset) / ctrl->expo_h1period);
			}
			break;

		case MOD_ID_IMX327:
			{
				vmax = 0x0465; // 1125
			}
			{
				// range 1..1123
				__u32 mode = 0; // Vorläufig 0 setzen bis set mode richtig implementiert ist.
				__u32 lf = mode ? 2 : 1;
				exposure = (1124 * 20000 - (__u32)(value) * 29 * 20 * lf) / 20000;
			}
			break;
		}

		// TODO: Is it realy nessecary to write the same vmax value back?


	} else {
		dev_dbg(dev, "%s(): Set exposure by method: >= Min2\n", __FUNCTION__);
		switch (ctrl->mod_id) {
		case MOD_ID_IMX226:
			{
				// exposure = 5 + ((unsigned long long)(value * 16384) - tOffset)/h1Period;
				// __u64 divresult = ((__u64)value * 16384) - ctrl->expo_toffset;
				// __u32 divisor   = IMX226_EXPO_H1PERIOD;
				// __u32 remainder = (__u32)(do_div(divresult, divisor)); // caution: division result value at dividend!
				// vmax = 5 + (__u32)divresult;
				vmax = ((__u64)value * 16384) - ctrl->expo_toffset;
				exposure = 0x0004;
			}
			break;

		case MOD_ID_IMX327:
			{
				// range 1123..
				__u32 mode = 0; // Vorläufig 0 setzen bis set mode richtig implementiert ist.
				__u32 lf = mode ? 2 : 1;
				vmax = ( 1 * 20000 + (__u32)(value) * 29 * 20 * lf ) / 20000;
				exposure = 0x0001;
			}
			break;
		}
	}

	switch (ctrl->mod_id) {
	case MOD_ID_IMX226:
	case MOD_ID_IMX327:
		ret  = vc_sen_write_vmax(ctrl, vmax);
		ret |= vc_sen_write_exposure(ctrl, exposure);
		break;
	}

	return ret;
}

int vc_sen_set_gain(struct vc_ctrl *ctrl, int value)
{
	struct i2c_client *client = ctrl->client_sen;
	struct device *dev = &client->dev;
	int ret = 0;

	dev_dbg(dev, "%s(): Set sensor gain: %u\n", __FUNCTION__, value);

	if (ctrl->sen_reg_gain_l) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_gain_l, L_BYTE(value));
		ret  = i2c_write_reg(client, ctrl->sen_reg_gain_l, L_BYTE(value));
	}
	if (ctrl->sen_reg_gain_m) {
		dev_dbg(dev, "%s(): Address: 0x%04x <= Value: 0x%02x\n", __FUNCTION__, ctrl->sen_reg_gain_m, M_BYTE(value));
		ret |= i2c_write_reg(client, ctrl->sen_reg_gain_m, M_BYTE(value));
	}
	if (ret)
		dev_err(dev, "%s(): Couldn't set 'Gain' (error=%d)\n", __FUNCTION__, ret);

	return ret;
}

int vc_sen_start_stream(struct vc_ctrl *ctrl, struct vc_state *state)
{
	struct i2c_client *client_sen = ctrl->client_sen;
	struct i2c_client *client_mod = ctrl->client_mod;
	struct device *dev = &client_sen->dev;
	int ret = 0;

	dev_dbg(dev, "%s(): Start streaming\n", __FUNCTION__);

	if(ctrl->io_control == 1) {
		ret = vc_mod_set_ext_trig(client_mod, REG_EXTTRIG_DISABLE);
		ret |= vc_mod_set_flash_output(client_mod, state->flash_output);
	}

	// Start sensor streaming
	ret = vc_sen_write_table(client_sen, ctrl->start_table);
	if (ret)
		dev_err(dev, "%s(): Unable to start streaming (error=%d)\n", __FUNCTION__, ret);

	return ret;
}

int vc_sen_stop_stream(struct vc_ctrl *ctrl, struct vc_state *state)
{
	struct i2c_client *client_sen = ctrl->client_sen;
	struct i2c_client *client_mod = ctrl->client_mod;
	struct device *dev = &client_sen->dev;
	int ret = 0;

	dev_dbg(dev, "%s(): Stop streaming\n", __FUNCTION__);

	// ********************************************************************
	// TODO: Check if it really the "best" way to stop streaming by power down the sensor :)

	// ret = vc_mod_set_power(ctrl->client_mod, 0);
	// if (ret)
	// 	return ret;

	// mdelay(200);
	// vc_mod_get_status(client_mod); // Read status just for debugging

	// ret |= vc_mod_reset_module(client_mod, state->mode);
	// ********************************************************************

	if (ctrl->io_control == 1) {
		ret |= vc_mod_set_ext_trig(client_mod, REG_EXTTRIG_DISABLE);
		ret |= vc_mod_set_flash_output(client_mod, REG_IOCTRL_DISABLE);
	}
	if (state->ext_trig) {
	        ret |= vc_mod_set_exposure(client_mod, 0, 0);
	}

	// Stop sensor streaming
	ret |= vc_sen_write_table(client_sen, ctrl->stop_table);
	if (ret)
		dev_err(dev, "%s(): Unable to stop streaming (error=%d)\n", __FUNCTION__, ret);

	return ret;
}