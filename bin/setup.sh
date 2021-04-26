#/bin/bash
#
. config/configure.sh


# Install Linux Tools
# *****************************************************************************
sudo apt update
sudo apt install -y bc build-essential git libncurses5-dev lzop perl libssl-dev
sudo apt install -y flex bison
sudo apt install -y gcc-aarch64-linux-gnu
sudo apt install -y device-tree-compiler

mkdir -p $WORKING_DIR/build
cd $WORKING_DIR/build

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
