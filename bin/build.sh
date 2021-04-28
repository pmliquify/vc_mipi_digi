#/bin/bash
#
. config/configure.sh

./patch.sh

unset LD_LIBRARY_PATH
. $GCC_DIR/environment-setup-aarch64-dey-linux

export LDFLAGS="-O1 --hash-style=gnu --as-needed"

cd $KERNEL_SOURCE
make kernelversion
make ccimx8_defconfig

if [[ -z $1 ]]; then 
        make -j$(nproc)
fi

if [[ $1 == "a" || $1 == "k" ]]; then 
        echo "Build Kernel ..."
        make -j$(nproc) Image.gz
fi
if [[ $1 == "a" || $1 == "m" ]]; then 
        echo "Build Modules ..."   
        rm -Rf $MODULES_DIR
        rm $BUILD_DIR/modules.tar.gz 
    
        make -j$(nproc) modules
        mkdir -p $MODULES_DIR
        export INSTALL_MOD_PATH=$MODULES_DIR
        make modules_install
        cd $MODULES_DIR
        echo Create module archive ...
        tar -czf ../modules.tar.gz .
        rm -Rf $MODULES_DIR
fi
if [[ $1 == "a" || $1 == "d" ]]; then 
        echo "Build Device Tree ..."
        make -j$(nproc) dtbs
fi

if [[ $1 == "v" ]]; then 
    cd $WORKING_DIR/src/vcmipidemo/src
    make clean
    make vcmipidemo
    cp $WORKING_DIR/src/vcmipidemo/src/vcmipidemo $WORKING_DIR/test
fi

#make -j8 htmldocs