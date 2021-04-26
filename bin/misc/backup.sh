#/bin/bash
#
. config/configure.sh

mkdir -p $BUILD_DIR/boot
scp root@$TARGET_NAME:/boot/* $BUILD_DIR/boot