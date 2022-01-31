#!/usr/bin/env bash

usage() {
	echo "Usage: $0 [options]"
	echo ""
	echo "Loads Toradex Easy Installer."
	echo ""
	echo "Supported options:"
	echo "-h, --help                Show this help text"
}

configure() {
        . config/configure.sh
}

setup_tezi_recover() {
        if [[ ! -d $BUILD_DIR/$TEZI_RECOVER ]]; then
                cd $BUILD_DIR
                wget $TEZI_RECOVER_URL/$TEZI_RECOVER.zip
                unzip $TEZI_RECOVER.zip
                rm $TEZI_RECOVER.zip
        fi
}

recover() {
        cd $BUILD_DIR/$TEZI_RECOVER
        . recovery-linux.sh
        rm -f ~/.ssh/known_hosts
}

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "Unknown option ${option}"
		exit 1
		;;
	esac
done

configure
setup_tezi_recover
recover