# Version History

## v0.5.0 (V4L2 controls)
  * New Features
    * Trigger mode '0: disabled', '1: external', '2: pulsewidth', '3: self', '4: single', '5: sync', '6: stream_edge', '7: stream_level' can be set via V4L2 control 'trigger_mode'
    * Flash mode '0: disabled', '1: enabled' can be set via V4L2 control 'flash_mode'
    * Frame rate can be set via V4L2 control 'frame_rate' *(except IMX412 and OV9281)*
    * Black level can be set via V4L2 control 'black_level' *(only IMX178, IMX183 and IMX296)*
    * Single trigger can be set via V4L2 control 'single_trigger' *(under development)*
  * Removed Features
    * Removed device tree properties 'trigger_mode', 'flash_mode' and 'frame_rate'    

## v0.4.0 (Support all pixel formats)
  * Added support for VC MIPI Camera Modules
      * IMX183
      * IMX250, IMX252, IMX264, IMX265, IMX273, IMX392
      * IMX290, IMX327
      * IMX296
      * IMX412
      * IMX415
      * OV9281
  * New Features
    * Image Streaming in Y10, Y12, Y14, RG10, RG12, GB10, GB12 (2 bit right shifted)
    * Trigger mode '0: disabled', '1: external', '2: pulsewidth', '3: self', '4: single', '5: sync', '6: stream_edge', '7: stream_level' can be set via device tree property 'trigger_mode'
    * Flash mode '0: disabled', '1: enabled' can be set via device tree property 'flash_mode'
    * Frame rate can be set via device tree property 'frame_rate' *(except IMX412 and OV9281)*

## v0.3.0 (Support IMX178)
  * New Features
    * Linux Kernel Version 5.4.129
    * Image Streaming in Y10-RAW format (4 bit left shifted)
    * Camera support for IMX178
    * Support for EXTERNAL trigger mode

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
