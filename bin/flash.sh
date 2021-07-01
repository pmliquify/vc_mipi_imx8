#!/bin/bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Flash kernel image, modules, device tree, and test tools to the target."
	echo ""
	echo "Supported options:"
        echo "-a, --all                 Flash kernel image, modules and device tree"
        echo "-d, --dt                  Flash device tree"
        echo "-h, --help                Show this help text"
        echo "-k, --kernel              Flash kernel image"
        echo "-m, --modules             Flash kernel modules"
        echo "-r, --reboot              Reboot after flash."
        echo "-t, --test                Flash test tools"
}

configure() {
        . config/configure.sh
}

flash_kernel() {
        echo "Flash kernel ..."
        scp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $TARGET_USER@$TARGET_IP:/boot
}

flash_modules() {
        echo "Flash kernel modules ..."
        scp $BUILD_DIR/modules.tar.gz $TARGET_USER@$TARGET_IP:/home/$TARGET_USER
        $TARGET_SHELL tar -xzvf modules.tar.gz -C /
        $TARGET_SHELL rm -f modules.tar.gz
        $TARGET_SHELL rm -Rf /var/log
}

flash_device_tree() {
        echo "Flash device tree ..."
        # Verdin
        #scp $BUILD_DIR/image/$DTB_FILE root@$TARGET_NAME:/boot 
        
        # Apalis
        scp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo root@$TARGET_NAME:/boot/overlays 
        scp $WORKING_DIR/misc/apalis/overlays.txt root@$TARGET_NAME:/boot
}

flash_test_tools() {
        echo "Flash test tools ..."
        TARGET_DIR=/home/$TARGET_USER/test
        $TARGET_SHELL rm -Rf $TARGET_DIR
        $TARGET_SHELL mkdir -p $TARGET_DIR
        scp $WORKING_DIR/test/* $TARGET_USER@$TARGET_NAME:$TARGET_DIR
}

reboot_target() {
        if [[ -n ${reboot} ]]; then
                $TARGET_SHELL /sbin/reboot
        fi
}

reboot=

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-a|--all)
		configure
                flash_kernel
                flash_modules	
                flash_device_tree
                reboot_target
                exit 0
		;;
        -d|--dt)
                configure
                flash_device_tree
                reboot_target
                exit 0
		;;
	-h|--help)
		usage
		exit 0
		;;
	-k|--kernel)
		configure
		flash_kernel
                reboot_target
                exit 0
		;;
        -m|--modules)
		configure
                flash_modules
                reboot_target
                exit 0
		;;
	-r|--reboot)
                reboot=1
                ;;
        -t|--test)
		configure
                flash_test_tools		
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