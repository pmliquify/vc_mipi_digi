#ifndef _VC_MIPI_IMX327_H
#define _VC_MIPI_IMX327_H

#include "vc_mipi_common.h"
#include <linux/v4l2-mediabus.h>

#define IMX327_SEN_REG_GAIN_L   0x0009
#define IMX327_SEN_REG_GAIN_M   0x000A
#define IMX327_SEN_REG_EXPO_L   0x000B
#define IMX327_SEN_REG_EXPO_M	0x000C

#define IMX327_SEN_REG_HMAX_L   0x7002
#define IMX327_SEN_REG_HMAX_H   0x7003
#define IMX327_SEN_REG_VMAX_L   0x7004
#define IMX327_SEN_REG_VMAX_M   0x7005
#define IMX327_SEN_REG_VMAX_H   0x7006

#define IMX327_EXPO_TIME_MIN1   160
#define IMX327_EXPO_TIME_MIN2   74480
#define IMX327_EXPO_TIME_MAX    10000000
#define IMX327_EXPO_NRLINES     3694
#define IMX327_EXPO_TOFFSET     47563  		// tOffset (U32)(2.903 * 16384.0)
#define IMX327_EXPO_H1PERIOD    327680 		// h1Period 20.00us => (U32)(20.000 * 16384.0)
#define IMX327_EXPO_VMAX        3728

/*----------------------------------------------------------------------*/
/*                                  IMX327                              */
/*----------------------------------------------------------------------*/
// IMX290, IMX327, IMX412, IMX415 sensor modes:
//  0 = 10-bit 2-lanes
//  1 = 10-bit 4-lanes (22_22 cable required)

#define IMX327_DIGITAL_GAIN_MIN        0
#define IMX327_DIGITAL_GAIN_MAX        240
#define IMX327_DIGITAL_GAIN_DEFAULT    0

/* In usec */
#define IMX327_DIGITAL_EXPOSURE_MIN     29          // microseconds (us)
#define IMX327_DIGITAL_EXPOSURE_MAX     7767184
#define IMX327_DIGITAL_EXPOSURE_DEFAULT 10000

#define IMX327_DX   1920
#define IMX327_DY   1080

static const struct vc_table_entry imx327_start[] = {
    {0x3000, 0x00},     /* mode select streaming on */
    {0x3002, 0x00},     /* mode select streaming on */
    {REG_TABLE_END, 0x00}
};

static const struct vc_table_entry imx327_stop[] = {
    {0x3002, 0x01},     /* mode select streaming off */
    {0x3000, 0x01},     /* mode select streaming off */
    {REG_TABLE_END, 0x00}
};

static const struct vc_table_entry imx327_mode_1920_1080[] = {
    { 0x3000, 0x1 },            // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby

    { 0x3473, IMX327_DX>>8 }, { 0x3472, IMX327_DX & 0xFF },   // X_OUT_SIZE[12:0] H,L = 0x3473,0x3472 = Horizontal (H) direction effective pixel width setting.
    { 0x3419, IMX327_DY>>8 }, { 0x3418, IMX327_DY & 0xFF },   // Y_OUT_SIZE[12:0] H,L = 0x3419,0x3418 = Vertical direction effective pixels

    { 0x3000, 0x0 },                // STANDBY[0]  = 0x3000 : Standby register: 0=Operating, 1=Standby
    {REG_TABLE_END, 0x00}
};

// // IMX327 - mono formats
// static struct vc_framefmt imx327_mono_fmts[] = {
//     { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
// };
// static int imx327_mono_fmts_size = ARRAY_SIZE(imx327_mono_fmts);

// IMX327 - color formats
static struct vc_framefmt imx327_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
};
static int imx327_color_fmts_size = ARRAY_SIZE(imx327_color_fmts);

#endif // _VC_MIPI_IMX327_H