#/bin/bash
#
. config/configure.sh

CMD=$1

if [[ $CMD == "host" ]]; then
        sudo apt install -y gawk wget git-core diffstat unzip texinfo 
        sudo apt install -y gcc-multilib g++-multilib build-essential 
        sudo apt install -y chrpath socat libsdl1.2-dev xterm minicom
        sudo apt install -y tftpd-hpa

        sudo rm -Rf $BUILD_DIR

        if [ ! -d $TMP_DIR ]; then
                mkdir -p $TMP_DIR
                cd $TMP_DIR
                wget $GCC_URL/$GCC_FILE
                chmod +x $GCC_FILE
                ./$GCC_FILE -y -d $GCC_DIR
        fi

        cd $BUILD_DIR
        git clone https://github.com/digi-embedded/linux.git -b v5.4/dey-3.0/maint

        # Tools to create kernel documentation
        #sudo apt-get install imagemagick dvipng fonts-noto-cjk latexmk librsvg2-bin texlive-xetex
        #/usr/bin/virtualenv sphinx_1.7.9
fi

if [[ $CMD == "netboot" ]]; then
        #sudo apt install -y tftpd-hpa
        #sudo mkdir -p /tftpboot
        #sudo chmod 1777 /tftpboot

        #TFTP_CFG=/etc/default/tftpd-hpa
        #sudo echo 'TFTP_USERNAME="tftp"' > $TFTP_CFG
        #sudo echo 'TFTP_DIRECTORY="/tftpboot"' >> $TFTP_CFG
        #sudo echo 'TFTP_ADDRESS="0.0.0.0:69"' >> $TFTP_CFG
        #sudo echo 'TFTP_OPTIONS="--secure"' >> $TFTP_CFG
        #sudo service tftpd-hpa restart

        sudo apt install -y nfs-kernel-server
        sudo mkdir -p /exports/nfsroot-ccimx8x_sbc_pro
        #sudo echo '/exports/nfsroot-ccimx8x_sbc_pro *(rw,no_root_squash,async,no_subtree_check)' >> /etc/exports
        #sudo service nfs-kernel-server restart

        cd $TMP_DIR
        DEY_IMG_URL=https://ftp1.digi.com/support/digiembeddedyocto/3.0/r4/images/ccimx8x-sbc-pro/fb
        wget $DEY_IMG_URL/Image.gz--5.4-r0.2-ccimx8x-sbc-pro.bin
        wget $DEY_IMG_URL/ccimx8x-sbc-pro--5.4-r0.2-ccimx8x-sbc-pro.dtb
        wget $DEY_IMG_URL/_ov_som_wifi_ccimx8x--5.4-r0.2-ccimx8x-sbc-pro.dtbo
        wget $DEY_IMG_URL/dey-image-qt-fb-ccimx8x-sbc-pro.rootfs.tar.bz2
        #wget $DEY_IMG_URL/dey-image-qt-fb-ccimx8x-sbc-pro.boot.vfat
        #wget $DEY_IMG_URL/dey-image-qt-fb-ccimx8x-sbc-pro.recovery.vfat
        #wget $DEY_IMG_URL/dey-image-qt-fb-ccimx8x-sbc-pro.rootfs.ext4
        #wget $DEY_IMG_URL/dey-image-qt-fb-ccimx8x-sbc-pro.rootfs.manifest    

        ./netboot.sh recover

        # => setenv autoload no
        # => dhcp
        # => setenv serverip 192.168.2.33
        # => setenv rootpath /exports/nfsroot-ccimx8x_sbc_pro
        # => setenv fdt_file boot/ccimx8x-sbc-pro--5.4-r0.2-ccimx8x-sbc-pro.dtb
        # => setenv overlays boot/_ov_som_wifi_ccimx8x--5.4-r0.2-ccimx8x-sbc-pro.dtbo
        # => setenv imagegz  boot/Image.gz--5.4-r0.2-ccimx8x-sbc-pro.bin
        # => setenv bootcmd 'dboot linux nfs'
        # => saveenv
fi

if [[ $CMD == "dey" ]]; then
        sudo curl -o /usr/local/bin/repo http://commondatastorage.googleapis.com/git-repo-downloads/repo
        sudo chmod a+x /usr/local/bin/repo

        sudo rm -Rf $BUILD_DIR/dey-3.0
        mkdir $BUILD_DIR/dey-3.0
        # sudo install -o <your-user> -g <your-group> -d $BUILD_DIR/dey-3.0
        cd $BUILD_DIR/dey-3.0
        repo init -u https://github.com/digi-embedded/dey-manifest.git -b refs/tags/3.0-r4.1
        repo sync -j8 --no-repo-verify

        sudo rm -Rf $BUILD_DIR/ccimx8x-sbc-pro
        mkdir $BUILD_DIR/ccimx8x-sbc-pro
        cd $BUILD_DIR/ccimx8x-sbc-pro
        . $BUILD_DIR/dey-3.0/mkproject.sh -p ccimx8x-sbc-pro
fi

if [[ $CMD == "test" ]]; then
        #rm ~/.ssh/known_hosts
        #ssh-copy-id -i ~/.ssh/id_rsa.pub $TARGET_USER@$TARGET_NAME
    
        TARGET_DIR=/home/$TARGET_USER/test
        $TARGET_SHELL rm -Rf $TARGET_DIR
        $TARGET_SHELL mkdir -p $TARGET_DIR
        scp $WORKING_DIR/test/* $TARGET_USER@$TARGET_NAME:$TARGET_DIR
        # $TARGET_SHELL chmod +x $TARGET_DIR/*.sh
fi