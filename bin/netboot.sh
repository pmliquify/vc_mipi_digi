#/bin/bash
#
. config/configure.sh

SOURCE_DIR=$KERNEL_SOURCE/arch/arm64
NETBOOT_DIR=/exports/nfsroot-ccimx8x_sbc_pro

if [[ $1 == "all" || $1 == "k" ]]; then      
        sudo cp $SOURCE_DIR/boot/Image.gz $NETBOOT_DIR/boot/Image.gz--5.4-r0.2-ccimx8x-sbc-pro.bin
fi

#if [[ $1 == "all" || $1 == "m" ]]; then
#        sudo tar -xzvf $BUILD_DIR/modules.tar.gz -C $TARGET_DIR
#fi

if [[ $1 == "all" || $1 == "d" ]]; then
        sudo cp $SOURCE_DIR/boot/dts/digi/ccimx8x-sbc-pro.dtb $NETBOOT_DIR/boot/ccimx8x-sbc-pro--5.4-r0.2-ccimx8x-sbc-pro.dtb
fi

if [[ $1 == "test" ]]; then
        sudo mkdir -p $NETBOOT_DIR/home/root/test
        sudo cp $WORKING_DIR/test/* $NETBOOT_DIR/home/root/test
fi

if [[ $1 == "recover" ]]; then
        sudo rm -Rf $NETBOOT_DIR/*
        cd $TMP_DIR
        sudo tar xvfp dey-image-qt-fb-ccimx8x-sbc-pro.rootfs.tar.bz2 -C /exports/nfsroot-ccimx8x_sbc_pro
        sudo cp Image.gz--5.4-r0.2-ccimx8x-sbc-pro.bin $NETBOOT_DIR/boot
        sudo cp ccimx8x-sbc-pro--5.4-r0.2-ccimx8x-sbc-pro.dtb $NETBOOT_DIR/boot
        sudo cp _ov_som_wifi_ccimx8x--5.4-r0.2-ccimx8x-sbc-pro.dtbo $NETBOOT_DIR/boot
fi

if [[ $1 == "a" || $1 == "k" || $1 = "d" || $1 == "recover" ]]; then
        $TARGET_SHELL /sbin/reboot
fi