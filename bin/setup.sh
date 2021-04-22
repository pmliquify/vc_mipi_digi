#/bin/bash
#
. config/configure.sh

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