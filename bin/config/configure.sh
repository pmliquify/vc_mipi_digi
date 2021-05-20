#/bin/bash
#
clear

WORKING_DIR=$(dirname $PWD)
BIN_DIR=$WORKING_DIR/bin
BUILD_DIR=$WORKING_DIR/build
KERNEL_SOURCE=$BUILD_DIR/linux
MODULES_DIR=$BUILD_DIR/modules
SRC_DIR=$WORKING_DIR/src/cc8x
TMP_DIR=$BUILD_DIR/downloads
GCC_DIR=$BUILD_DIR/toolchain

GCC_URL=https://ftp1.digi.com/support/digiembeddedyocto/3.0/r4/sdk/ccimx8x-sbc-pro/xwayland/
GCC_FILE=dey-glibc-x86_64-dey-image-qt-xwayland-aarch64-ccimx8x-sbc-pro-toolchain-3.0-r4.sh

RECOVERY_DIR=$BUILD_DIR/recovery
IMAGE_URL=https://ftp1.digi.com/support/digiembeddedyocto/3.0/r3/images
IMAGE_FILE=ccimx8x-sbc-pro-installer.zip
UUU_URL=https://github.com/NXPmicro/mfgtools/releases/download/uuu_1.4.72

# TARGET_NAME=ccimx8x-sbc-pro
TARGET_NAME=192.168.2.20
TARGET_USER=root
TARGET_SHELL="ssh $TARGET_USER@$TARGET_NAME"