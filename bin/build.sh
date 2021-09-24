#!/bin/bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Build kernel image, modules, device tree, u-boot script and test tools."
	echo ""
	echo "Supported options:"
        echo "-a, --all                 Build kernel image, modules and device tree"
        echo "-d, --dt                  Build device tree"
        echo "-h, --help                Show this help text"
        echo "-k, --kernel              Build kernel image"
        echo "-m, --modules             Build kernel modules"
        echo "-t, --test                Build test tools"
}

configure() {
        . config/configure.sh
}

patch_kernel() {
        echo "Patching driver sources into kernel sources ..."
        cp -Ruv $SRC_DIR/* $KERNEL_SOURCE
}

configure_kernel() {
        cd $KERNEL_SOURCE
        make toradex_defconfig
}

build_kernel() {
        echo "Build kernel ..."
        cd $KERNEL_SOURCE
        make -j$(nproc) Image.gz
}

build_modules() {
        echo "Build kernel modules ..."
        cd $KERNEL_SOURCE
        make -j$(nproc) modules 
}

create_modules() {
        rm -Rf $MODULES_DIR
        mkdir -p $MODULES_DIR
        export INSTALL_MOD_PATH=$MODULES_DIR
        make modules_install
        cd $MODULES_DIR
        echo Create module archive ...
        rm -f $BUILD_DIR/modules.tar.gz 
        tar -czf ../modules.tar.gz .
        rm -Rf $MODULES_DIR    
}

build_device_tree() {
        echo "Build device tree ..."
        cd $KERNEL_SOURCE
     
        make -j$(nproc) DTC_FLAGS="-@" freescale/imx8qm-apalis-eval.dtb
        make -j$(nproc) DTC_FLAGS="-@" freescale/imx8qm-apalis-ixora-v1.1.dtb
        make -j$(nproc) DTC_FLAGS="-@" freescale/imx8qm-apalis-v1.1-eval.dtb
        make -j$(nproc) DTC_FLAGS="-@" freescale/imx8qm-apalis-v1.1-ixora-v1.1.dtb

        echo "Build Device Tree Overlays ..."
        cd $KERNEL_SOURCE/arch/arm64/boot/dts/freescale
        # Verdin
        #cpp -nostdinc -I $KERNEL_SOURCE/arch/arm64/boot/dts -I $KERNEL_SOURCE/include -undef -x assembler-with-cpp vc_mipi.dts vc_mipi.dts.preprocessed
        #dtc -@ -Hepapr -I dts -O dtb -i $KERNEL_SOURCE/arch/arm64/boot/dts/ -o $DTB_FILE vc_mipi.dts.preprocessed
        
        # Apalis
        cpp -undef -nostdinc -x assembler-with-cpp -I $KERNEL_SOURCE/include $DTO_FILE.dts $DTO_FILE.dts.preprocessed
        # -undef 
        #    Do not predefine any system-specific or GCC-specific macros.  The standard predefined macros remain defined
        # -nostdinc
        #    Do not search the standard system directories for header files. Only the directories explicitly specified with -I,
        #   -iquote, -isystem, and/or -idirafter options (and the directory of the current file, if appropriate) are searched.
        # -x <language>
        dtc -@ -q -I dts -O dtb -o $DTO_FILE.dtbo $DTO_FILE.dts.preprocessed
        # Usage: dtc [options] <input file>
        # -@ 
        #    Enable generation of symbols
        # -H 
        #    Valid phandle formats are:
        #            legacy - "linux,phandle" properties only
        #            epapr  - "phandle" properties only
        #            both   - Both "linux,phandle" and "phandle" properties
        # -I, --in-format <arg>      
        #    Input formats are:
        #            dts - device tree source text
        #            dtb - device tree blob
        #            fs  - /proc/device-tree style directory
        # -O, --out-format <arg>     
        #    Output formats are:
        #            dts - device tree source text
        #            dtb - device tree blob
        #            yaml - device tree encoded as YAML
        #            asm - assembler source
        # -o, --out <arg>            
        #    Output file
}

build_test_tools() {
        echo "Build test tools ..."
        cd $WORKING_DIR/src/vcmipidemo/linux
        make clean
        make
        mkdir -p $WORKING_DIR/test
        mv -f vcmipidemo $WORKING_DIR/test
        mv -f vcimgnetsrv $WORKING_DIR/test
}

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-a|--all)
		configure
                patch_kernel
                configure_kernel
                build_kernel
                build_modules	
                create_modules
                build_device_tree
                exit 0
		;;
        -d|--dt)
		configure
                patch_kernel
                configure_kernel
                build_device_tree
                exit 0
		;;
	-h|--help)
		usage
		exit 0
		;;
	-k|--kernel)
		configure
                patch_kernel
                configure_kernel
		build_kernel
                exit 0
		;;
        -m|--modules)
		configure
                patch_kernel
                configure_kernel
                build_modules
                create_modules
                exit 0
		;;
        -t|--test)
		configure
                build_test_tools		
                exit 0
		;;
	*)
		echo "Unknown option ${option}"
		exit 1
		;;
	esac
done

usage
exit 1