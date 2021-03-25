#/bin/bash
#
. config/configure.sh $1

TARGET_DIR=/home/root/test

$TARGET_SHELL rm -R $TARGET_DIR
$TARGET_SHELL mkdir $TARGET_DIR
scp ../test/* root@verdin-imx8mm:$TARGET_DIR
$TARGET_SHELL chmod +x $TARGET_DIR/*.sh
$TARGET_SHELL . test/gst_play_327c.sh