#/bin/bash
#
. config/configure.sh $1

./patch.sh

cd $KERNEL_SOURCE
make defconfig
make -j3 Image.gz 2>&1 | tee build.log
cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $BUILD_DIR/image

cd $KERNEL_SOURCE/arch/arm64/boot/dts/freescale
cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp vc_mipi.dts vc_mipi.dts.preprocessed
dtc -@ -Hepapr -I dts -O dtb -i $WORKING_DIR/linux-stable.git/arch/arm/boot/dts/ -o vc_mipi.dtb vc_mipi.dts.preprocessed
cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/vc_mipi.dtb $BUILD_DIR/image/imx8mm-verdin-wifi-v1.1-dev.dtb