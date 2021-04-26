#/bin/bash
#
clear

. config/configure.sh

./patch.sh

#CONFIG=defconfig
CONFIG=toradex_defconfig

cd $KERNEL_SOURCE 
make $CONFIG

if [[ $1 == "a" || $1 == "k" ]]; then 
    echo "Build Kernel ..."       
    make -j$(nproc) Image.gz
fi

if [[ $1 == "a" || $1 == "m" ]]; then 
    echo "Build Modules ..."   
    rm -Rf $MODULES_DIR
    rm $BUILD_DIR/modules.tar.gz 
    
    make -j$(nproc) modules
    mkdir -p $MODULES_DIR
    export INSTALL_MOD_PATH=$MODULES_DIR
    make modules_install
    cd $MODULES_DIR
    echo Create module archive ...
    tar -czf ../modules.tar.gz .
fi

if [[ $1 == "a" || $1 == "d" ]]; then 
    echo "Build Device Tree ..."
    cd $KERNEL_SOURCE 
    #make dtbs
    make DTC_FLAGS="-@" freescale/imx8qm-apalis-eval.dtb
    make DTC_FLAGS="-@" freescale/imx8qm-apalis-ixora-v1.1.dtb
    make DTC_FLAGS="-@" freescale/imx8qm-apalis-v1.1-eval.dtb
    make DTC_FLAGS="-@" freescale/imx8qm-apalis-v1.1-ixora-v1.1.dtb
    
    echo "Build Device Tree Overlays ..."
    cd $KERNEL_SOURCE/arch/arm64/boot/dts/freescale
    # Verdin
    #cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp vc_mipi.dts vc_mipi.dts.preprocessed
    #dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o $DTB_FILE vc_mipi.dts.preprocessed
    
    # Apalis
    cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp $DTO_FILE.dts $DTO_FILE.dts.preprocessed
    dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o $DTO_FILE.dtbo $DTO_FILE.dts.preprocessed
fi

if [[ $1 == "v" ]]; then 
    cd $WORKING_DIR/src/vcmipidemo/src
    make clean
    make vcmipidemo
    cp $WORKING_DIR/src/vcmipidemo/src/vcmipidemo $WORKING_DIR/test
fi