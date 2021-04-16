#/bin/bash
#
. config/configure.sh $1

sudo rm -Rf $BUILD_DIR/linux-toradex

sudo apt update
sudo apt install -y bc build-essential git libncurses5-dev lzop perl libssl-dev
sudo apt install -y flex bison
sudo apt install -y gcc-aarch64-linux-gnu
sudo apt install -y device-tree-compiler

cd ..
mkdir -p build
cd build
mkdir -p image

# Q1.2021
#git clone -b toradex_imx_v2020.04_5.4.24_2.1.0 git://git.toradex.com/u-boot-toradex.git
#git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/device-tree-overlays.git
git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/linux-toradex.git

# Ab Q2.2021
#git clone -b toradex_imx_v2020.04_5.4.70_2.3.0 git://git.toradex.com/u-boot-toradex.git
#git clone -b toradex_5.4-2.3.x-imx git://git.toradex.com/device-tree-overlays.git
#git clone -b toradex_5.4-2.3.x-imx git://git.toradex.com/linux-toradex.git