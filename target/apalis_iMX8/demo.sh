#!/usr/bin/env bash

usage() {
        echo "Usage: $0 [options]"
        echo ""
        echo "Script to test and demonstrate camera features."
        echo ""
        echo "Supported options:"
        echo "-a, --flash               Set the flash mode (Options: 0-1)"
	echo "-b, --blacklevel          Set the black level (Options: 0-255)"
        echo "-f, --format              Pixelformat (Options: GREY, 'Y10 ', 'Y12 ', 'Y14 ', RGGB, RG10, RG12)"
        echo "-g, --gain                Set the gain"
	echo "    --gst                 Uses a v4l2src gstreamer pipeline"
        echo "-h, --help                Show this help text"
        echo "-o, --option              Set the options"
	echo "-r, --framerate           Set the frame rate in Hz"
	echo "-p, --print               Print the device topology as a dot graph"
        echo "-s, --shutter             Set the shutter time in µs"
	echo "    --single              Executes a single trigger"
        echo "-t, --trigger             Set the trigger mode (Options: 0-7)"
	echo "-w, --disable-wayland     Disables wayland app"
}

disable_wayland_app() {
	systemctl stop wayland-app-launch
	systemctl disable wayland-app-launch
}

print_device_topology() {
	media-ctl -d /dev/media0 --print-dot > top.dot
	echo "sudo apt install -y graphviz"
	echo "dot -Tsvg top.dot -o top.svg"
}

single_trigger() {
        for i in {0..1000}; do
                value=$((i % 2))
                echo "[${i}] single trigger ${value}"
        	v4l2-ctl -d "/dev/v4l-subdev${subdevice}" -c single_trigger="${value}"
                sleep 1
        done
}
        
width=
height=

get_image_size() {
	sensor="$(journalctl -k | grep -oe '5-0.*SENSOR.*' | grep -oe 'IMX[0-9]*')"

	case "${sensor}" in
		IMX178) width=3072 height=2076 ;;
		IMX183) width=5440 height=3648 ;;
		IMX226) width=3840 height=3046 ;;
		IMX250) width=2448 height=2048 ;;
		IMX252) width=2048 height=1536 ;;
		IMX264) width=2432 height=2048 ;;
		IMX265) width=2048 height=1536 ;;
		IMX273) width=1440 height=1080 ;;
		IMX290) width=1920 height=1080 ;;
		IMX296) width=1440 height=1080 ;;
		IMX327) width=1920 height=1080 ;;
		IMX392) width=1920 height=1200 ;;
		IMX412) width=4032 height=3040 ;;
		IMX415) width=3840 height=2160 ;;
		OV9281) width=1280 height=800  ;;
		*)
			echo "Connected Sensor Type ${sensor} is unknown!"
			exit 1
		;;
	esac
}

device=0
subdevice=1
format=
trigger=
flash=
option2=
optionY="-y1"
shutter=10000
gain=0
framerate=
blacklevel=
gstreamer=

while [ $# != 0 ] ; do
	option="$1"
	shift

	case "${option}" in
        -a|--flash)
		flash="$1"
		shift
		;;
	-b|--blacklevel)
		blacklevel="$1"
		shift
		;;
        -d|--device)
		device="$1"
		shift
		;;
	-f|--format)
		format="$1"
		shift
		;;
	-g|--gain)
		gain="$1"
		shift
		;;
	--gst)
		gstreamer=1
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
        -o|--option)
		option2="$1"
		shift
		;;
        -p|--print)
		print_device_topology
		exit 0
		;;
	-r|--framerate)
		framerate="$1"
		shift
		;;
	-s|--shutter)
		shutter="$1"
		shift
		;;
	--single)
		single_trigger
		exit 0
		;;
        -t|--trigger)
		trigger="$1"
		shift
		;;
	-w|--disable-wayland)
		disable_wayland_app
		exit 0
		;;
	-y)
		optionY="-y$1"
		shift
		;;
	*)
		echo "Unknown option ${option}"
		exit 1
		;;
	esac
done

get_image_size
v4l2-ctl -d "/dev/video${device}" --set-fmt-video="width=${width},height=${height}"

if [[ -n ${format} ]]; then
        echo "Set format: ${format}"
        v4l2-ctl -d "/dev/video${device}" --set-fmt-video=pixelformat="${format}"
fi
if [[ -n ${framerate} ]]; then
        echo "Set frame rate: ${framerate}"
        v4l2-ctl -d "/dev/v4l-subdev${subdevice}" -c frame_rate="${framerate}"
fi
if [[ -n ${trigger} ]]; then
        echo "Set trigger mode: ${trigger}"
        v4l2-ctl -d "/dev/v4l-subdev${subdevice}" -c trigger_mode="${trigger}"
fi
if [[ -n ${flash} ]]; then
        echo "Set flash mode: ${flash}"
        v4l2-ctl -d "/dev/v4l-subdev${subdevice}" -c flash_mode="${flash}"
fi
if [[ -n ${blacklevel} ]]; then
        echo "Set black level: ${blacklevel}"
        v4l2-ctl -d "/dev/v4l-subdev${subdevice}" -c black_level="${blacklevel}"
fi

if [[ -n ${gstreamer} ]]; then
	gst-launch-1.0 -v --gst-debug-level=3 \
		 v4l2src device="/dev/video${device}" ! \
		"video/x-raw,width=${width},height=${height},format=NV12,framerate=0/1" ! \
                 videoconvert ! autovideosink

else 
	vcmipidemo -d"${device}" -2 -a"${option2}" "${optionY}" -s"${shutter}" -g"${gain}"
fi