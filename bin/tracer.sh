#bin/bash
#

cd /sys/kernel/debug/tracing
echo  > trace
#echo nop > current_tracer
#echo function_graph > current_tracer
echo function > current_tracer

echo $1 > set_ftrace_filter
echo 1 > tracing_on

#v4l2-ctl -c exposure=1000
#v4l2-ctl -c gain=1000
#v4l2-ctl --set-fmt-video=pixelformat=Y10,width=1000,height=1000
v4l2-ctl --stream-mmap --stream-count=1 --stream-to=file.raw

echo 0 > tracing_on