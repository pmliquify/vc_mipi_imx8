// #include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
// #include <assert.h>
// #include <errno.h>
#include <fcntl.h>
// #include <dirent.h>
#include <sys/ioctl.h>
// #include <sys/types.h>
// #include <sys/stat.h>
#include <sys/mman.h>
// #include <sys/sysmacros.h>
#include <linux/videodev2.h>
// #include <linux/media.h>
// #include <linux/v4l2-subdev.h>




typedef struct {
	char **st;                              /*!<   multiple allocated image regions due to hardware */
	struct v4l2_plane *plane;
	size_t planeCount;                      /*!<   Number of regions          */
} QBuf;

int main(int argc, char *argv[])
{
        int fd = 0;
        struct v4l2_selection selection;
        __u32 type;
        int ret;

        printf("Open /dev/video0\n");
        fd = open("/dev/video0", O_RDWR, 0);
        if (fd < 0) {
                return -1;
        }

        printf("Set ROI\n");
        selection.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        selection.target = V4L2_SEL_TGT_CROP;
        selection.r.left = 0;
        selection.r.top = 0;
        selection.r.width = 3840;
        selection.r.height = 3040;
        ret = ioctl(fd, VIDIOC_S_SELECTION, &selection);

        printf("Start stream\n");
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ret = ioctl(fd, VIDIOC_STREAMON, type);

        return ret;
}