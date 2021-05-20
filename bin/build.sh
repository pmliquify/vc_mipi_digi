#/bin/bash
#
. config/configure.sh

create_modules() {
        rm -Rf $MODULES_DIR
        mkdir -p $MODULES_DIR
        export INSTALL_MOD_PATH=$MODULES_DIR
        make modules_install
        cd $MODULES_DIR
        echo Create module archive ...
        rm $BUILD_DIR/modules.tar.gz 
        tar -czf ../modules.tar.gz .
        rm -Rf $MODULES_DIR
}

if [[ ! $1 == "demo" ]]; then 
        if [[ -z $1 ]]; then 
                ./patch.sh f
        else
                ./patch.sh
        fi 
fi

unset LD_LIBRARY_PATH
. $GCC_DIR/environment-setup-aarch64-dey-linux
export LDFLAGS="-O1 --hash-style=gnu --as-needed"

if [[ ! $1 == "demo" ]]; then 
        cd $KERNEL_SOURCE
        make kernelversion
        make ccimx8_defconfig
fi
if [[ -z $1 ]]; then 
        make -j$(nproc)
        create_modules
fi
if [[ $1 == "a" || $1 == "k" ]]; then 
        echo "Build Kernel ..."
        make -j$(nproc) Image.gz
fi
if [[ $1 == "a" || $1 == "m" ]]; then 
        echo "Build Modules ..."      
        make -j$(nproc) modules
        create_modules
fi
if [[ $1 == "a" || $1 == "d" ]]; then 
        echo "Build Device Tree ..."
        make dtbs
fi
if [[ $1 == "test" ]]; then 
        cd $WORKING_DIR/src/vcmipidemo/linux
        make
        mkdir -p $WORKING_DIR/test
        mv vcmipidemo $WORKING_DIR/test
        mv vcimgnetsrv $WORKING_DIR/test
        mv vctest $WORKING_DIR/test
fi