#/bin/bash
#
clear

. config/configure.sh

TARGET_DIR=/home/root
TRACER=tracer.sh

scp $TRACER root@$TARGET_IP:$TARGET_DIR
$TARGET_SHELL . $TARGET_DIR/$TRACER $1
$TARGET_SHELL cat /sys/kernel/debug/tracing/trace 