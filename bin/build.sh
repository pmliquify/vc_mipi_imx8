#/bin/bash
#
. config/configure.sh $1

./patch.sh

if [[ -z $2 || $2 == "k" ]]; then 
    echo "Kernel"
    cd $KERNEL_SOURCE
    make defconfig
    make -j3 Image.gz 2>&1 | tee build.log
    cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $BUILD_DIR/image
fi

if [[ -z $2 || $2 == "d" ]]; then 
    echo "Device Tree"
    cd $KERNEL_SOURCE/arch/arm64/boot/dts/freescale
    cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp vc_mipi.dts vc_mipi.dts.preprocessed
    dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o vc_mipi.dtb vc_mipi.dts.preprocessed
    cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/vc_mipi.dtb $BUILD_DIR/image/$DTB_FILE
fi