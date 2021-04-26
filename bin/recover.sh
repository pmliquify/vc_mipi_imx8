#/bin/bash
#
. config/configure.sh

cd $WORKING_DIR/build/easy_apalis 
. recovery-linux.sh
rm ~/.ssh/known_hosts