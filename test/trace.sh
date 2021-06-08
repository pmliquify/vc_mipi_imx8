#/bin/bash
#
clear

TARGET_DIR=/home/root/test
TRACER=tracer.sh

. $TARGET_DIR/$TRACER $1
$TARGET_SHELL cat /sys/kernel/debug/tracing/trace 