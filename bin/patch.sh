#/bin/bash
#
. config/configure.sh $1

echo "Patching driver sources into kernel sources ..."
cp -Ruv $SRC_DIR/* $KERNEL_SOURCE