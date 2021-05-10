#/bin/bash
#
. config/configure.sh


$TARGET_SHELL mount -o remount,rw /dev/mmcblk0p1 /mnt/linux

if [[ $1 == "a" || $1 == "f" || $1 == "k" ]]; then
        scp $KERNEL_SOURCE/arch/arm64/boot/Image.gz $TARGET_USER@$TARGET_NAME:/mnt/linux/Image.gz-ccimx8x-sbc-pro.bin
fi
if [[ $1 == "a" || $1 == "f" || $1 == "m" ]]; then
        #scp -r $BUILD_DIR/modules/* $TARGET_USER@$TARGET_NAME:/

        scp $BUILD_DIR/modules.tar.gz $TARGET_USER@$TARGET_NAME:/home/$TARGET_USER
        $TARGET_SHELL tar -xzf modules.tar.gz -C /
        $TARGET_SHELL rm modules.tar.gz
fi
if [[ $1 == "a" || $1 == "f" || $1 == "d" ]]; then
        scp $KERNEL_SOURCE/arch/arm64/boot/dts/digi/ccimx8x-sbc-pro.dtb $TARGET_USER@$TARGET_NAME:/mnt/linux/
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
        $TARGET_SHELL dmesg | grep 'mxc\|16-00\|vc-mipi'
fi

if [[ $2 == "s" ]]; then
        $TARGET_SHELL dmesg -n 8
        $TARGET_SHELL v4l2-ctl --set-fmt-video=pixelformat=RGGB,width=3840,height=3040
        $TARGET_SHELL v4l2-ctl --stream-mmap --stream-count=1 --stream-to=file.raw
fi