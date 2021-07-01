#!/bin/bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Create a Toradex Easy Installer for Toradex Verdin i.MX8MP."
	echo ""
	echo "Supported options:"
	echo "-h, --help                Show this help text"
        echo "-m, --media               The destination media to copy TEZI to"
}

configure() {
        . config/configure.sh
}

setup_tezi() {
        if [[ ! -d $BUILD_DIR/$TEZI ]]; then
                echo "Setup Toradex Easy Installer Image ..."
                mkdir -p $BUILD_DIR
                cd $BUILD_DIR
                rm -f $TEZI.tar
                rm -Rf $TEZI
                wget $TEZI_URL/$TEZI.tar
                tar -xf $TEZI.tar
                rm -f $TEZI.tar
        fi
}

patch_bootfs() {
        cd $BUILD_DIR/$TEZI
        rm -Rf $BOOTFS
        mkdir -p $BOOTFS
        cd $BOOTFS
        tar -xJf ../$BOOTFS.tar.xz
        cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $BUILD_DIR/$TEZI/$BOOTFS
        cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/*.dtb $BUILD_DIR/$TEZI/$BOOTFS
        cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo $BUILD_DIR/$TEZI/$BOOTFS/overlays
        cp $MISC_DIR/* $BUILD_DIR/$TEZI/$BOOTFS
        tar -cJf ../$BOOTFS.tar.xz *
        cd ..
        rm -Rf $BOOTFS
}

patch_rootfs() {
        cd $BUILD_DIR/$TEZI
        rm -Rf $ROOTFS
        mkdir -p $ROOTFS
        cd $ROOTFS
        tar -xJf ../$ROOTFS.tar.xz
        rm -Rf ./lib/modules/*
        tar -xzf $BUILD_DIR/modules.tar.gz
        tar -cJvf ../$ROOTFS.tar.xz *
        cd ..
        rm -Rf $ROOTFS
}

patch_image_json() {
        cd $BUILD_DIR/$TEZI
        sed -i 's/"description": "Image for BSP verification with QT and multimedia features"/"description": "Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8"/g' image.json
        sed -i 's/"name": "Toradex Embedded Linux Reference Multimedia Image"/"name": "Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8"/g' image.json
}

patch_initial_env() {
        cd $BUILD_DIR/$TEZI
        echo "vidargs=video=HDMI-A-1:1280x720-16@60D video=HDMI-A-2:1280x720-16@60D" >> u-boot-initial-env-sd
}

patch_tezi() {
        echo "Patch Toradex Easy Installer Image with kernel and device tree ..."
        patch_bootfs
        # patch_rootfs
        patch_image_json
        # patch_initial_env       
}

copy_tezi() {
        if [[ ! -z ${media} ]]; then
                if [[ ! -d ${media} ]]; then
                        echo ${media} is not present!
                        exit 1
                fi

                echo "Copy Toradex Easy Installer to ${media}"
                cp -rf $BUILD_DIR/$TEZI/* ${media}
                umount ${media}        
        fi
}

media=

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-h|--help)
		usage
		exit 0
		;;
	-m|--media)
		media="$1"
                shift
		;;
	*)
		echo "Unknown option ${option}"
		exit 1
		;;
	esac
done

configure
setup_tezi
# patch_tezi
copy_tezi