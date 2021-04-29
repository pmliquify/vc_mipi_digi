#ifndef _VC_MIPI_IMX226_H
#define _VC_MIPI_IMX226_H

#include "vc_mipi_common.h"
#include <linux/v4l2-mediabus.h>

#define IMX226_SEN_REG_GAIN_L   0x0009
#define IMX226_SEN_REG_GAIN_M   0x000A

#define IMX226_SEN_REG_EXPO_L   0x000B
#define IMX226_SEN_REG_EXPO_M   0x000C
#define IMX226_SEN_REG_EXPO_H   0x0000

// #define IMX226_SEN_REG_HMAX_L   0x7002
// #define IMX226_SEN_REG_HMAX_H   0x7003

#define IMX226_SEN_REG_VMAX_L   0x7004
#define IMX226_SEN_REG_VMAX_M   0x7005
#define IMX226_SEN_REG_VMAX_H   0x7006

#define IMX226_EXPO_TIME_MIN1   160
#define IMX226_EXPO_TIME_MIN2   74480
#define IMX226_EXPO_TIME_MAX    10000000
#define IMX226_EXPO_NRLINES     3694
#define IMX226_EXPO_TOFFSET     47563  		// tOffset (U32)(2.903 * 16384.0)
#define IMX226_EXPO_H1PERIOD    327680 		// h1Period 20.00us => (U32)(20.000 * 16384.0)
#define IMX226_EXPO_VMAX        3728

/*----------------------------------------------------------------------*/
/*                                  IMX226                              */
/*----------------------------------------------------------------------*/
/* IMX183-IMX273 sensor modes : 0-2=8/10/12-bit (2 lanes), 3-5=8/10/12-bit ext.trigger (2 lanes), 6-11=...(4 lanes) */
// 0x00 :  8bit, 2 lanes, streaming
// 0x01 : 10bit, 2 lanes, streaming
// 0x02 : 12bit, 2 lanes, streaming
// 0x03 :  8bit, 2 lanes, external trigger global shutter reset
// 0x04 : 10bit, 2 lanes, external trigger global shutter reset
// 0x05 : 12bit, 2 lanes, external trigger global shutter reset
//
// 0x06 :  8bit, 4 lanes, streaming
// 0x07 : 10bit, 4 lanes, streaming
// 0x08 : 12bit, 4 lanes, streaming
// 0x09 :  8bit, 4 lanes, external trigger global shutter reset
// 0x0A : 10bit, 4 lanes, external trigger global shutter reset
// 0x0B : 12bit, 4 lanes, external trigger global shutter reset

/* In dB*10 */
#define IMX226_DIGITAL_GAIN_MIN        0x00
#define IMX226_DIGITAL_GAIN_MAX        0x7A5     // 1957
#define IMX226_DIGITAL_GAIN_DEFAULT    0x10

/* In usec */
#define IMX226_DIGITAL_EXPOSURE_MIN     160         // microseconds (us)
#define IMX226_DIGITAL_EXPOSURE_MAX     10000000
#define IMX226_DIGITAL_EXPOSURE_DEFAULT 10000

#if 1
  #define IMX226_DX 3840
  #define IMX226_DY 3040
#else    // test mode
  #define IMX226_DX 1440    // IMX273
  #define IMX226_DY 1080
//#define IMX226_DX 1920    // IMX327
//#define IMX226_DY 1080
//#define IMX226_DX 1280    // OV9281
//#define IMX226_DY 800
#endif

static const struct vc_table_entry imx226_start[] = {
    {0x7000, 0x01},         /* mode select streaming on */
    {REG_TABLE_END, 0x00}
};
static const struct vc_table_entry imx226_stop[] = {
    {0x7000, 0x00},         /* mode select streaming off */
    {REG_TABLE_END, 0x00}
};
/* MCLK:25MHz  3840 x 3040   MIPI LANE2 */
static const struct vc_table_entry imx226_mode_3840_3040[] = {
#if 1
    { 0x6015, IMX226_DX & 0xFF }, { 0x6016, (IMX226_DX>>8) & 0xFF },   // hor. output width  L,H  = 0x6015,0x6016,
    { 0x6010, IMX226_DY & 0xFF }, { 0x6011, (IMX226_DY>>8) & 0xFF },   // ver. output height L,H  = 0x6010,0x6011,
#endif
    {REG_TABLE_END, 0x00}
};

// // IMX226 - mono formats
// static struct vc_framefmt imx226_mono_fmts[] = {
//     { MEDIA_BUS_FMT_Y8_1X8,       V4L2_COLORSPACE_SRGB },   /* 8-bit grayscale pixel format  : V4L2_PIX_FMT_GREY 'GREY'     */
//     { MEDIA_BUS_FMT_Y10_1X10,     V4L2_COLORSPACE_SRGB },   /* 10-bit grayscale pixel format : V4L2_PIX_FMT_Y10  'Y10 '     */
//     { MEDIA_BUS_FMT_Y12_1X12,     V4L2_COLORSPACE_SRGB },   /* 12-bit grayscale pixel format : V4L2_PIX_FMT_Y12  'Y12 '     */
//     { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
// // use 8-bit 'RGGB' instead GREY format to save 8-bit frame(s) to raw file by v4l2-ctl
// };
// static int imx226_mono_fmts_size = ARRAY_SIZE(imx226_mono_fmts);

// IMX226 - color formats
static struct vc_framefmt imx226_color_fmts[] = {
    { MEDIA_BUS_FMT_SRGGB8_1X8,   V4L2_COLORSPACE_SRGB },   /* 8-bit color pixel format      : V4L2_PIX_FMT_SRGGB8  'RGGB'  */
    { MEDIA_BUS_FMT_SRGGB10_1X10, V4L2_COLORSPACE_SRGB },   /* 10-bit color pixel format     : V4L2_PIX_FMT_SRGGB10 'RG10'  */
    { MEDIA_BUS_FMT_SRGGB12_1X12, V4L2_COLORSPACE_SRGB },   /* 12-bit color pixel format     : V4L2_PIX_FMT_SRGGB12 'RG12'  */
};
static int imx226_color_fmts_size = ARRAY_SIZE(imx226_color_fmts);

#endif // _VC_MIPI_IMX226_H