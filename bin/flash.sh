#/bin/bash
#
. config/configure.sh $1

if [[ -n $1 ]]; then
        ./build.sh $1 $2
fi

if [[ -z $2 || $2 == "k" ]]; then
        scp $BUILD_DIR/image/Image.gz root@$TARGET_NAME:/boot
fi
if [[ -z $2 || $2 == "d" ]]; then
        # Verdin
        #scp $BUILD_DIR/image/$DTB_FILE root@$TARGET_NAME:/boot 
        
        # Apalis
        scp $BUILD_DIR/image/$DTO_FILE.dtbo root@$TARGET_NAME:/boot/overlays 
        scp $BUILD_DIR/image/overlays.txt root@$TARGET_NAME:/boot
fi
$TARGET_SHELL /sbin/reboot

printf "Waiting for $TARGET_NAME ..."
sleep 5
while ! ping -c 1 -n -w 1 $TARGET_NAME &> /dev/null
do
        printf "."
done
printf " OK\n\n"

$TARGET_SHELL dmesg | grep vc-mipi