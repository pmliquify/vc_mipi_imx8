# Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8
![VC MIPI camera](https://www.vision-components.com/fileadmin/external/documentation/hardware/VC_MIPI_Camera_Module/VC_MIPI_Camera_Module_Hardware_Operating_Manual-Dateien/mipi_sensor_front_back.png)

## Version 0.3.0 ([History](VERSION.md))
* Supported boards
  * Toradex Ixora Carrier Board V1.2A
* Supported cameras 
  * VC MIPI IMX226 / VC MIPI IMX226C  
  * VC MIPI IMX178
* Linux kernel 
  * Version 5.4.129
* Features
  * Image Streaming in Y10 and SGBRG10 format (4 bit left shifted).
  * Exposure and Gain can be set via V4L2 library.
  * vcmipidemo supports software implementation to correct the 4 bit left shift.

## Prerequisites for cross-compiling
### Host PC
* Recommended OS is Ubuntu 18.04 LTS or Ubuntu 20.04 LTS
* You need git to clone this repository
* All other packages are installed by the scripts contained in this repository

# Installation
When we use the **$** sign it is meant that the command is executed on the host PC. A **#** sign indicates the promt from the target so the execution on the target. In our case the Ixora Apalis board.

1. Create a directory and clone the repository   
   ```
     $ cd <working_dir>
     $ git clone https://github.com/pmliquify/vc_mipi_toradex
   ```

2. Setup the toolchain and the kernel sources. The script will additionaly install some necessary packages like *build-essential* or *device-tree-compiler*.
   ```
     $ cd vc_mipi_toradex/bin
     $ ./setup.sh host
   ```

3. Build (all) the kernel image, kernel modules and device tree files.
   ```
     $ ./build.sh all
   ```

4. Create a new Toradex Easy Installer Image. Insert a USB stick (FAT formated) with minimum 1GB capacity. The script will download the reference image from toradex patch it with the build kernel and device tree files from step 3 and copy the image to the usb stick.
   ```
     $ ./misc/create_tezi.sh c /media/<username>/<usb-stick-name>
   ```

5. Enter recovery mode by following the [imx-recovery-mode](https://developer.toradex.com/knowledge-base/imx-recovery-mode) instructions.   
We provide a script to easily flash an image. It will download the tools from toradex and start to watch for a matching usb device to flash to.
   ```
     $ ./recover.sh
   ```

6. Plugin the USB stick and install the prepared image.

7. After boot up, install the kernel modules we have build in step 3.
   ```
     $ ./flash.sh m
   ```

# Testing the camera
The system should start properly and the Qt Cinematic Demo should be seen on the screen.   

1. First install the test tools to the target.
   ```
     $ ./build.sh test
     $ ./setup.sh test
   ```

2. On the target switch to a console terminal by pressing Ctrl+Alt+F1

3. Login and check if the driver was loaded properly. You should see something like this in the second box.
   ```
     apalis-imx8 login: root
     # dmesg | grep 5-00
   ```
   ```
     [    3.535176] vc-mipi-cam 5-001a: vc_mod_setup(): Setup the module
     [    3.798698] i2c 5-0010: +--- VC MIPI Camera -----------------------------------+
     [    3.806134] i2c 5-0010: | MANUF. | Vision Components               MID: 0x0427 |
     [    3.813562] i2c 5-0010: | MODULE | ID:  0x0178                     REV: 0x0000 |
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

4. Start image aquisition by executing following commands. The folder *test* was installed by the script in step 1.   
   **Please note the option -afx4 to suppress ASCII output, output the image to the framebuffer, output image informations and apply the 4 bit shift correction**
   ```
     # v4l2-ctl --set-fmt-video=pixelformat=GB10,width=3840,height=3040
     # ./test/vcmipidemo -afx4 -s 2000 -g 10
     img.org (dx: 3840, dy: 3040, pitch: 7680) - 9024 5022 f025 0022 7024 a022 0025 4022 2025 b022 
     img.org (dx: 3840, dy: 3040, pitch: 7680) - 4025 7021 b024 7022 1025 1022 9025 8022 2025 7022 
     img.org (dx: 3840, dy: 3040, pitch: 7680) - 0025 f021 8024 d022 2025 5022 e024 7022 6025 1022 
     img.org (dx: 3840, dy: 3040, pitch: 7680) - e024 7022 6025 4022 4024 4022 f024 c022 d024 e021
     ...
   ```

5. The image information output shows the first 20 byte of the image raw data. In your application you have to correct the 4 bit shift while debayering raw image data.
   ```
                                                G    B    G    B    ...
    img.org (dx: 3840, dy: 3040, pitch: 7680) - 9024 5022 f025 0022 7024 a022 0025 4022 2025 b022
                                                  ^    ^    ^    ^  ...
                                                  This are the MSBs (most significant bits)
                                                  A color component is represented as little endian
                                                  
                                                0902 4502 2f02 5002 2702 4a02 2002 5402 2202 5b02
                        >> 4 bit right shifted     ^    ^    ^    ^    ^    ^    ^    ^    ^    ^
                                    big endian  0209 0245 022f 0250 0227 024a 0220 0254 0222 025b
                                       decimal   521  581  559  592  551  586  544  596  546  603
   ```
