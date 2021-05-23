#/bin/bash
#

WORKING_DIR=$(dirname $PWD)
BIN_DIR=$WORKING_DIR/bin

#. $BIN_DIR/config/config_verdin.sh
. $BIN_DIR/config/config_apalis.sh

BUILD_DIR=$WORKING_DIR/build/$BOARD
SRC_DIR=$WORKING_DIR/src/$BOARD
MISC_DIR=$WORKING_DIR/misc/$BOARD

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