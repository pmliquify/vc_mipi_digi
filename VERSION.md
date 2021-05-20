# Version History

## v0.3.0 (Exposure and Gain)
  * New Features
    * Exposure and Gain can be set via V4L2 library.
  * Known Issues
    * Handling for arbitrary image width and height not ready.     

## v0.2.0 (Image streaming with 4 bit left shifted image)
  * New Features
    * Image Streaming in SRGGB10-RAW format (4 bit left shifted).
    * vcmipidemo supports software implementation to correct the 4 bit left shift.
  * Bugfixes
    * Fixed build errors of dts files.
  * Known Issues
    * Exposure and Gain cannot be addressed via V4L2 library.
    * Handling for arbitrary image width and height not ready.

## v0.1.0 (Pre version for demonstration only)
  * New Features
    * Supported cameras VC MIPI IMX327 / VC MIPI IMX327C
    * Image streaming with faulty pixel formating
  * Known Issues
    * Exposure and Gain cannot be addressed via V4L2 library.
    * Image pixel formating does not work properly so far. This leads to an extrem noisy image.
    * The setup process in this documentation leads to some error messages while boot up.
    * Crashes in the imx8-isi driver while live image streaming.