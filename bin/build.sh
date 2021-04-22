#/bin/bash
#
. config/configure.sh

unset LD_LIBRARY_PATH
. $GCC_DIR/environment-setup-aarch64-dey-linux

export LDFLAGS="-O1 --hash-style=gnu --as-needed"

cd $KERNEL_SOURCE
make ccimx8_defconfig
make -j8