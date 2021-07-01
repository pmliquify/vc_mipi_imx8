#!/bin/bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Setup host and target for development and testing."
	echo ""
	echo "Supported options:"
	echo "-h, --help                Show this help text"
        echo "-k, --kernel              Setup/Reset kernel sources"
        echo "-n, --netboot             Setup netboot environment"
        echo "-o, --host                Installs some system tools, the toolchain and kernel sources"
}

configure() {
        . config/configure.sh
}

install_system_tools() {
        echo "Setup system tools."
        sudo apt update
        sudo apt install -y bc build-essential git libncurses5-dev lzop perl libssl-dev
        sudo apt install -y flex bison
        sudo apt install -y gcc-aarch64-linux-gnu
        sudo apt install -y device-tree-compiler
}

setup_toolchain() {
        echo "Setup tool chain."
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        rm -Rf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu
        wget -O gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz?revision=61c3be5d-5175-4db6-9030-b565aae9f766&la=en&hash=0A37024B42028A9616F56A51C2D20755C5EBBCD7"
        tar xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
        rm gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
}

setup_kernel() {
        echo "Setup kernel sources."
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        rm -Rf $KERNEL_SOURCE
        git clone -b toradex_5.4-2.3.x-imx git://git.toradex.com/linux-toradex.git $KERNEL_SOURCE
        cd $KERNEL_SOURCE
        # git checkout fca7e6292821c09a823ea6f471302b854d427f7f
        cp -R $SRC_DIR/* $KERNEL_SOURCE        
}

setup_dhcp_server() {
        echo "Setup dhcp server."
        sudo apt install -y isc-dhcp-server

        # /etc/default/isc-dhcp-server
        #       INTERFACESv4="enx98fc84ee15d7"

        # /etc/dhcp/dhcpd.conf
        #       subnet 192.168.10.0 netmask 255.255.255.0 {
        #                 default-lease-time              86400;
        #                 max-lease-time                  86400;
        #                 option broadcast-address        192.168.10.255;
        #                 option domain-name              "toradex.net";
        #                 option domain-name-servers      ns1.example.org;
        #                 option ip-forwarding            off;
        #                 option routers                  192.168.10.1;
        #                 option subnet-mask              255.255.255.0;
        #                 interface                       enx98fc84ee15d7;
        #                 range                           192.168.10.32 192.168.10.254;
        #       }
        #       #MAC address dependent IP assignment, used for the apalis target device
        #       host eval {
        #                 filename                        "Image.gz";
        #                 fixed-address                   192.168.10.2;
        #                 hardware ethernet               00:14:2d:67:d6:d8;
        #                 next-server                     192.168.10.1;
        #                 option host-name                "apalis-imx8";
        #                 option root-path                "/srv/nfs/rootfs,v4,tcp,clientaddr=0.0.0.0";
        #       }
        # sudo service isc-dhcp-server restart
}

setup_tftp_server() {
        echo "Setup tftp server."
        sudo apt install -y tftpd-hpa 
        sudo rm -Rf $TFTP_DIR/*
        sudo mkdir -p $TFTP_DIR
        cd $DOWNLOADS_DIR/$TEZI
        sudo tar xvfp Reference-Multimedia-Image-apalis-imx8.bootfs.tar.xz -C $TFTP_DIR

        # /etc/default/tftpd-hpa
        #       TFTP_USERNAME="tftp"
        #       TFTP_DIRECTORY="/srv/tftp"
        #       TFTP_ADDRESS="0.0.0.0:69"
        #       TFTP_OPTIONS="--secure"
        # sudo service tftpd-hpa restart
}

setup_nfs_server() {
        echo "Setup nfs server."
        sudo apt install -y nfs-kernel-server 
        sudo rm -Rf $NFS_DIR/*
        sudo mkdir -p $NFS_DIR
        cd $DOWNLOADS_DIR/$TEZI
        sudo tar xvfp Reference-Multimedia-Image-apalis-imx8.tar.xz -C $NFS_DIR

        # /etc/exports
        #       /srv/nfs 192.168.10.1/24(no_root_squash,no_subtree_check,rw)
        # sudo service nfs-kernel-server restart  
}

setup_u_boot() {
        echo "Setup u-boot server."
        #. ./netboot.sh recover

        # => setenv bootcmd 'run bootcmd_dhcp'
        # => saveenv

        # Boot from internal mmc again
        # => setenv bootcmd 'run distro_bootcmd'
        # => saveenv
}

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-h|--help)
		usage
		exit 0
		;;
	-k|--kernel)
		configure
		setup_kernel
                exit 0
		;;
	-o|--host)
		configure
                install_system_tools
                setup_toolchain
		setup_kernel
                exit 0
		;;
        -n|--netboot)
                configure
                setup_dhcp_server
                setup_tftp_server
                setup_nfs_server
                setup_u_boot
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