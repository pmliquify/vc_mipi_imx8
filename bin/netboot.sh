#/bin/bash
#
. config/configure.sh

if [[ $1 == "all" || $1 == "k" ]]; then
        echo "Install kernel image ..."
        sudo cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $TFTP_DIR/Image.gz
fi

if [[ $1 == "all" || $1 == "m" ]]; then
        echo "Install kernel modules ..."
        sudo tar -xzvf $BUILD_DIR/modules.tar.gz -C $NFS_DIR
        sudo rm -Rf $NFS_DIR/var/log
        sudo mkdir $NFS_DIR/var/log
fi

if [[ $1 == "all" || $1 == "d" ]]; then
        echo "Install device tree files ..."
        sudo cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo $TFTP_DIR/overlays 
        sudo cp $WORKING_DIR/misc/apalis/overlays.txt $TFTP_DIR
fi

if [[ $1 == "test" ]]; then
        sudo mkdir -p $NFS_DIR/home/root/test
        sudo cp $WORKING_DIR/test/* $NFS_DIR/home/root/test
fi

if [[ $1 == "recover" ]]; then
        sudo rm -Rf $TFTP_DIR/*
        sudo rm -Rf $NFS_DIR/*
        cd $DOWNLOADS_DIR/$TEZI
        sudo tar xvfp Reference-Multimedia-Image-apalis-imx8.bootfs.tar.xz -C $TFTP_DIR
        sudo tar xvfp Reference-Multimedia-Image-apalis-imx8.tar.xz -C $NFS_DIR
        rm -Rf ~/.ssh/known_hosts
fi

if [[ $1 == "all" || $1 == "k" || $1 == "m" || $1 = "d" ]]; then
        echo "Reboot target ..."
        $TARGET_SHELL /sbin/reboot
fi