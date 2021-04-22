#/bin/bash
#
. config/configure.sh


$TARGET_SHELL mount -o remount,rw /dev/mmcblk0p1 /mnt/linux

scp $KERNEL_SOURCE/arch/arm64/boot/Image.gz root@$TARGET_NAME:/mnt/linux/Image.gz-ccimx8x-sbc-pro.bin
scp $KERNEL_SOURCE/arch/arm64/boot/dts/digi/ccimx8x-sbc-pro.dtb root@$TARGET_NAME:/mnt/linux/

$TARGET_SHELL /sbin/reboot

printf "Waiting for $TARGET_NAME ..."
sleep 5
while ! ping -c 1 -n -w 1 $TARGET_NAME &> /dev/null
do
        printf "."
done
printf " OK\n\n"