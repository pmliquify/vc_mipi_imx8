#/bin/bash
#
. config/configure.sh

if [[ ! -d $BUILD_DIR/$TEZI_RECOVER ]]; then
        cd $BUILD_DIR
        wget $TEZI_RECOVER_URL/$TEZI_RECOVER.zip
        unzip $TEZI_RECOVER.zip
        rm $TEZI_RECOVER.zip
fi

cd $BUILD_DIR/$TEZI_RECOVER
. recovery-linux.sh
rm -f ~/.ssh/known_hosts