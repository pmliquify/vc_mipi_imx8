#/bin/bash
#
. config/configure.sh $1

scp $BUILD_DIR/image/* root@verdin-imx8mm:/boot
$TARGET_SHELL /sbin/reboot