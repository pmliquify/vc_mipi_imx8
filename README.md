# Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8
![VC MIPI camera](https://www.vision-components.com/fileadmin/external/documentation/hardware/VC_MIPI_Camera_Module/VC_MIPI_Camera_Module_Hardware_Operating_Manual-Dateien/mipi_sensor_front_back.png)

## Version 0.4.0 ([History](VERSION.md))
* Supported system on modules
  * [Toradex Apalis iMX8](https://www.toradex.com/de/computer-on-modules/apalis-arm-family/nxp-imx-8)
* Supported carrier boards
  * [Toradex Ixora Carrier Board V1.2A](https://www.toradex.com/de/products/carrier-board/ixora-carrier-board)
* Supported [VC MIPI Camera Modules](https://www.vision-components.com/fileadmin/external/documentation/hardware/VC_MIPI_Camera_Module/index.html) 
  * IMX178, IMX183, IMX226
  * IMX250, IMX252, IMX264, IMX265, IMX273, IMX392
  * IMX290, IMX327
  * IMX296
  * IMX412
  * IMX415
  * OV9281
* Linux kernel 
  * Version 5.4.129
* Features
  * Image Streaming in Y10, Y12, Y14, RG10, RG12, GB10, GB12 (2 bit right shifted)
  * **Exposure** can be set via V4L2 API 'exposure'
  * **Gain** can be set via V4L2 API 'gain'
  * **Trigger mode** '0: disabled', '1: external', '2: pulsewidth', '3: self', '4: single', '5: sync', '6: stream_edge', '7: stream_level' can be set via device tree property 'trigger_mode'
  * **Flash mode** '0: disabled', '1: enabled' can be set via device tree property 'flash_mode'
  * **Frame rate** can be set via device tree property 'frame_rate' *(except IMX412 and OV9281)*

## Prerequisites for cross-compiling
### Host PC
* Recommended OS is Ubuntu 18.04 LTS or Ubuntu 20.04 LTS
* You need git to clone this repository
* All other packages are installed by the scripts contained in this repository

# Installation
> When we use the **$** sign it is meant that the command is executed on the host PC. A **#** sign indicates the promt from the target.

1. Download the Apalis-iMX8_Reference-Multimedia-Image with integrated VC MIPI camera driver from [Releases](https://github.com/pmliquify/vc_mipi_imx8/releases).  

2. Unzip the archive and copy all the contents onto an empty FAT formatted USB stick.

3. Create a directory and clone the repository   
   ```
     $ cd <working_dir>
     $ git clone https://github.com/pmliquify/vc_mipi_imx8
   ```

4. Setup the toolchain and the kernel sources. The script will additionaly install some necessary packages like *build-essential* and the *device-tree-compiler*.
   ```
     $ cd vc_mipi_imx8/bin
     $ ./setup.sh --host
   ```

5. Enter recovery mode by following the [imx-recovery-mode](https://developer.toradex.com/knowledge-base/imx-recovery-mode) instructions.   
We provide a script to easily flash an image. It will download the tools from toradex and start to watch for a matching usb device to flash to.
   ```
     $ ./recover.sh
   ```

6. Plugin the USB stick and install the image.

7. After boot up, adjust the IP address of your target in the configuration file.
   ```
     $ nano config/config_apalis_iMX8.sh
   ```

8. You have to install a first version of the device tree overlay file to load the camera driver. To achieve that follow the instructions in the next section.

# Changing camera settings in the device tree
If you want to change some settings of a camera in the device tree, please follow these steps.

1. Edit the device tree file for your hardware setup. To edit the device tree file you can simply use the setup script. It will open the device tree file in the nano editor.
   ```
     $ ./setup.sh --camera
   ```

2. Build and flash the device tree file to the target. The `--reboot` parameter causes the target to be restarted directly. 
   ```
     $ ./build.sh --dt
     $ ./flash.sh --reboot --dt
   ```

3. Login to your target and check if the driver was loaded properly. You should see something like this in the second box.
   ```
     apalis-imx8 login: root
     # dmesg | grep 5-00
   ```
   ```
     [    3.535176] vc-mipi-cam 5-001a: vc_mod_setup(): Setup the module
     [    3.798698] i2c 5-0010: +--- VC MIPI Camera -----------------------------------+
     [    3.806134] i2c 5-0010: | MANUF. | Vision Components               MID: 0x0427 |
     [    3.813562] i2c 5-0010: | MODULE | ID:  0x0178                     REV:   0000 |
     [    3.820984] i2c 5-0010: | SENSOR | SONY IMX178                                 |
     [    3.828412] i2c 5-0010: +--------+---------------------------------------------+
     [    3.835837] i2c 5-0010: +--- Sensor Registers ------+--------+--------+--------+
     [    3.843272] i2c 5-0010: |                           | low    | mid    | high   |
     [    3.850694] i2c 5-0010: +---------------------------+--------+--------+--------+
     [    3.858121] i2c 5-0010: | idle                      | 0x7000 |        |        |
     [    3.865540] i2c 5-0010: | horizontal start          | 0x6013 | 0x6014 |        |
     [    3.872968] i2c 5-0010: | vertical start            | 0x600e | 0x600f |        |
     ...
   ```

# Special pixel format
According to the **i.MX 8QuadMax Applications Processor Reference Manual** (iMX8QM_RM_Rev_F.pdf, Page 5300, Section 15.2.1.3.6 CSI-2 Rx Controller Core Data Types Formatting) all RAW pixel formats a left aligned formated. The MSB for RAW08 to RAW14 format starts on BIT[13]. Bit[15] and Bit[14] are zero. Unfortunatly it is necessary to transmit 8 bit pixel format in 16 bit memory layout to obtain all data.

> **<center>NOTE** (Quote from manual)</center> 
The CSI data is right LSB aligned and zero padded depending on data format. When interfacing ISI, CSI data is shifted 6-bits due to ISI bits [5:0] always being zero (0bxxCSIDATAxxxxxx). All RAW14, RAW12, RAW10, RAW8, and RAW6 video data is filled from BIT[13] to LSB, the remaining bits are zero padded. Only RAW16 input data will be saved to memory from BIT[15] to LSB.

For the different pixel formats you will get the following memory layout. x stands for a value of zero.
```
              MSB
               v
  RAW08 -> 0bxx76543210xxxxxx
  RAW10 -> 0bxx9876543210xxxx
  RAW12 -> 0bxxba9876543210xx
  RAW14 -> 0bxxdcba9876543210
```

> In your application, you must take this into account when interpreting the pixel data. The ```-2``` parameter of the vcmipidemo application does this for demonstration purposes.

# Testing the camera

1. On the target switch to a console terminal by pressing Ctrl+Alt+F1

2. Setup the image width, height and the pixel format
   ```
     # v4l2-ctl --set-fmt-video=width=3072,height=2076,pixelformat='Y14 '
   ```
   In the table you will find the correct settings for your camera module.

   | Camera | width       | height      | RAW08 | RAW10 | RAW12 | RAW14 |
   | ------ | ----------- | ----------- | ----- | ----- | ----- | ----- |
   | IMX178 |        3072 |        2076 |     X |     X |     X |     X |
   | IMX183 |        5440 |        3648 |       |     X |     X |       |
   | IMX226 |        3840 |        3046 |       |     X |     X |       |
   | IMX250 |        2448 |        2048 |     X |     X |     X |       |
   | IMX252 |        2048 |        1536 |     X |     X |     X |       |
   | IMX264 |        2432 |        2048 |     X |     X |     X |       |
   | IMX265 |        2048 |        1536 |     X |     X |     X |       |
   | IMX273 |        1440 |        1080 |     X |     X |     X |       |
   | IMX290 |        1920 |        1080 |       |     X |     X |       |
   | IMX296 |        1440 |        1080 |       |     X |       |       |
   | IMX327 |        1920 |        1080 |       |     X |     X |       |
   | IMX392 |        1920 |        1200 |     X |     X |     X |       |
   | IMX412 |        4032 |        3040 |       |     X |       |       |
   | IMX415 |        3840 |        2160 |       |     X |       |       |
   | OV9281 |        1280 |         800 |     X |     X |       |       |

3. Start image aquisition by executing the preinstalled vcmipidemo application.
   ```
     # vcmipidemo -fa -y1 -2 -s 100000
   ```
   | Parameter | Description
   | --------- | --------------------------------------------------------- |
   | -fa       | Activates framebuffer output and deactivates ASCII output |
   | -y1       | Prints the first six bytes in binary format               |
   | -2        | Shifts pixel date two bits to the left                    |
   | -s        | Sets the shutter time in ms                               |
   ```
     Suppressing ASCII capture at stdout.
     Activating /dev/fb0 framebuffer output.
     Printing image info for every acquired image.
     Setting Shutter Value to 100000 us.
     [#0001, ts: 7321301, t:   0 ms] (fmt: Y14 , dx: 3072, dy: 2076, pitch: 6144) - 0000000110111011 0000000111100100 0000000111011011 
     [#0002, ts: 7321401, t: 100 ms] (fmt: Y14 , dx: 3072, dy: 2076, pitch: 6144) - 0000000111111100 0000001000010111 0000001000001111 
     [#0003, ts: 7321501, t: 100 ms] (fmt: Y14 , dx: 3072, dy: 2076, pitch: 6144) - 0000001000010010 0000000111111010 0000001000001000 
     [#0004, ts: 7321601, t: 100 ms] (fmt: Y14 , dx: 3072, dy: 2076, pitch: 6144) - 0000000111100111 0000000111110010 0000000111111110 
     ...
   ```