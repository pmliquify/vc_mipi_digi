#/bin/bash
#
. config/configure.sh

if [[ $1 == "r" ]]; then
        rm -Rf $RECOVERY_DIR
fi

export PATH=$PATH:$RECOVERY_DIR

if [ ! -d $RECOVERY_DIR ]; then
        mkdir -p $RECOVERY_DIR
        cd $RECOVERY_DIR

        wget $IMAGE_URL/$IMAGE_FILE
        unzip $IMAGE_FILE
        rm $IMAGE_FILE
        chmod +x install_linux_fw_uuu.sh

        wget $UUU_URL/uuu
        chmod +x uuu
        sudo mv uuu /usr/bin

        sudo sh -c "uuu -udev >> /etc/udev/rules.d/99-uuu.rules"
        sudo udevadm control --reload-rules
fi

clear
echo "1. Connect a USB type-C cable between target and host."
echo "2. Reboot device in U-Boot console and selecting the USB interface you want it to listen to."
echo "   => fastboot 1"
echo "Press a key"
read -n 1

cd $RECOVERY_DIR
sudo bash install_linux_fw_uuu.sh imx-boot-ccimx8x-sbc-pro-B0-2GB_32bit.bin-flash
rm ~/.ssh/known_hosts