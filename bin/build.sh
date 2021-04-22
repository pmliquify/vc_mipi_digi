#/bin/bash
#
. config/configure.sh

./patch.sh

unset LD_LIBRARY_PATH
. $GCC_DIR/environment-setup-aarch64-dey-linux

export LDFLAGS="-O1 --hash-style=gnu --as-needed"

cd $KERNEL_SOURCE
make ccimx8_defconfig

if [[ -z $1 ]]; then 
        make -j8

elif [[ $1 == "k" ]]; then 
        make -j8 vmlinux

elif [[ $1 == "d" ]]; then 
        make -j8 dtbs
fi

#make -j8 htmldocs