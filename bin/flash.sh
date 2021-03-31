#/bin/bash
#
. config/configure.sh $1

./build.sh $1 $2

scp $BUILD_DIR/image/Image.gz root@$TARGET_NAME:/boot
scp $BUILD_DIR/image/$DTB_FILE root@$TARGET_NAME:/boot
$TARGET_SHELL /sbin/reboot