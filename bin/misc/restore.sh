#/bin/bash
#
. config/configure.sh

scp $BUILD_DIR/boot/* root@$TARGET_NAME:/boot/