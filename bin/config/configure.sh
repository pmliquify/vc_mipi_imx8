#!/usr/bin/env bash

WORKING_DIR=$(dirname "$PWD")
BIN_DIR=$WORKING_DIR/bin

. $BIN_DIR/config/config_apalis_iMX8.sh

BUILD_DIR=$WORKING_DIR/build/$SOM
PATCH_DIR=$WORKING_DIR/patch/$SOM
SRC_DIR=$WORKING_DIR/src/$SOM
DT_CAM_FILE=$WORKING_DIR/src/$SOM/arch/arm64/boot/dts/freescale/apalis-imx8_vc_mipi_overlay.dts
MISC_DIR=$WORKING_DIR/misc/$SOM

DOWNLOADS_DIR=$BUILD_DIR/downloads
KERNEL_SOURCE=$BUILD_DIR/linux-toradex
MODULES_DIR=$BUILD_DIR/modules
TFTP_DIR=/srv/tftp
NFS_DIR=/srv/nfs/rootfs

export CROSS_COMPILE=aarch64-none-linux-gnu-
export PATH=$BUILD_DIR/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/:$PATH
export DTC_FLAGS="-@"
export ARCH=arm64

TARGET_SHELL="ssh $TARGET_USER@$TARGET_IP"