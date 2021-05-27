#/bin/bash
#
. config/configure.sh

#if [[ $1 == "a" ]]; then
#        ./build.sh $1
#fi

if [[ $1 == "k" || $1 == "all" ]]; then
        scp $KERNEL_SOURCE/arch/arm64/boot/Image.gz root@$TARGET_NAME:/boot
fi
if [[ $1 == "m" || $1 == "all" ]]; then
        #scp -r $BUILD_DIR/modules/* root@$TARGET_NAME:/

        scp $BUILD_DIR/modules.tar.gz root@$TARGET_NAME:/home/root
        $TARGET_SHELL tar -xzf modules.tar.gz -C /
        $TARGET_SHELL rm modules.tar.gz
        $TARGET_SHELL rm -Rf /var/log
        $TARGET_SHELL mkdir /var/log
fi
if [[ $1 == "d" || $1 == "a" ]]; then
        # Verdin
        #scp $BUILD_DIR/image/$DTB_FILE root@$TARGET_NAME:/boot 
        
        # Apalis
        scp $KERNEL_SOURCE/arch/arm64/boot/dts/freescale/$DTO_FILE.dtbo root@$TARGET_NAME:/boot/overlays 
        scp $WORKING_DIR/misc/apalis/overlays.txt root@$TARGET_NAME:/boot
fi

$TARGET_SHELL /sbin/reboot

if [[ $2 == "b" || $2 == "s" ]]; then
        printf "Waiting for $TARGET_NAME ..."
        sleep 5
        while ! ping -c 1 -n -w 1 $TARGET_NAME &> /dev/null
        do
                printf "."
        done
        printf " OK\n\n"
fi

if [[ $2 == "b" ]]; then
        # Set loglevel=8 at boot time 
        # setenv defargs xxx loglevel=8
        $TARGET_SHELL dmesg | grep 'mxc\|5-00\|vc-mipi'
fi

if [[ $2 == "s" ]]; then
        $TARGET_SHELL dmesg -n 8
        $TARGET_SHELL v4l2-ctl --set-fmt-video=pixelformat=RGGB,width=3840,height=3040
        $TARGET_SHELL v4l2-ctl --stream-mmap --stream-count=1 --stream-to=file.raw
fi