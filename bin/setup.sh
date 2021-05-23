#/bin/bash
#
. config/configure.sh

CMD=$1

if [[ $CMD == "host" ]]; then
        # Install Linux Tools
        # *****************************************************************************
        sudo apt update
        sudo apt install -y bc build-essential git libncurses5-dev lzop perl libssl-dev
        sudo apt install -y flex bison
        sudo apt install -y gcc-aarch64-linux-gnu
        sudo apt install -y device-tree-compiler

        mkdir -p $BUILD_DIR
        cd $BUILD_DIR

        # Install Toolchain
        # *****************************************************************************
        wget -O gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz?revision=61c3be5d-5175-4db6-9030-b565aae9f766&la=en&hash=0A37024B42028A9616F56A51C2D20755C5EBBCD7"
        tar xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
        rm gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz

        # Install Sources
        # *****************************************************************************
        # -----------------------------------------------------------------------------
        # Toradex Easy Installer -> 5.1.0-devel-202012+build.5 (2020-12-01)
        #
        #git clone -b toradex_imx_v2020.04_5.4.24_2.1.0 git://git.toradex.com/u-boot-toradex.git
        #git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/device-tree-overlays.git
        #git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/linux-toradex.git

        # Repo Kernel
        # Linux apalis-imx8 5.4.77-31031-g322691f7b572 #1 SMP PREEMPT Fri Apr 23 13:15:42 CEST 2021 aarch64 aarch64 aarch64 GNU/Linux

        # -----------------------------------------------------------------------------
        # Toradex Easy Installer -> 5.2.0+build.7 (2021-04-08)
        #
        #git clone -b toradex_imx_v2020.04_5.4.70_2.3.0 git://git.toradex.com/u-boot-toradex.git
        #sudo rm -Rf device-tree-overlays
        #git clone -b toradex_5.4-2.3.x-imx git://git.toradex.com/device-tree-overlays.git
        sudo rm -Rf linux-toradex
        git clone -b toradex_5.4-2.3.x-imx git://git.toradex.com/linux-toradex.git

        # Easy Installer Kernel
        # Linux apalis-imx8 5.4.91-5.2.0+git.6afb048a71e3 #1 SMP PREEMPT Wed Apr 7 08:36:44 UTC 2021 aarch64 aarch64 aarch64 GNU/Linux
        # Repo Kernel
        # Linux apalis-imx8 5.4.91-33088-g8c05b31a44c3 #1 SMP PREEMPT Mon Apr 19 15:11:10 CEST 2021 aarch64 aarch64 aarch64 GNU/Linux
fi

if [[ $CMD == "netboot" ]]; then
        # DHCP Server
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

        # TFTP Server        
        sudo apt install -y tftpd-hpa 
        sudo mkdir -p $TFTP_DIR

        # /etc/default/tftpd-hpa
        #       TFTP_USERNAME="tftp"
        #       TFTP_DIRECTORY="/srv/tftp"
        #       TFTP_ADDRESS="0.0.0.0:69"
        #       TFTP_OPTIONS="--secure"
        # sudo service tftpd-hpa restart

        # NFS Server
        sudo apt install -y nfs-kernel-server 
        sudo mkdir -p $NFS_DIR

        # /etc/exports
        #       /srv/nfs 192.168.10.1/24(no_root_squash,no_subtree_check,rw)
        # sudo service nfs-kernel-server restart

        mkdir -p $DOWNLOADS_DIR
        cd $DOWNLOADS_DIR
        
        rm -Rf $TEZI
        wget $TEZI_URL/$TEZI.tar
        tar xf $TEZI.tar
        rm -f $TEZI.tar

        #. ./netboot.sh recover

        # => setenv bootcmd 'run bootcmd_dhcp'
        # => saveenv

        # Boot from internal mmc again
        # => setenv bootcmd 'run distro_bootcmd'
        # => saveenv
fi

if [[ $CMD == "test" ]]; then
        #rm ~/.ssh/known_hosts
        #ssh-copy-id -i ~/.ssh/id_rsa.pub $TARGET_USER@$TARGET_NAME
    
        TARGET_DIR=/home/$TARGET_USER/test
        $TARGET_SHELL rm -Rf $TARGET_DIR
        $TARGET_SHELL mkdir -p $TARGET_DIR
        scp $WORKING_DIR/test/* $TARGET_USER@$TARGET_NAME:$TARGET_DIR
        # $TARGET_SHELL chmod +x $TARGET_DIR/*.sh
fi