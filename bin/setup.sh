#/bin/bash
#
. config/configure.sh $1

sudo rm -Rf $BUILD_DIR

sudo apt update
sudo apt install -y bc build-essential git libncurses5-dev lzop perl libssl-dev
sudo apt install -y flex bison
sudo apt install -y gcc-aarch64-linux-gnu
sudo apt install -y device-tree-compiler

cd ..
mkdir build
cd build
mkdir image
git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/linux-toradex.git
git clone -b toradex_5.4-2.1.x-imx git://git.toradex.com/device-tree-overlays.git