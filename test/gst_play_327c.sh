#!/bin/bash

video_dev=/dev/video0
jetson_model=nano
# jetson_model=xavier
# jetson_model=tx2

function pause(){
   read -p "$*"
}

# set -x
# set -u
# set -e

rc=$?
if [ "$1" != "" ]; then video_dev=$1; fi

echo ---------------------------------------------------------------------------------
echo "  Play VC MIPI IMX327C sensor stream on Toradex by V4L/GStreamer"
echo "Usage:"
echo "   $0 [video_dev]"
echo "where:"
echo "   video_dev = Video device: /dev/video0 (def), /dev/video1, ..."
echo ---------------------------------------------------------------------------------
echo video_dev=$video_dev

VARS_NAME=v4l_read_vars.sh
/home/root/test/v4l_read -subs $SUBS_STEP -i $VARS_NAME -o -
#. $VARS_NAME

SUBS_STEP=1
V4L_FRAME_DX=1920
V4L_FRAME_DY=1080
V4L_FRAME_PITCH=3840
V4L_FRAME_SIZE=2073600

GST_PIXFMT=GRAY8
RAW_PIXFMT=gray8
# GST_PIXFMT=RGB

#SHOW_DX=$(( $V4l_FRAME_DX / 2 ))
#SHOW_DY=$(( $V4l_FRAME_DY / 2 ))

#echo Jetson model = $jetson_model
echo Frame dx,dy=$V4L_FRAME_DX,$V4L_FRAME_DY
echo Frame size=$V4L_FRAME_SIZE
# echo SHOW_DX,SHOW_DY=$SHOW_DX,$SHOW_DY

export XDG_RUNTIME_DIR=/run/user/0

/home/root/test/v4l_read -d $video_dev -w $V4L_FRAME_DX -h $V4L_FRAME_DY -f RG10 -m $jetson_model -subs $SUBS_STEP -n 0 -o - | \
gst-launch-1.0 -v --gst-debug-level=3 fdsrc blocksize=$V4L_FRAME_SIZE ! \
"video/x-raw,width=$V4L_FRAME_DX,height=$V4L_FRAME_DY,format=(string)$GST_PIXFMT,framerate=60/1" ! \
rawvideoparse width=$V4L_FRAME_DX height=$V4L_FRAME_DY format=$RAW_PIXFMT ! \
videoconvert ! \
waylandsink -v sync=false

