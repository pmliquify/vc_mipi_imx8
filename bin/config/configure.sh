#/bin/bash
#

WORKING_DIR=$(dirname $PWD)
BIN_DIR=$WORKING_DIR/bin
BUILD_DIR=$WORKING_DIR/build
KERNEL_SOURCE=$BUILD_DIR/linux-toradex

#. $BIN_DIR/config/config_verdin.sh
. $BIN_DIR/config/config_apalis.sh

export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
export DTC_FLAGS='-@'