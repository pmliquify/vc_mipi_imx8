echo Play VC MIPI sensor OV9281 stream on desktop by GStreamer

export XDG_RUNTIME_DIR=/run/user/0
gst-launch-1.0 v4l2src device="/dev/video0" ! "video/x-raw,width=1280,height=800,format=(string)GRAY8" !\
 videoconvert ! videoscale ! "video/x-raw,width=640,height=400" ! waylandsink -v sync=false