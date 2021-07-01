#!/bin/bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Update tftp and nfs dirs while development and testing."
	echo ""
	echo "Supported options:"
        echo "-a, --all                 Copy kernel image, modules and device tree"
        echo "-d, --dt                  Copy device tree"
        echo "-h, --help                Show this help text"
        echo "-k, --kernel              Copy kernel image"
        echo "-m, --modules             Copy kernel modules"
        echo "-t, --test                Copy test tools"
}

configure() {
        . config/configure.sh
}

copy_kernel() {
        echo "Copy kernel ..."
        sudo cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $TFTP_DIR
}

copy_modules() {
        echo "Copy kernel modules ..."
        sudo tar xzvf $BUILD_DIR/modules.tar.gz -C $NFS_DIR
        sudo rm -Rf $NFS_DIR/var/log
        sudo mkdir $NFS_DIR/var/log
}

copy_device_tree() {
        echo "Copy device tree ..."
        sudo cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo $TFTP_DIR/overlays 
        sudo cp $WORKING_DIR/misc/apalis/overlays.txt $TFTP_DIR
}

copy_test_tools() {
        echo "Copy test tools ..."
        sudo mkdir -p $NFS_DIR/home/root/test
        sudo cp $WORKING_DIR/test/* $NFS_DIR/home/root/test
}

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-a|--all)
		configure
                copy_kernel
                copy_modules	
                copy_device_tree
                exit 0
		;;
        -d|--dt)
                configure
                copy_device_tree
                exit 0
		;;
	-h|--help)
		usage
		exit 0
		;;
	-k|--kernel)
		configure
		copy_kernel
                exit 0
		;;
        -m|--modules)
		configure
                copy_modules
                exit 0
		;;
        -t|--test)
		configure
                copy_test_tools		
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