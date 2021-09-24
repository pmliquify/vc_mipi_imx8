#/bin/bash
#
. config/configure.sh

mkdir -p $BUILD_DIR/boot
scp $TARGET_USER@$TARGET_IP:/boot/* $BUILD_DIR/boot