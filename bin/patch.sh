#/bin/bash
#
. config/configure.sh

echo "Patching driver sources into kernel sources ..."
CP_FLAGS=-Ruv
if [[ $1 == "f" ]]; then
        CP_FLAGS=-Rv       
fi
cp $CP_FLAGS $SRC_DIR/* $KERNEL_SOURCE