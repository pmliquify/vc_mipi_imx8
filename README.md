# Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8
![VC MIPI camera](https://www.vision-components.com/fileadmin/external/documentation/hardware/VC_MIPI_Camera_Module/VC_MIPI_Camera_Module_Hardware_Operating_Manual-Dateien/mipi_sensor_front_back.png)

Supported board: Toradex Apalis i.MX8   
Supported camera: VC MIPI IMX226 / VC MIPI IMX226C 

Linux kernel version: 5.4.91

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
  $ ./setup.sh
```

3. Build (all) the kernel image, kernel modules and device tree files.
```
  $ ./build.sh a
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

7. After boot up install the kernel modules we have build in step 3.
```
  $ ./flash.sh m
```

### Workaround 
Because of a bug in the Toradex Easy Installer Image 5.2.0 it is necessary to do a little workaround. Login, remove an recreate the folder /var/log.
```
  $ ssh root@apalis-imx8
  # rm -R /var/log
  # mkdir /var/log
  # reboot
```

# Testing the camera
The system should start properly and the Qt Cinematic Demo should be seen on the screen.   

1. First install the test tools to the target.
```
  $ ./test.sh
```

2. On the target switch to a console terminal by pressing Ctrl+Alt+F1

3. Login and check if the driver was loaded properly. You should see something like this in the second box.
```
  apalis-imx8 login: root
  # dmesg | grep 5-00
```
```
  [    3.478661] i2c 5-0010: VC MIPI Module - Hardware Descriptor
  [    3.484348] i2c 5-0010: [ MAGIC  ] [ mipi-module ]
  [    3.489156] i2c 5-0010: [ MANUF. ] [ Vision Components ] [ MID=0x0427 ]
  [    3.495780] i2c 5-0010: [ SENSOR ] [ SONY IMX226C ]
  [    3.500672] i2c 5-0010: [ MODULE ] [ ID=0x0226 ] [ REV=0x0008 ]
  [    3.506612] i2c 5-0010: [ MODES  ] [ NR=0x000c ] [ BPM=0x0010 ]
  [    3.929809] i2c 5-0010: VC MIPI Sensor succesfully initialized.
  [    4.749053] mx8-img-md: Registered sensor subdevice: vc-mipi-cam 5-001a (1)
  [    4.769124] mx8-img-md: created link [vc-mipi-cam 5-001a] => [mxc-mipi-csi2.1]
```

4. Start image aquisition by executing following commands. The folder *test* was installed by the script in step 1. 
```
  v4l2-ctl --set-fmt-video=pixelformat=RGGB,width=3840,height=3040
  ./test/vcmipidemo -fa
```
