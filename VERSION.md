# Version History

## v0.3.0 (Support IMX178)
  * New Features
    * Linux Kernel Version 5.4.129
    * Image Streaming in Y10-RAW format (4 bit left shifted)
    * Camera support for IMX178

## v0.2.1 (BugFix)
  * BugFixes
    * Added missing include of SoC dtsi in apalis-imx8_vc_mipi_overlay.dts

## v0.2.0 (Image streaming with 4 bit left shifted image)
  * New Features
    * Image Streaming in SGBRG10-RAW format (4 bit left shifted)
    * vcmipidemo supports software implementation to correct the 4 bit left shift

## v0.1.0 (Pre version for demonstration only)
  * Supported boards
    * Toradex Ixora Carrier Board V1.2A
  * Supported cameras
    * VC MIPI IMX226 / VC MIPI IMX226C
  * Supported linux kernel versions
    * 5.4.115
  * New Features
    * Image streaming (with faulty pixel formating)
    * Exposure and Gain can addressed via V4L2 library
  * Known Issues
    * Image pixel formating does not work properly so far. This leads to an extrem noisy image
    * Trigger in and flash out not supported yet
    * Cropping not supported yet
