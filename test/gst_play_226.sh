echo Play VC MIPI sensor IMX226 stream on desktop by GStreamer

export XDG_RUNTIME_DIR=/run/user/0
gst-launch-1.0 v4l2src device="/dev/video0" ! "video/x-raw,width=3840,height=3040,format=(string)GRAY8" !\
 videoconvert ! videoscale ! "video/x-raw,width=640,height=400" ! waylandsink -v sync=false