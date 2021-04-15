#/bin/bash
#
. config/configure.sh $1

echo "Patching driver sources into kernel sources ..."
cp -Ruv $SRC_DIR/* $KERNEL_SOURCE
cp -Ruv $WORKING_DIR/src/linux_V1/* $KERNEL_SOURCE