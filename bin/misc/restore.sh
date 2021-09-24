#/bin/bash
#
. config/configure.sh

scp $BUILD_DIR/boot/* $TARGET_USER@$TARGET_IP:/boot/