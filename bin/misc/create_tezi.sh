#/bin/bash
#
. config/configure.sh

#. build.sh a

MEDIA=$2
if [[ $1 == "c" ]]; then
        if [[ ! -d $MEDIA ]]; then
                echo $MEDIA is not present!
                exit
        fi
fi

mkdir -p $WORKING_DIR/build
cd $WORKING_DIR/build
rm -f $TEZI.tar
rm -Rf $TEZI
wget $TEZI_URL/$TEZI.tar
tar xf $TEZI.tar
rm -f $TEZI.tar

cd $TEZI
mkdir $BOOTFS
cd $BOOTFS
tar xJf ../$BOOTFS.tar.xz
cp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $WORKING_DIR/build/$TEZI/$BOOTFS
cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/*.dtb $WORKING_DIR/build/$TEZI/$BOOTFS
cp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo $WORKING_DIR/build/$TEZI/$BOOTFS/overlays
cp $MISC_DIR/* $WORKING_DIR/build/$TEZI/$BOOTFS
tar cJf ../$BOOTFS.tar.xz *
cd ..
rm -R $BOOTFS

sed -i 's/"description": "Image for BSP verification with QT and multimedia features"/"description": "Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8"/g' image.json
sed -i 's/"name": "Toradex Embedded Linux Reference Multimedia Image"/"name": "Vision Components MIPI CSI-2 driver for Toradex Apalis i.MX8"/g' image.json

if [[ $1 == "c" ]]; then
        cp -rvf $WORKING_DIR/build/$TEZI/* $MEDIA
        umount $MEDIA
fi