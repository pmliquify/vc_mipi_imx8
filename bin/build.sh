#/bin/bash
#
clear

. config/configure.sh $1

./patch.sh

if [[ -z $2 || $2 == "k" ]]; then 
    echo "Build Kernel ..."
    cd $KERNEL_SOURCE
    make defconfig
    make -j3 Image.gz 2>&1 | tee build.log
    cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $BUILD_DIR/image
fi

if [[ -z $2 || $2 == "d" ]]; then 
    echo "Build Device Tree ..."
    cd $KERNEL_SOURCE/arch/arm64/boot/dts/freescale
    
    # Verdin
    #cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp vc_mipi.dts vc_mipi.dts.preprocessed
    #dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o $DTB_FILE vc_mipi.dts.preprocessed
    #cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTB_FILE $BUILD_DIR/image/$DTB_FILE
    
    # Apalis
    cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp $DTO_FILE.dts $DTO_FILE.dts.preprocessed
    dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o $DTO_FILE.dtbo $DTO_FILE.dts.preprocessed
    cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo $BUILD_DIR/image/$DTO_FILE.dtbo
fi