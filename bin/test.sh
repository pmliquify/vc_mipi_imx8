#/bin/bash
#
# To deactivate the Qt5 Cinematic Experience demo application 
# systemctl disable wayland-app-launch.service
#
. config/configure.sh

TARGET_DIR=/home/root/test

$TARGET_SHELL rm -R $TARGET_DIR
$TARGET_SHELL mkdir $TARGET_DIR
scp ../test/* root@$TARGET_NAME:$TARGET_DIR
$TARGET_SHELL chmod +x $TARGET_DIR/*.sh
#$TARGET_SHELL . test/gst_play_226.sh